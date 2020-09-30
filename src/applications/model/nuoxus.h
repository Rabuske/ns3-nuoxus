#ifndef NUOXUS_H
#define NUOXUS_H

#include "ns3/nuoxus-cache-strategy.h"
#include "ns3/uinteger.h"
#include "ns3/object.h"
#include "ns3/simple-ref-count.h"
#include "ns3/ipv4-address.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/object-factory.h"
#include "ns3/nuoxus-traced-values.h"
#include "ns3/nuoxus-proactive-callback.h"
#include "ns3/application.h"
#include <exception>
#include <map>

namespace ns3 {

typedef std::map<Ipv4Address, NuoxusProactiveCallback*> proactiveCallback_t;

class Nuoxus : public SimpleRefCount<Nuoxus, Object>
{
public:
    static TypeId GetTypeId (void);
    Nuoxus();
    virtual ~Nuoxus();

  /**
   * \brief Register the request of a content from a device
   * \param contentId the unique identifier for a content
   * \param requester the Ipv4 address of the requester
   * \parem size of the requested content
   */            
    void RegisterRequest(uint32_t contentId, Ipv4Address requester, uint32_t size);
    
  /**
   * \brief Add a request to the history
   * \param contentId the unique identifier for a content
   * \param packetNumber number of the requested packet
   */        
    void NotifyReceived(uint32_t contentId, uint32_t packetNumber);
    
  /**
   * \brief Add a request to the history
   * \param contentId the unique identifier for a content
   */        
    bool IsContentCached(uint32_t contentId);

  /**
   * \brief Add a request to the history
   * \param contentId the unique identifier for a content
   * \param requestedPacket the number of the requested packet
   */        
    Ptr<Packet> GetContent(uint32_t contentId, uint32_t requestedPacket);

    void AddProactiveCallback(Ipv4Address nodeId, NuoxusProactiveCallback* callback);
    bool NotifyProactiveCacheRequest(uint32_t contentId, double_t score, uint32_t size);

    Ipv4Address GetNuoxusId(void);

    void Initialize(uint32_t packetSize, TypeId cachingStrategy, Ipv4Address nodeId, uint64_t diskSizeKb, bool enableProactiveCaching);
    
    Ptr<NuoxusCachingStrategy> GetCachingStrategy (void) 
    {
      return m_caching_strategy;
    }

private:
    Ptr<NuoxusCachingStrategy> m_caching_strategy; 
    Ptr<NuoxusCacheManager>   m_cache_manager;            //!< Manages the content stored on disk
    Ptr<NuoxusHistoryManager> m_history_manager;          //!< Manages the access history
    uint32_t m_packet_size;                               //!< Size of the packets handled by this instance of Nuoxus
    Ipv4Address m_this_node_id;                               //!< Identification of the node running this instance of Nuoxus
    TypeId m_caching_strategy_tid;
    TracedValue<NuoxusTracedRequest> m_traced_request;
    proactiveCallback_t m_callbacks;
};

typedef std::map<Ipv4Address, Ptr<Nuoxus>> nuoxus_instance_t;

class NuoxusFactory{
public:
  static Ptr<Nuoxus> Create(uint32_t packetSize, TypeId cachingStrategy, Ipv4Address nodeId, uint64_t diskSizeKb, bool enableProactiveCaching);
private:
  static nuoxus_instance_t m_instances;	

};

} // namespace ns3

#endif // NUOXUS_H
