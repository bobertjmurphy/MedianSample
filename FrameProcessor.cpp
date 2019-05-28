//
//  FrameProcessor.cpp
//  sample_p
//
//  Created by Bob Murphy on 5/27/19.
//  Copyright © 2019 Nashi Software. All rights reserved.
//

#include "FrameProcessor.hpp"

#include <sstream>
#include <algorithm>

#if defined(__cplusplus)
extern "C" {
#endif
    
#include <libavcodec/avcodec.h>
#include <libavutil/common.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include "libswscale/swscale.h"
    
#if defined(__cplusplus)
}
#endif


FrameProcessor::FrameProcessor(AVStream *stream, AVCodecContext* codecContext, int gridRows, int gridCols) :
m_AVStream(stream),
m_AVCodecContext(codecContext),
m_GridRows(gridRows),
m_GridCols(gridCols),
m_SwsContext(nullptr)
{
    assert(m_AVStream != nullptr);
    assert(m_AVCodecContext != nullptr);
}

FrameProcessor::~FrameProcessor()
{
    if (m_SwsContext) {
        sws_freeContext(m_SwsContext);
    }
}


void FrameProcessor::ProcessKeyFrame(AVFrame *frame)
{
    FrameData frameData;
    
    // Determine the frame time
    int64_t presentationTime = frame->pts;
    presentationTime -= m_AVStream->start_time;
    AVRational timeBase = m_AVStream->time_base;
    double frameTimeInSeconds = ((double)presentationTime * (double)timeBase.num) / (double)timeBase.den;
    frameData.m_Timestamp = frameTimeInSeconds;
    
    
    // Get a context for converting the image, recreating one if needed
    int w = frame->width;
    int h = frame->height;
    AVPixelFormat sourcePixelFormat = (AVPixelFormat)frame->format;
    AVPixelFormat destPixelFormat = AV_PIX_FMT_GRAY8;   // 8 bits/pixel grayscale
    int scalingAlgorithm = 0;        // No scaling here; could be SWS_FAST_BILINEAR, etc.
    m_SwsContext = sws_getCachedContext(m_SwsContext,               // Reuse the old one if the args match
                                        w, h, sourcePixelFormat,    // Source image info
                                        w, h, destPixelFormat,      // Dest image size
                                        scalingAlgorithm, 
                                        NULL,               // No source filter
                                        NULL,               // No dest filter
                                        NULL);              // The nonexistent scaling algorithm doesn't need tuning
    
    // Allocate an array to hold the grayscale values
    int destBytesPerPixel = 1;
    int destRowBytes = w * destBytesPerPixel;
    int destImageSize = h * destRowBytes;
    uint8_t *destImageBuffer = new uint8_t[destImageSize];
    
    // Do the conversion
    int outputSliceHeight = ::sws_scale(m_SwsContext,
                                      frame->data, frame->linesize,
                                      0, h,                                                // Convert all source rows, starting at row 0
                                      &destImageBuffer, &destRowBytes);
    assert(outputSliceHeight == h);

    
    // Determine the dest image's grid cell sizes. These are rounded up so that, for instance,
    // if dividing a 100x100 image into 3x3 cells, you don't wind up with 33 rows x 33 columns
    // per cell, and wind up with some pixels not getting counted.
    int imageRowsInGridCell = ceil((double)h / (double)m_GridRows);
    int imageColsInGridCell = ceil((double)w / (double)m_GridCols);
    
    // Prepare to collect the values in each of the cells
    typedef std::vector<uint8_t> uint8_t_vector;
    uint8_t_vector accum[m_GridRows][m_GridCols];
    
    // Reserve space in the accumulators to avoid memory thrashing
    int maxGridCellPixelCount = imageRowsInGridCell * imageColsInGridCell;
    for (size_t thisGridRow = 0; thisGridRow < m_GridRows; thisGridRow++) {
        for (size_t thisGridCol = 0; thisGridCol < m_GridCols; thisGridCol++) {
            uint8_t_vector &thisAccum = accum[thisGridRow][thisGridCol];
            thisAccum.reserve(maxGridCellPixelCount);
        }
    }
    
    // Walk destImageBuffer, figure out where each offset is in the grid, and apply the values
    // to the accumulators
    uint8_t *thisGrayscaleValuePtr = destImageBuffer;
    for (size_t thisImageRow = 0; thisImageRow < h; thisImageRow++) {
        
        // Convert the image row to a grid row
        size_t thisGridRow = thisImageRow / imageRowsInGridCell;
        assert(thisGridRow < m_GridRows);
        
        for (size_t thisImageCol = 0; thisImageCol < w; thisImageCol++) {
            
            // Convert the image column to a grid column
            size_t thisGridCol = thisImageCol / imageColsInGridCell;
            assert(thisGridCol < m_GridCols);
                   
            // Accumulate this value
            uint8_t_vector &thisAccum = accum[thisGridRow][thisGridCol];
            thisAccum.push_back(*thisGrayscaleValuePtr);
            
            // Prepare for the next iteration
            thisGrayscaleValuePtr++;
        }
    }
    
    // Calculate and store the median values for the grid cells
    for (size_t thisGridRow = 0; thisGridRow < m_GridRows; thisGridRow++) {
        for (size_t thisGridCol = 0; thisGridCol < m_GridCols; thisGridCol++) {
            // Sort the accumulated values
            uint8_t_vector &thisAccum = accum[thisGridRow][thisGridCol];
            std::sort(thisAccum.begin(), thisAccum.end());
            
            // Determine and store the median
            size_t valueCount = thisAccum.size();
            size_t halfValueCount = valueCount / 2;
            uint8_t median;
            if (valueCount & 0x01) {        // The count is odd
                // The median is the middle value
                median = thisAccum[halfValueCount + 1];
            }
            else {                          // The count is even
                // The median is the mean of the two middle values
                // Do some casting to avoid overflow in the addition
                median = ((int)thisAccum[halfValueCount] + (int)thisAccum[halfValueCount + 1]) / 2;
            }
            frameData.m_CellGrayMedians.push_back(median);
        }
    }
    
    // Cache the result for this frame, and clean up
    m_FrameData.push_back(frameData);
    delete [] destImageBuffer;
}


std::string FrameProcessor::Report() const
{
    std::ostringstream accum;
    
    for (const FrameData &fr : m_FrameData) {
        accum << fr.m_Timestamp;
        
        for (int med : fr.m_CellGrayMedians) {
            accum << "," << med;
        }
        
        accum << std::endl;
    }
    
    std::string result = accum.str();
    return result;
}

