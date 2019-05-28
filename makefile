CC = clang-7
CXX = clang++-7
RM=rm -f

CFLAGS = -g -Wall
CXXFLAGS = $(CFLAGS) -std=c++11 -I.

# DEPS=CPP.h
SRCS = sample_p.cpp
PROG = sample_p
OBJ = sample_p.o


FFMPEG_PKGS= libavdevice libavformat libavfilter libavcodec  libswresample libswscale  libavutil
PKGCONFIG= pkg-config
FFMPEG_CFLAGS:=  $(shell $(PKGCONFIG) --cflags $(FFMPEG_PKGS))
FFMPEG_LIBFLAGS:= $(shell $(PKGCONFIG) --libs $(FFMPEG_PKGS)) -ldl

# compile only, C source
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(FFMPEG_CFLAGS)

# compile only, C++ source
%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(FFMPEG_CFLAGS)

# link
$(PROG): $(OBJ)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(FFMPEG_LIBFLAGS)
	
clean:
	$(RM) $(OBJ) $(PROG)
