#!/bin/bash
##
# Bourne shell test for sample_p on macOS
##

START_TIME_IN_SECONDS="$(date +%s)"

# Utility routines
convert_secs_to_hours_minutes_and_seconds() {
	((h=${1}/3600))
	((m=(${1}%3600)/60))
	((s=${1}%60))
	printf "%02d:%02d:%02d\n" $h $m $s
}

# Set up some variables
MOVIES_DIR="./sample_files/"
RESULTS_DIR="./test_results/"
EXE_FILE="./macbuild/Debug/sample_p"

SAMPLE_MOVIES=( \
	05_blackhole.mp4 \
	four_clips.mp4 \
	four_clips_small.mov \
	festival_1.mp4 \
	four_clips_h265_big.mp4 \
	festival_1.mpg \
	four_clips_h265_small.mp4
	)
	
# Routine to run an individual set of tests
# Example: run_test_set 3x3
run_test_set() {
	DIMENSIONS=$1
	echo
	echo "Testing dimensions:" ${DIMENSIONS}
	for MOVIE in ${SAMPLE_MOVIES[@]}; do
		echo -n "    $MOVIE"
		SRC_MOVIE_PATH=${MOVIES_DIR}${MOVIE}
		DEST_FILE=${DIMENSIONS}_${MOVIE}_results.txt
		DEST_PATH=${RESULTS_DIR}${DEST_FILE}
        COMMAND="${EXE_FILE} --input ${SRC_MOVIE_PATH} --dim ${DIMENSIONS} --output ${DEST_PATH}"
        eval "${COMMAND}" 2> /dev/null		# libav can emit unimportant warnings
        RESULT=$?
        if [ $RESULT -ne 0 ]; then
    		echo " FAILED"
    	else
    		echo
        fi
	done
}


# Make sure the results directory exists and is empty
if [ -d "${RESULTS_DIR}" ]; then
    cd "${RESULTS_DIR}"
    rm -rf *
    cd ..
else
    mkdir "${RESULTS_DIR}"
fi

# These should succeed
echo
echo "============================================================"
echo "Tests should all succeeed"
echo "============================================================"
run_test_set "3x3"
run_test_set "16x16"
run_test_set "49x20"

# These should fail
echo
echo "============================================================"
echo "Tests should all fail"
echo "============================================================"
run_test_set "0x1"		# Dimension is too small
run_test_set "1x0"		# Dimension is too small
run_test_set "9999x3"	# Dimension is larger than image dimension
run_test_set "3x9999"	# Dimension is larger than image dimension

# Final report
echo
echo "============================================================"
END_TIME=`date +"%T"`
END_TIME_IN_SECONDS="$(date +%s)"
ELAPSED_SECONDS="$(expr $END_TIME_IN_SECONDS - $START_TIME_IN_SECONDS)"
FORMATTED_HMS=$(convert_secs_to_hours_minutes_and_seconds $ELAPSED_SECONDS)
echo "Elapsed time: ${FORMATTED_HMS}  (${ELAPSED_SECONDS} seconds)"
echo "Finished at:  ${END_TIME}"

