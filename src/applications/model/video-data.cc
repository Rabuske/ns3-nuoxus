#include "video-data.h"

namespace ns3{

    VideoData::VideoData(void)
    {
        m_id = 0;
        m_duration = 0;
    }

    void VideoData::SetId(uint32_t id)
    {
        m_id = id;
    }

    void VideoData::SetDurationSeconds(uint32_t duration)
    {
        m_duration = duration;
    }

    uint32_t VideoData::GetId()
    {
        return m_id;
    }

    uint32_t VideoData::GetDurationSeconds()
    {
        return m_duration;
    }

    uint32_t VideoData::GetNumberOfPackets(uint32_t packetSize)
    {
        if(m_duration == 0) return 1;
        uint64_t dataSize = m_duration * 5000; // 5000 - kbps of the standard video format we are using
        if((dataSize % packetSize) == 0)
        {
            return dataSize / packetSize;
        }        
        else
        {
            return (dataSize / packetSize) + 1;
        }
    }

}
