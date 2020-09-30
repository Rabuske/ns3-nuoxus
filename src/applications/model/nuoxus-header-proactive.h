#ifndef NUOXUS_HEADER_PROACTIVE_H
#define NUOXUS_HEADER_PROACTIVE_H

#include "ns3/header.h"
#include "ns3/nstime.h"

namespace ns3 {

class NuoxusProactiveHeader : public Header
{
public:
  NuoxusProactiveHeader ();

  Time GetTs (void) const;  
  void SetContentId(uint32_t contentId);
  void SetScore(double_t score);

  uint32_t GetContentId(void);
  double_t GetScore(void);

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

private:
  uint64_t m_ts; //!< Timestamp
  uint32_t m_content_id; //!< Content ID
  double_t m_score; //!< Score
};

} // namespace ns3

#endif /* NUOXUS_HEADER_PROACTIVE_H */
