#include "ns3/video-library.h"
#include "ns3/random-variable-stream.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"

namespace ns3{

NS_LOG_COMPONENT_DEFINE ("VideoLibrary");

void VideoLibrary::Initialize(uint32_t size, double mean, double variance)
{
    Ptr<NormalRandomVariable> generator = CreateObject<NormalRandomVariable>();
    generator->SetAttribute ("Mean", DoubleValue (mean));
    generator->SetAttribute ("Variance", DoubleValue (variance));
    // The expected value for the mean of the values returned by a normally distributed random variable is equal to mean.
    for(uint32_t i = 1; i <= size; i++)
    {
        Ptr<VideoData> video = CreateObject<VideoData>();
        video->SetId(i);
        video->SetDurationSeconds((uint32_t)generator->GetValue());
        m_library[i] = video;
        NS_LOG_INFO ("Video Created:  " << video->GetId() <<
                      " duration "<< video->GetDurationSeconds());
        
    }
}

Ptr<VideoData> VideoLibrary::GetVideo(uint32_t id)
{
    NS_LOG_FUNCTION(this << id );
    return m_library[id];
}

}
