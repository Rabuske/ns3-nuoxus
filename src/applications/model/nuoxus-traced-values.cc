#include "nuoxus-traced-values.h"

namespace ns3
{

NuoxusTracedRequest::NuoxusTracedRequest(Ipv4Address nodeId, Ipv4Address requester, uint32_t contentId, bool wasCached, bool isCached)
{
    m_request_time = Simulator::Now();
    m_nodeId       = nodeId;
    m_requester    = requester;
    m_contentId    = contentId;
    m_was_cached   = wasCached;
    m_is_cached    = isCached;
}

void NuoxusTracedRequest::SetTime(Time time)
{
    m_request_time = time;
}

void NuoxusTracedRequest::SetRequester(Ipv4Address requester)
{
    m_requester = requester;
}

void NuoxusTracedRequest::SetNodeId(Ipv4Address nodeId)
{
    m_nodeId = nodeId;
}


void NuoxusTracedRequest::SetContentId(uint32_t contentId)
{
    m_contentId = contentId;
}

void NuoxusTracedRequest::SetWasCached(bool wasCached)
{
    m_was_cached = wasCached;
}

void NuoxusTracedRequest::SetIsCached(bool isCached)
{
    m_is_cached = isCached;
}

Time NuoxusTracedRequest::GetTime(void)
{
    return m_request_time;
}

Ipv4Address NuoxusTracedRequest::GetNodeId(void)
{
    return m_nodeId;
}

Ipv4Address NuoxusTracedRequest::GetRequester(void)
{
    return m_requester;
}

uint32_t NuoxusTracedRequest::GetContentId(void)
{
    return m_contentId;
}

bool NuoxusTracedRequest::GetWasCached(void)
{
    return m_was_cached;
}

bool NuoxusTracedRequest::GetIsCached(void)
{
    return m_is_cached;
}

bool NuoxusTracedRequest::operator==(const NuoxusTracedRequest& comp)
{
    return (this->m_request_time == comp.m_request_time &&  
            this->m_nodeId     == comp.m_nodeId &&         
            this->m_requester  == comp.m_requester && 
            this->m_contentId  == comp.m_contentId &&
            this->m_was_cached == comp.m_was_cached &&
            this->m_is_cached  == comp.m_is_cached
           );   
}
                 
bool NuoxusTracedRequest::operator!=(const NuoxusTracedRequest& comp)
{
    return !(*this == comp);
}

}