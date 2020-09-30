
#include "nuoxus.h"
#include "ns3/inet-socket-address.h"
#include "ns3/log.h"
#include "nuoxus-header.h"
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include <exception>

namespace ns3 {
    
NS_LOG_COMPONENT_DEFINE ("Nuoxus");
NS_OBJECT_ENSURE_REGISTERED (Nuoxus);


TypeId Nuoxus::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::Nuoxus")
        .SetParent<Object>()
        .AddConstructor<Nuoxus> ()
        .AddTraceSource("NuoxusRequest",
                        "A request made by a client to Nuoxus",
                        MakeTraceSourceAccessor(&Nuoxus::m_traced_request))
	;
    return tid;
}

Nuoxus::Nuoxus()
{
  NS_LOG_FUNCTION (this);
}

Nuoxus::~Nuoxus ()
{
  NS_LOG_FUNCTION (this);
}

void Nuoxus::Initialize(uint32_t packetSize, TypeId cachingStrategy, Ipv4Address nodeId, uint64_t diskSizeKb, bool enableProactiveCaching)
{
    NS_LOG_FUNCTION(this << packetSize << cachingStrategy << nodeId);
    m_this_node_id = nodeId;
    m_caching_strategy_tid = cachingStrategy;
    m_packet_size = packetSize;
    m_cache_manager    = CreateObject<NuoxusCacheManager>(m_packet_size, diskSizeKb);
    m_history_manager  = CreateObject<NuoxusHistoryManager>();
    m_cache_manager->SetDiskSize(diskSizeKb);
    m_caching_strategy = NuoxusCachingStrategyFactory::CreateStrategy(m_caching_strategy_tid);
    m_caching_strategy->SetHistoryManager(m_history_manager);
    m_caching_strategy->SetCacheManager(m_cache_manager);
    m_caching_strategy->SetNodeId(m_this_node_id);
    m_caching_strategy->EnableProactiveCaching(enableProactiveCaching);
}

void Nuoxus::RegisterRequest(uint32_t contentId, Ipv4Address requester, uint32_t size)
{
    NS_LOG_FUNCTION(this << contentId << requester << size);
    m_history_manager->AddRequest(contentId, requester);
    m_history_manager->AddRequest(contentId, m_this_node_id);        
    bool wasCached = IsContentCached(contentId);

    // Notify the request has been received, and receives back a list of canditates for proactive caching
    score_per_node_t scorePerNode = m_caching_strategy->NotifyRequestReceived(contentId, requester, size);

    // Trace the request
    m_traced_request = NuoxusTracedRequest(m_this_node_id, requester, contentId, wasCached, IsContentCached(contentId));    

    // Now, use the callback to let the server now of possible proactive content
    for(score_per_node_t::iterator scoreIterator = scorePerNode.begin(); scoreIterator != scorePerNode.end(); ++scoreIterator)
    {
        proactiveCallback_t::iterator callbackIterator = m_callbacks.find(scoreIterator->first);
        if(callbackIterator == m_callbacks.end() || callbackIterator->second == 0) continue;
	NS_LOG_INFO("Proactive callback to the children " << callbackIterator->first);
        //NuoxusProactiveCallback * callback = (NuoxusProactiveCallback *) PeekPointer<Application>(callbackIterator->second);
        callbackIterator->second->ProactiveCacheRequest(contentId, scoreIterator->second);
    }
}

void Nuoxus::NotifyReceived(uint32_t contentId, uint32_t packetNumber)
{
    NS_LOG_FUNCTION(this);    
    NuoxusCacheStatus status = m_caching_strategy->GetStatus(contentId);
    if(status == NuoxusCacheStatus::caching)  
    {
        try{
            m_cache_manager->Store(contentId, packetNumber);    
        } catch (std::exception e)
        {
            // Oh well, this should not happen
        }
        
        m_caching_strategy->NotifyWriteInCache(contentId);
    }    
}

bool Nuoxus::IsContentCached(uint32_t contentId)
{
    NS_LOG_FUNCTION(this << contentId);
    NuoxusCacheStatus status = m_caching_strategy->GetStatus(contentId);
    bool isCached = (status == NuoxusCacheStatus::cached || status == NuoxusCacheStatus::caching);
    NS_LOG_INFO("Content is cached: " << isCached );
    return isCached;
}

Ptr<Packet> Nuoxus::GetContent(uint32_t contentId, uint32_t requestedPacket)
{
    NS_LOG_FUNCTION(this << contentId << requestedPacket);
    try
    {
        m_cache_manager->Retrieve(contentId, requestedPacket);
        m_caching_strategy->NotifyReadFromCache(contentId);        
        NuoxusHeader header;
        header.SetContentId(contentId);
        header.SetPacketNumber(requestedPacket);
        header.SetSeq(1);
        Ptr<Packet> packet =  Create<Packet> (m_packet_size);        
        packet->AddHeader(header);
        return packet;
    }
    catch(std::exception e)
    {
	NS_LOG_INFO("Content is no longer cached");
        return 0;
    }
}

Ipv4Address Nuoxus::GetNuoxusId(void)
{
    return m_this_node_id;
}

bool Nuoxus::NotifyProactiveCacheRequest(uint32_t contentId, double_t score, uint32_t size)
{
    // Do not cache an alredy cached content
    if(IsContentCached(contentId)) return false;

    // Let the strategy knows that there is a proactive request
    m_caching_strategy->AddProactiveRequest(contentId, score, size);

    // Return if the content shall be cached
    return IsContentCached(contentId);
}

void Nuoxus::AddProactiveCallback(Ipv4Address nodeId, NuoxusProactiveCallback * callback)
{
    NS_LOG_FUNCTION(this << nodeId << callback);    
    m_callbacks[nodeId] = callback;
}

nuoxus_instance_t NuoxusFactory::m_instances; 

Ptr<Nuoxus> NuoxusFactory::Create(uint32_t packetSize, TypeId cachingStrategy, Ipv4Address nodeId, uint64_t diskSizeKb, bool enableProactiveCaching)
{
    if(m_instances.find(nodeId) != m_instances.end()) return m_instances[nodeId];
    Ptr<Nuoxus> instance = CreateObject<Nuoxus> ();
    instance->Initialize(packetSize, cachingStrategy, nodeId, diskSizeKb, enableProactiveCaching);
    m_instances[nodeId] = instance;
    return instance;
}

} // namespace ns3
