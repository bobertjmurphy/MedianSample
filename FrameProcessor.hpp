//
//  FrameProcessor.hpp
//  sample_p
//
//  Created by Bob Murphy on 5/27/19.
//  Copyright © 2019 Nashi Software. All rights reserved.
//

#ifndef FrameProcessor_hpp
#define FrameProcessor_hpp

#include <string>
#include <vector>

// Foreward declarations
struct AVFrame;
struct AVStream;
struct AVCodecContext;
struct SwsContext;

class FrameProcessor
{
public:
    FrameProcessor(AVStream *stream, AVCodecContext* codecContext, int gridRows, int gridCols);
    ~FrameProcessor();
    
    void ProcessKeyFrame(AVFrame *frame);
    
    std::string Report() const;
    
protected:
    AVStream*       m_AVStream;
    AVCodecContext* m_AVCodecContext;
    int             m_GridRows;
    int             m_GridCols;
    
    struct FrameData {
        double  m_Timestamp;
        std::vector<int> m_CellGrayMedians;
    };
    std::vector<FrameData> m_FrameData;
    
    SwsContext*     m_SwsContext;
};

#endif /* FrameProcessor_hpp */
