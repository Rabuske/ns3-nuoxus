#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "nuoxus-header-proactive.h"

namespace ns3{

NS_LOG_COMPONENT_DEFINE ("NuoxusProactiveHeader");

NS_OBJECT_ENSURE_REGISTERED (NuoxusProactiveHeader);

NuoxusProactiveHeader::NuoxusProactiveHeader ()
  : m_ts (Simulator::Now ().GetTimeStep ()),
    m_content_id (0),
    m_score(0)
{
  NS_LOG_FUNCTION (this);
}

Time NuoxusProactiveHeader::GetTs (void) const
{
  NS_LOG_FUNCTION (this);
  return TimeStep (m_ts);
}

TypeId NuoxusProactiveHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NuoxusProactiveHeader")
    .SetParent<Header> ()
    .SetGroupName("Applications")
    .AddConstructor<NuoxusProactiveHeader> ()
  ;
  return tid;
}

TypeId NuoxusProactiveHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void NuoxusProactiveHeader::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "(time=" << TimeStep (m_ts).GetSeconds () << " content=" << m_content_id << " score=" << m_score << ")";
}

uint32_t NuoxusProactiveHeader::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint64_t);
}

void
NuoxusProactiveHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtonU64 (m_ts);
  i.WriteHtonU32 (m_content_id);

  // Score needs to be converted from double_t to uint64_t, so it can be written on buffer
  uint64_t intscore;
  memcpy((void*)&intscore,(void*)&m_score,sizeof(m_score));
  std::stringstream stream;
  stream.precision(std::numeric_limits<double_t>::digits);
  stream << intscore;
  intscore = stoull(stream.str());
  i.WriteHtonU64 (intscore);  
}

uint32_t NuoxusProactiveHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_ts = i.ReadNtohU64 ();
  m_content_id = i.ReadNtohU32 ();
  uint64_t intscore = i.ReadNtohU64 ();

  // Convert serialized score from uint64_t back to double_t
  memcpy((void*)&m_score,(void*)&intscore,sizeof(m_score));
  return GetSerializedSize ();
}

void NuoxusProactiveHeader::SetContentId(uint32_t contentId) 
{ 
  NS_LOG_FUNCTION (this << contentId);  
  m_content_id = contentId;
}

void NuoxusProactiveHeader::SetScore(double_t score) 
{ 
  NS_LOG_FUNCTION (this << score);  
  m_score = score;
}

uint32_t NuoxusProactiveHeader::GetContentId(void) 
{ 
  NS_LOG_FUNCTION (this);  
  return m_content_id;
}

double_t NuoxusProactiveHeader::GetScore(void) 
{ 
  NS_LOG_FUNCTION (this);  
  return m_score;
}

} // namespace ns3
