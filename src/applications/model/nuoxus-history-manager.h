#ifndef NUOXUS_HISTORY_MANAGER_H
#define NUOXUS_HISTORY_MANAGER_H

#include "ns3/uinteger.h"
#include "ns3/object.h"
#include "ns3/simple-ref-count.h"
#include "ns3/ipv4-address.h"
#include "ns3/ptr.h"
#include <map>
#include <list>

namespace ns3
{

typedef std::list<Ipv4Address> listOfAddresses_t;
typedef std::map<uint32_t, uint32_t> node_access_history_t;    // contentId, NumberOfAccess

class NuoxusNodeAccessHistory
{
public:
    NuoxusNodeAccessHistory(){};
    ~NuoxusNodeAccessHistory(){};
    void AddAccess(uint32_t contentId);
    uint32_t GetNumberOfAccesses(uint32_t contentId);
    node_access_history_t GetHistory();
    uint32_t Size();

protected:    
    node_access_history_t m_access_history;
};

typedef std::map<Ipv4Address, NuoxusNodeAccessHistory> node_access_t; //

class NuoxusHistoryManager : public SimpleRefCount<NuoxusHistoryManager, Object>
{
public:
    
  /**
   * \brief Add a request to the history
   * \param contentId the unique identifier for a content
   * \param requester the Ipv4 address of the requester
   */    
    NuoxusHistoryManager(){};
    ~NuoxusHistoryManager(){};
    void AddRequest(uint32_t contentId, Ipv4Address requester);
    NuoxusNodeAccessHistory GetNodeHistory(Ipv4Address node);
    listOfAddresses_t GetAllNodes();

private:
    node_access_t m_node_history;        //!< The map containing the history
};

}
#endif
