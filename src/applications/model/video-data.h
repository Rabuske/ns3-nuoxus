#ifndef VIDEO_DATA_H
#define VIDEO_DATA_H

#include "ns3/uinteger.h"
#include "ns3/object.h"
#include "ns3/simple-ref-count.h" // So we can turn this into a smart object

namespace ns3{


class VideoData : public SimpleRefCount<VideoData, Object>
{
    public:
	VideoData(void);
        uint32_t GetId();
        uint32_t GetDurationSeconds();
        uint32_t GetNumberOfPackets(uint32_t packetSize);
        void SetId(uint32_t id);
        void SetDurationSeconds(uint32_t duration);

    private:
        uint32_t m_id;
        uint32_t m_duration; // in seconds                
};

}

#endif
