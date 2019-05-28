INTRODUCTION
============

sample_p is an implementation of the following coding problem:

    Given a video file and dimensions (e.g. 32x32, 64x64, 128x128), 
    extract keyframes from the video, convert the frame into grayscale, 
    split each frame into a grid of said dimensions, calculate median 
    value of all the pixels of each cell of the grid and write the values to a CSV file
    together with the timestamp of the frame.

    Example:

    If a frame timestamp (in seconds) is 3.14 and the dimensions are 3x3 an example line
    might look like:
        3.14,42,255,9,13,67,0,27,33,123  // timestamp + 9 values (3x3) 


DEVELOPMENT
===========

This software was developed using macOS 10.13.6, built with Xcode 9.4.1, and static
libraries built from ffmpeg 4.1.3

To make the code clearer to follow, sample_p doesn't use facilities that might enhance
performance in a production environment, such as threading, libdispatch, OpenCV, OpenCL,
etc.


TESTING
=======

Keyframe Recognition
--------------------

The keyframe recognition code was verified using several .mp4 and .mov files as follows:
1. Use Apple's Atom Inspector to open the file, find the video track, and examine the stss
   atom's contents, which contains the total keyframe count and the sample number for each
   keyframe.
2. Use this program to analyze the file, and check its reports of keyframe count and
   sample numbers against the Atom Inspector results.

Keyframe Analysis
-----------------

This was checked by making sure the same keyframe gave substantially the same results
across transformations:

1. An original movie file was imported into a transcoding utility.

2. The movie was then exported using a different container format, image resoltion, and
codec.

3. Both movies were analyzed with sample_p.

4. Because of the transformations, the two movies couldn't usually be expected to have
identical keyframe times, or identical median values for the grid. So the pairs of results
from step 3 were scanned for similar times nd such times' median values were reviewed for
similarity. Two sets of median values were considered to be "okay" if the differences
were minor, since changes would be inevitable due to slightly different times, and image
color changes caused by different compression algorithms' color handling and losses.

For example, these two sets of values were considered to be "okay":
40,59,140,125,80,64,75,83,55,72,47,40,65
40.04,59,141,126,80,64,76,85,54,72,49,40,66

The git repository contains these sets of movies that passed this kind of check:

Festival:
- festival_1.mp4: The original file, 568x320, 30 fps, MPEG-4 container, AVC/H.264 codec
- festival_1.mpg: Transcoded, 720x480, 29.97 fps, MPEG-2 PS container, MPEG-2 codec

Four Clips: All are derived from an original 106.3 MB movie
- four_clips_small.mov: 960x720, 23.976 fps, MPEG-4 container, HEVC/H.265 codec
- four_clips_h265_small.mp4: 480x360, 23.976 fps, MPEG-4 container, HEVC/H.265 codec
- four_clips_h265_big.mp4: 240x180, 23.976 fps, QuickTime container, ProRes 422 Proxy codec
- four_clips.mp4: 960x720, 23.976 fps, MPEG-4 container, AVC/H.264 codec

Some other movies were also tested that were too large, to be suitable for submission
to github.

Test Script
-----------

Once sample_p is built as a debug executable, cd to the top-level directory, and run
test_mac_debug.sh.

This bash script creates a test_results directory, runs sample_p against all the sample movie files
using a variety of grid dimensions, and puts the results into test_results.

test_mac_debug.sh runs both positive tests, where sample_p is expected to succeed, and negative
tests, where it's expected to fail due to invalid grid dimensions.

The results from this were verified by:
- Examining all results from the same movie set, and that use the same grid dimensions
- Compare those result files
- Spot-check some simlar keyframe times to make sure the median value patterns were similar

