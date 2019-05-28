// sample_p.cpp
// Video analysis sample program
// Bob Murphy, May 2019

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>


#if defined(__cplusplus)
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavutil/common.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
    
#if defined(__cplusplus)
}
#endif

#include "CommandLine.h"
#include "FrameProcessor.hpp"

#if 0       // Enable when needed
#define LOG printf
#else
inline void LOG(...) {}
#endif


#pragma mark - main()

// Returns whether this should be called again to try to process video frames on the same packet
static void prvProcessPacket(FrameProcessor &frameProcessor, AVPacket *packet,
                             AVCodecContext *codecContext, AVFrame *frame,
                             size_t &frameCount, size_t &keyframeCount)
{
    // Supply raw packet data as input to a decoder
    int packetSendResponse = avcodec_send_packet(codecContext, packet);
    assert(packetSendResponse >= 0);
    
    // Get the frames from this packet
    /// \todo Improve this by making it asynchronous and using a notification callback
    int receiveFrameResponse = avcodec_receive_frame(codecContext, frame);
    while (receiveFrameResponse >= 0) {
        frameCount++;
        
        if (frame->key_frame) {
            keyframeCount++;
            LOG("Keyframe %zu at sample %zu\n", keyframeCount, frameCount);
            frameProcessor.ProcessKeyFrame(frame);
        }
        
        receiveFrameResponse = avcodec_receive_frame(codecContext, frame);
    }
}


int main(int argc, char **argv)
{

    // Interpret the command line arguments
    CommandLineArguments cliArgs = ProcessCommandLine(argc, argv);
    LOG("Input file: \"%s\"\n", cliArgs.m_InputFilepath.c_str());
    
    
    // Open the input file and determine its format
    AVFormatContext *formatContext = avformat_alloc_context();
    avformat_open_input(&formatContext,
                        cliArgs.m_InputFilepath.c_str(),
                        NULL,           // Auto-detect the format
                        NULL);          // Don't use any private options
    LOG("Format %s, duration %lld µs (%f sec.)\n", formatContext->iformat->long_name,
           formatContext->duration, (double)formatContext->duration / (double)AV_TIME_BASE);
    
    
    // Look for video streams. It's unlikely there will be more than one, and this code only
    // pays attention to the first one, but it could be extended to handle multiple video streams.
    std::vector<int> videoStreamIndices;
    avformat_find_stream_info(formatContext,  NULL);
    for (int i = 0; i < formatContext->nb_streams; i++) {
        AVStream* thisStream = formatContext->streams[i];
        if (thisStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndices.push_back(i);
        }
    }
    if (videoStreamIndices.empty()) {
        fprintf(stderr, "No video found\n");
        exit(-1);
    }
    if (videoStreamIndices.size() > 1) {
        fprintf(stderr, "%zu video streams found; only the first will be analyzed\n", videoStreamIndices.size());
    }
    
    
    // Determine information about the first video stream
    int mainVideoStreamIndex = videoStreamIndices.front();
    AVStream* mainVideoStream = formatContext->streams[mainVideoStreamIndex];
    LOG("Main video stream has %lld frames\n", mainVideoStream->nb_frames);
    AVCodecParameters* mainVideoStreamParameters = mainVideoStream->codecpar;
    int imageWidth = mainVideoStreamParameters->width;
    int imageHeight = mainVideoStreamParameters->height;
    LOG("Video image size: %dx%d\n", imageWidth, imageHeight);
    if (imageWidth < cliArgs.m_Cols || imageHeight < cliArgs.m_Rows) {
        fprintf(stderr, "Video image is smaller smaller than the grid\n");
        exit(-1);
    }
    AVCodec *codec = avcodec_find_decoder(mainVideoStreamParameters->codec_id);
    if (codec == NULL) {
        fprintf(stderr, "Can't find a codec for the video stream\n");
        exit(-1);
    }
    
    
    // Set up a codec context
    AVCodecContext *codecContext = avcodec_alloc_context3(codec);
    if (codecContext == NULL) {
        fprintf(stderr, "Can't allocate a codec context for the video stream\n");
        exit(-1);
    }
    int status = avcodec_parameters_to_context(codecContext, mainVideoStreamParameters);
    assert(status >= 0);
    status = avcodec_open2(codecContext, codec, NULL);
    assert(status >= 0);
    
    
    // Set up a frame and a packet
    AVFrame *frame = av_frame_alloc();
    assert(frame != nullptr);
    AVPacket *packet = av_packet_alloc();
    assert(packet != nullptr);

    
    // Set up a frame processor
    FrameProcessor frameProcessor(mainVideoStream, codecContext, cliArgs.m_Rows, cliArgs.m_Cols);
    
    // Process all the packets to look for frames
    size_t frameCount = 0;
    size_t keyframeCount = 0;
    int frameReadStatus = av_read_frame(formatContext, packet);
    while (frameReadStatus >= 0) {
        
        if (packet->stream_index == mainVideoStreamIndex) {
            prvProcessPacket(frameProcessor, packet, codecContext, frame, frameCount, keyframeCount);
        }
        
        // Prepare for the next iteration
        av_packet_unref(packet);
        frameReadStatus = av_read_frame(formatContext, packet);
    }
    
    
    // Process any cached frames
    av_packet_unref(packet);
    packet->stream_index = mainVideoStreamIndex;
    prvProcessPacket(frameProcessor, packet, codecContext, frame, frameCount, keyframeCount);
    
    // Report the results
    LOG("Found %zu video frames, %zu keyframes\n", frameCount, keyframeCount);
    std::string results = frameProcessor.Report();
    if (cliArgs.m_OutputFilepath.empty()) {
        std::cout << results << std::endl;
    }
    else {
        std::ofstream ofs;
        ofs.open (cliArgs.m_OutputFilepath, std::ofstream::out | std::ofstream::trunc);
        ofs << results << std::endl;
    }
    
    
    // Clean up
    avformat_close_input(&formatContext);
    avformat_free_context(formatContext);
    av_packet_free(&packet);
    av_frame_free(&frame);
    avformat_free_context(formatContext);

    return 0;
}
