#include "nuoxus-history-manager.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("NuoxusHistoryManager");
NS_OBJECT_ENSURE_REGISTERED (NuoxusHistoryManager);

void NuoxusNodeAccessHistory::AddAccess(uint32_t contentId)
{
    NS_LOG_FUNCTION (this << contentId);
    m_access_history[contentId]++;
}

uint32_t NuoxusNodeAccessHistory::GetNumberOfAccesses(uint32_t contentId)
{
    NS_LOG_FUNCTION (this << contentId);
    node_access_history_t::iterator it = m_access_history.find(contentId);
    if(it != m_access_history.end()){
        return it->second;
    }
    return 0;
}

uint32_t NuoxusNodeAccessHistory::Size()
{
    return m_access_history.size();
}

node_access_history_t NuoxusNodeAccessHistory::GetHistory()
{
    return m_access_history;
}


void NuoxusHistoryManager::AddRequest(uint32_t contentId, Ipv4Address requester)
{
    NS_LOG_FUNCTION (this  << contentId  << requester);
    m_node_history[requester].AddAccess(contentId);
}

listOfAddresses_t NuoxusHistoryManager::GetAllNodes()
{
    listOfAddresses_t allNodes;
    for(node_access_t::iterator it = m_node_history.begin(); it != m_node_history.end(); ++it)
    {
       allNodes.push_back(it->first);
    }
    return allNodes;
}

NuoxusNodeAccessHistory NuoxusHistoryManager::GetNodeHistory(Ipv4Address node)
{
    NS_LOG_FUNCTION (this);
    return m_node_history[node];
}

}
