#include "nuoxus-cache-strategy.h"
#include "ns3/log.h"
#include <algorithm>    // std::find

#include <iostream>
#include <fstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("NuoxusCachingStrategy");
NS_OBJECT_ENSURE_REGISTERED (NuoxusCachingStrategy);


void NuoxusCachingStrategy::SetCacheManager(Ptr<NuoxusCacheManager> cacheManager)
{
    NS_LOG_FUNCTION (this);
    m_cache_manager = cacheManager;
}

void NuoxusCachingStrategy::SetHistoryManager(Ptr<NuoxusHistoryManager> historyManager)
{
    NS_LOG_FUNCTION (this);
    m_history_manager = historyManager;
}

void NuoxusCachingStrategy::SetNodeId(Ipv4Address nodeId)
{
    NS_LOG_FUNCTION (this);
    m_this_node_id = nodeId;
}
    
void NuoxusCachingStrategy::NotifyWriteInCache(uint32_t contentId)
{
    NS_LOG_FUNCTION (this);
    uint32_t totalPackets = m_total_packets[contentId];
    if(totalPackets == 0) return;
    try{
        // Try to read the last block
        m_cache_manager->Retrieve(contentId, totalPackets);
        // If it does work, then set the status to cached instead of caching
        m_status[contentId] = NuoxusCacheStatus::cached;
    } catch(std::exception e)
    {
        return;
    }
}

void NuoxusCachingStrategy::EnableProactiveCaching(bool proactiveCaching)
{
    NS_LOG_FUNCTION(this << proactiveCaching);
    m_with_proactive_caching = proactiveCaching;
}

void NuoxusCachingStrategy::NotifyReadFromCache(uint32_t contentId)
{
    NS_LOG_FUNCTION (this);
    return;
}

NuoxusCacheStatus NuoxusCachingStrategy::GetStatus(uint32_t contentId)
{
    NS_LOG_FUNCTION (this);
    status_t::iterator statusIt = m_status.find(contentId);
    if(statusIt == m_status.end()) return NuoxusCacheStatus::notCached;
    return statusIt->second;
}

uint32_t NuoxusCachingStrategy::GetContentWithLowerScoreIgnoring(content_list_t contentToIgnore)
{
    NS_LOG_FUNCTION (this);
    uint32_t lowerScoreContentId = UINT32_MAX;
    for(score_t::iterator it = m_score.begin(); it != m_score.end(); ++it)
    {
        // Ignore not cached content
        if(GetStatus(it->first) == NuoxusCacheStatus::notCached) continue;
    
        // Ignore content that should be ignored
        content_list_t::iterator content_it = std::find(contentToIgnore.begin(), contentToIgnore.end(), it->first);
        if(content_it != contentToIgnore.end()) continue;
        

        if(lowerScoreContentId == UINT32_MAX || m_score[lowerScoreContentId] > it->second)
        {
            lowerScoreContentId = it->first;
        }
    }
    return lowerScoreContentId;
}

score_per_node_t NuoxusCachingStrategy::NotifyRequestReceived(uint32_t contentId, Ipv4Address requester, uint32_t totalPackets)
{
    NS_LOG_FUNCTION (this);
    score_per_node_t scorePerChild;
    m_total_packets[contentId] = totalPackets;
    CalculateScore(contentId, requester);

    // If content is already cached, do nothing
    NuoxusCacheStatus status = GetStatus(contentId);
    if(status != NuoxusCacheStatus::notCached) return scorePerChild;

    // Try to cache 
    AttemptCache(contentId, totalPackets);

    // If the content has been cached, attempt now to distribute this content to its children
    status = GetStatus(contentId);
    if(status == NuoxusCacheStatus::notCached) return scorePerChild;

    // Proactive Caching
    if(m_with_proactive_caching)
    {
        listOfAddresses_t allChilds = m_history_manager->GetAllNodes();
        for(listOfAddresses_t::iterator it = allChilds.begin(); it != allChilds.end(); ++it)
        {
            if(*it == m_this_node_id || *it == requester) continue;
            // TODO: Encapsulate this bit
            NuoxusNodeAccessHistory thisNodeHistory       = m_history_manager->GetNodeHistory(m_this_node_id);            
            NuoxusNodeAccessHistory nodeCandidateHistory  = m_history_manager->GetNodeHistory(*it);
            NuoxusNodeAccessHistory requesterHistory      = m_history_manager->GetNodeHistory(requester);
            double_t similarity = NuoxusCousineCalculator().CousineBetweenNodes(
                nodeCandidateHistory,
                requesterHistory
            );
        
            double_t sizeRatio = (double_t) requesterHistory.Size() / (double_t) nodeCandidateHistory.Size();
            double_t totalAccessesToContent = (double_t)thisNodeHistory.GetNumberOfAccesses(contentId);
        
            scorePerChild[*it] = similarity * sizeRatio * totalAccessesToContent;
        }
    }
    return scorePerChild;
}

void NuoxusCachingStrategy::AttemptCache(uint32_t contentId, uint32_t totalPackets)
{
    NS_LOG_FUNCTION (this << contentId << totalPackets);    
    // Is there enough space in disk for this content?
    uint32_t availableSpaceInDisk = m_cache_manager->GetAvailableSpace();
    uint32_t blockSize = m_cache_manager->GetBlockSize();
    uint64_t requiredSpace = (uint64_t)totalPackets * (uint64_t)blockSize;
    int64_t availableSpaceInDiskAfterAddingFile = availableSpaceInDisk - requiredSpace;
    
    NS_LOG_INFO("Space Available: " << availableSpaceInDisk << " After Adding the file: " << availableSpaceInDiskAfterAddingFile << " whose packet quantity is " << totalPackets);
    if(availableSpaceInDiskAfterAddingFile >= 0)
    {
        // In case there is enough space, just set the status to caching, so the content is stored by the Nuoxus Application
        NS_LOG_INFO("Space was enough, so the content has been cached ");        
        m_status[contentId] = NuoxusCacheStatus::caching;
        m_cache_manager->ReserveSpace(requiredSpace);
        return;
    }

    // Gets the contents, by order of score, that would need to be removed for this content to fit in disk
    double_t scoreCandidate = m_score[contentId];
    content_list_t contentIdsToBeRemoved;
    
    while(availableSpaceInDiskAfterAddingFile < 0)
    {        
        uint32_t nextCachedContentWithLowerScore = GetContentWithLowerScoreIgnoring(contentIdsToBeRemoved);
        NS_LOG_INFO("Content Id: " << contentId << " Next content with lower Score " << nextCachedContentWithLowerScore << " score=" << m_score[nextCachedContentWithLowerScore]  << " this score=" << scoreCandidate);        
        if(nextCachedContentWithLowerScore == UINT32_MAX) 
        {
            NS_LOG_INFO("Content Id: " << contentId << " Will not be stored in cache - this file is bigger than the disk");        
            return;            
        }
        
        // Only continues if the content that would need to be removed has a lower score than the candidate
        if(m_score[nextCachedContentWithLowerScore] > scoreCandidate)
        {
            NS_LOG_INFO("Content Id: " << contentId << " Will not be stored in cache - the score of this content is lower than necessary" );                    
            return;
        } 

        contentIdsToBeRemoved.push_back(nextCachedContentWithLowerScore);
        availableSpaceInDiskAfterAddingFile += m_total_packets[nextCachedContentWithLowerScore] * blockSize;
        NS_LOG_INFO("Content Id: " << contentId << " Even after removing the file, the space missing is " << availableSpaceInDiskAfterAddingFile);            
    }           

    NS_LOG_INFO("Content Id: " << contentId << " Will be stored in cache" );            
    
    // Trace this change
    score_t replacing;
    score_t replaced;
    replacing[contentId] = scoreCandidate;
    NuoxusContentReplacementTrace replacementTrace = NuoxusContentReplacementTrace(replacing);

    // Remove the content needed
    for(content_list_t::iterator it = contentIdsToBeRemoved.begin(); it != contentIdsToBeRemoved.end(); ++it)
    {
        replaced[*it] = m_score[*it];
        m_status.erase(*it);
        m_cache_manager->Delete(*it, (uint64_t)((uint64_t)m_total_packets[*it] * (uint64_t)blockSize));
    }

    // Content is now being cached (replacing old content)
    m_status[contentId] = NuoxusCacheStatus::caching;    

    // Reserve The space
    m_cache_manager->ReserveSpace(requiredSpace);
    
    // Finishes the trace
    replacementTrace.SetReplaced(replaced);
    m_replacement_trace = replacementTrace; // Trigger the trace

    // Should be reviewed to use the tracing system
    std::ofstream myfile;
    std::stringstream fileName; 
    fileName << "nuxous-file-replacement" << m_this_node_id << ".txt";
    myfile.open (fileName.str(), std::ios::app);
    
    myfile << replacementTrace.GetTimeStamp().GetSeconds() << "\t";
    score_t replacingContent = replacementTrace.GetReplacing();
    score_t replacedContent  = replacementTrace.GetReplaced();

    for(score_t::iterator it = replacingContent.begin(); it !=  replacingContent.end(); ++it)
    {
        myfile << it->first << "\t" << it->second << "\t";
    }

    for(score_t::iterator it = replacedContent.begin(); it !=  replacedContent.end(); ++it)
    {
        myfile << it->first << "\t" << it->second << "\t";
    }
    myfile << std::endl;
    myfile.close();    
}

void NuoxusCachingStrategy::AddProactiveRequest(uint32_t contentId, double_t score, uint32_t totalPackets)
{
    NS_LOG_FUNCTION (this << contentId << score << totalPackets);
    m_score[contentId] = score;
    m_total_packets[contentId] = totalPackets;

    // Try to cache 
    AttemptCache(contentId, totalPackets);    
}

void NuoxusCachingStrategyDefault::CalculateScore(uint32_t contentId, Ipv4Address requester)
{       
    NS_LOG_FUNCTION (this << contentId << requester);
    NuoxusNodeAccessHistory thisNodeHistory  = m_history_manager->GetNodeHistory(m_this_node_id);
    NuoxusNodeAccessHistory requesterHistory = m_history_manager->GetNodeHistory(requester);
    double_t similarity = NuoxusCousineCalculator().CousineBetweenNodes(
        thisNodeHistory,
        requesterHistory
    );

    double_t sizeRatio = (double_t) requesterHistory.Size() / (double_t) thisNodeHistory.Size();
    double_t totalAccessesToContent = (double_t)thisNodeHistory.GetNumberOfAccesses(contentId);

    double_t score = similarity * sizeRatio * totalAccessesToContent;

    // Do not decrease a higher score
    if( m_score[contentId] < score )
    {
        m_score[contentId] = score;
    }
}

TypeId NuoxusCachingStrategyDefault::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::NuoxusCachingStrategyDefault")
    .AddConstructor<NuoxusCachingStrategyDefault> ()    
    .SetParent<NuoxusCachingStrategy>()
    .SetGroupName("Network")
    .AddTraceSource("FileReplacement",
                    "Files Replaced",
                    MakeTraceSourceAccessor(&NuoxusCachingStrategy::m_replacement_trace))    
    ;    
    return tid;
}

void NuoxusCachingStrategyLFU::CalculateScore(uint32_t contentId, Ipv4Address requester)
{
    NS_LOG_FUNCTION (this);
    double_t score = m_score[contentId];
    m_score[contentId] = score + 1; // Most accessed content has a higher score ;)
}

TypeId NuoxusCachingStrategyLFU::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::NuoxusCachingStrategyLFU")
    .AddConstructor<NuoxusCachingStrategyLFU> ()
    .SetParent<NuoxusCachingStrategy>()
    .SetGroupName("Network")
    .AddTraceSource("FileReplacement",
                    "Files Replaced",
                     MakeTraceSourceAccessor(&NuoxusCachingStrategy::m_replacement_trace))        
    ;    
    return tid;
}

void NuoxusCachingStrategyLRU::CalculateScore(uint32_t contentId, Ipv4Address requester)
{
    NS_LOG_FUNCTION (this);
    m_score[contentId] = UINT32_MAX; // Last access content has a higher score ;)
    for(score_t::iterator it = m_score.begin(); it != m_score.end(); ++it)
        it->second--;
}

TypeId NuoxusCachingStrategyLRU::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::NuoxusCachingStrategyLRU")
    .AddConstructor<NuoxusCachingStrategyLRU> ()
    .SetParent<NuoxusCachingStrategy>()
    .SetGroupName("Network")
    .AddTraceSource("FileReplacement",
                    "Files Replaced",
                    MakeTraceSourceAccessor(&NuoxusCachingStrategy::m_replacement_trace))        
    ;    
    return tid;
}

void NuoxusCachingStrategyNoCache::CalculateScore(uint32_t contentId, Ipv4Address requester)
{
    NS_LOG_FUNCTION (this);    
    m_score[contentId] = 0; // Last access content has a higher score ;)
}

NuoxusCacheStatus NuoxusCachingStrategyNoCache::GetStatus(uint32_t contentId)
{
    NS_LOG_FUNCTION (this);
    return NuoxusCacheStatus::notCached;
}

TypeId NuoxusCachingStrategyNoCache::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::NuoxusCachingStrategyNoCache")
    .AddConstructor<NuoxusCachingStrategyNoCache> ()
    .SetParent<NuoxusCachingStrategy>()
    .SetGroupName("Network")
    .AddTraceSource("FileReplacement",
                    "Files Replaced",
                    MakeTraceSourceAccessor(&NuoxusCachingStrategy::m_replacement_trace))        
    ;    
    return tid;
}


Ptr<NuoxusCachingStrategy> NuoxusCachingStrategyFactory::CreateStrategy(TypeId tid)
{
    if(tid == NuoxusCachingStrategyLFU::GetTypeId())
    {
        return CreateObject<NuoxusCachingStrategyLFU>();        
    }

    if(tid == NuoxusCachingStrategyLRU::GetTypeId())
    {
        return CreateObject<NuoxusCachingStrategyLRU>();                
    }

    if(tid == NuoxusCachingStrategyNoCache::GetTypeId())
    {
        return CreateObject<NuoxusCachingStrategyNoCache>();               
    }

    return CreateObject<NuoxusCachingStrategyDefault>();                
}

}
