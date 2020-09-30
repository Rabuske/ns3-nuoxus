#ifndef VIDEO_LIBRARY_H
#define VIDEO_LIBRARY_H

#include "ns3/video-data.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ptr.h"
#include <map>

namespace ns3{

class VideoLibrary: public SimpleRefCount<VideoData, Object>
{
    public:
        VideoLibrary() = default;            // Default constructor, avoiding linking errors
        void Initialize(uint32_t size, double mean, double variance);
        Ptr<VideoData> GetVideo(uint32_t id);
	
    private:        	       
        std::map<uint32_t, Ptr<VideoData>> m_library;       
};

}

#endif
