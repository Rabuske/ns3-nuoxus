#ifndef NUOXUS_CACHE_STRATEGY_H
#define NUOXUS_CACHE_STRATEGY_H

#include "ns3/uinteger.h"
#include "ns3/object.h"
#include "ns3/simple-ref-count.h"
#include "ns3/ipv4-address.h"
#include "ns3/ptr.h"
#include "ns3/nuoxus-cache-manager.h"
#include "ns3/nuoxus-cousine-calculator.h"
#include "ns3/nuoxus-history-manager.h"
#include "ns3/traced-value.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"
#include <exception>
#include <map>
#include <list>

namespace ns3
{

enum NuoxusCacheStatus
{
    notCached, 
    caching, 
    cached
};

typedef std::map<uint32_t, double_t> score_t; 
typedef std::map<uint32_t, NuoxusCacheStatus> status_t; 
typedef std::map<uint32_t, uint32_t> total_packets_t;
typedef std::list<uint32_t> content_list_t;
typedef std::map<Ipv4Address, double_t> score_per_node_t;

class NuoxusContentReplacementTrace
{
public:
    NuoxusContentReplacementTrace()
    {
        m_time_stamp = Simulator::Now();        
    }

    NuoxusContentReplacementTrace(score_t content)
    {
        m_time_stamp = Simulator::Now();    
        m_replacing  = content;    
    }

    void SetReplaced(score_t content)
    {
        m_replaced = content;
    }

    score_t GetReplaced(void) 
    {
        return m_replaced;
    }

    Time GetTimeStamp(void)
    {
        return m_time_stamp;
    }

    score_t GetReplacing(void)
    {
        return m_replacing;
    }

    bool operator==(const NuoxusContentReplacementTrace& comp)
    {
        return (this->m_time_stamp == comp.m_time_stamp &&
                this->m_replacing.begin()->second == comp.m_replacing.begin()->second);
    }

    bool operator!=(const NuoxusContentReplacementTrace& comp)
    {
        return(*this == comp);
    }


private:
    Time m_time_stamp;
    score_t  m_replaced;
    score_t  m_replacing;
};

class NuoxusCachingStrategy : public SimpleRefCount<NuoxusCachingStrategy, Object>
{
public:
    NuoxusCachingStrategy(){};
    virtual ~NuoxusCachingStrategy(){};
   
    void EnableProactiveCaching(bool proactiveCaching);

  /**
   * \brief Set the Cache Manager
   * \param cacheManager a pointer to the cache manager
   */                
    void SetCacheManager(Ptr<NuoxusCacheManager> cacheManager);

  /**
   * \brief Set the History Manager
   * \param historyManager a pointer to the cache manager
   */                
   void SetHistoryManager(Ptr<NuoxusHistoryManager> historyManager);

  /**
   * \brief Set the History Manager
   * \param historyManager a pointer to the cache manager
   */                
   void SetNodeId(Ipv4Address nodeId);   

   /**
   * \brief Notifies the strategy that some content has been stored into cache
   * \param contentId a unique identifier for a content
   */    
   void NotifyWriteInCache(uint32_t contentId);

   /**
   * \brief Notifies the strategy that some content has been retrieved from cache
   * \param contentId a unique identifier for a content
   */    
   void NotifyReadFromCache(uint32_t contentId);

   /**
   * \brief Updates the caching given
   * \param contentId a unique identifier for a content
   * \param requester the ID of the requester node
   * \param totalPackets the total number of the packets
   */    
   score_per_node_t NotifyRequestReceived(uint32_t contentId, Ipv4Address requester, uint32_t totalPackets);   

   /**
   * \brief Get the status of the given content
   * \param contentId a unique identifier for a content
   */       
   NuoxusCacheStatus GetStatus(uint32_t contentId);

   /**
   * \brief Return the content with lower score ignoring the list passed as parameter 
   * \param contentToIgnore 
   */       
   uint32_t GetContentWithLowerScoreIgnoring(content_list_t contentToIgnore);

   void AddProactiveRequest(uint32_t contentId, double_t score, uint32_t totalPackets);
   void AttemptCache(uint32_t contentId, uint32_t totalPackets);

   TracedValue<NuoxusContentReplacementTrace> m_replacement_trace;
   
protected:
    virtual void CalculateScore(uint32_t contentId, Ipv4Address requester) = 0;


    Ptr<NuoxusCacheManager>   m_cache_manager;       //!< Store and retrieve data from disk
    Ptr<NuoxusHistoryManager> m_history_manager;     //!< Manages the access history
    Ipv4Address m_this_node_id;                      //!< This node Id 
    score_t  m_score;                                //!< The scores of the contents
    status_t m_status;                               //!< The status of the content
    total_packets_t m_total_packets;                 //!< Auxiliary that maintains the size of the content
    bool m_with_proactive_caching;                   //!< Support proactive caching?
};

class NuoxusCachingStrategyDefault : public NuoxusCachingStrategy
{
public:
    NuoxusCachingStrategyDefault(){};
    ~NuoxusCachingStrategyDefault(){}
    static TypeId GetTypeId (void);
protected:
    void CalculateScore(uint32_t contentId, Ipv4Address requester);
};

class NuoxusCachingStrategyLFU : public NuoxusCachingStrategy
{
public:
    NuoxusCachingStrategyLFU(){};
    ~NuoxusCachingStrategyLFU(){};
    static TypeId GetTypeId (void);
protected:
    void CalculateScore(uint32_t contentId, Ipv4Address requester);
};

class NuoxusCachingStrategyLRU : public NuoxusCachingStrategy
{
public:
    NuoxusCachingStrategyLRU(){};
    ~NuoxusCachingStrategyLRU(){};
    static TypeId GetTypeId (void);
protected:
    void CalculateScore(uint32_t contentId, Ipv4Address requester);
};

class NuoxusCachingStrategyNoCache : public NuoxusCachingStrategy
{
public:
    NuoxusCachingStrategyNoCache(){};
    ~NuoxusCachingStrategyNoCache(){};
    static TypeId GetTypeId (void);
    NuoxusCacheStatus GetStatus(uint32_t contentId);
protected:
    void CalculateScore(uint32_t contentId, Ipv4Address requester);

};

class NuoxusCachingStrategyFactory
{
public:
    NuoxusCachingStrategyFactory(){};
    ~NuoxusCachingStrategyFactory(){};
    static Ptr<NuoxusCachingStrategy> CreateStrategy(TypeId tid);    
};

}

#endif
