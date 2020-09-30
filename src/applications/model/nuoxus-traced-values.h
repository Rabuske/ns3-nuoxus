#ifndef NUOXUS_TRACED_VALUES_H
#define NUOXUS_TRACED_VALUES_H

#include "ns3/uinteger.h"
#include "ns3/object.h"
#include "ns3/simple-ref-count.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/traced-value.h"
#include "ns3/object-factory.h"
#include "ns3/nstime.h"
#include "ns3/ipv4-address.h"
#include "ns3/simulator.h"

namespace ns3 {

class NuoxusTracedRequest  
{
public:
    NuoxusTracedRequest(){};
    ~NuoxusTracedRequest(){};
    NuoxusTracedRequest(Ipv4Address nodeId, Ipv4Address requester, uint32_t contentId, bool wasCached, bool isCached);

    void SetTime(Time time);       
    void SetNodeId(Ipv4Address nodeId);    
    void SetRequester(Ipv4Address requester);
    void SetContentId(uint32_t contentId);
    void SetWasCached(bool wasCached);
    void SetIsCached(bool isCached);

    Time        GetTime(void);
    Ipv4Address GetNodeId(void);    
    Ipv4Address GetRequester(void);
    uint32_t    GetContentId(void);
    bool        GetWasCached(void);
    bool        GetIsCached(void);

    bool operator==(const NuoxusTracedRequest& comp);    
    bool operator!=(const NuoxusTracedRequest& comp);

private:
    Time         m_request_time;
    Ipv4Address  m_nodeId;
    Ipv4Address  m_requester;
    uint32_t     m_contentId;
    bool         m_was_cached;
    bool         m_is_cached;
};    

}

#endif