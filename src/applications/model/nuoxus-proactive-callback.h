#ifndef NUOXUS_PROACTIVE_CALLBACK_H
#define NUOXUS_PROACTIVE_CALLBACK_H

#include "ns3/uinteger.h"
#include "ns3/object.h"
#include "ns3/application.h"
#include "ns3/ipv4-address.h"
#include <map>

namespace ns3 {

class NuoxusProactiveCallback
{
public:
    NuoxusProactiveCallback();
    virtual ~NuoxusProactiveCallback();
    virtual void ProactiveCacheRequest(uint32_t contentId, double_t score) = 0;
};

} // namespace ns3

#endif // NUOXUS_PROACTIVE_CALLBACK_H
