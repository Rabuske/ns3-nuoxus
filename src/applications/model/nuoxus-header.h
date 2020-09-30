#ifndef NUOXUS_HEADER_H
#define NUOXUS_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"

namespace ns3 {
/**
 * \ingroup udpclientserver
 *
 * \brief Packet header for UDP client/server application.
 *
 * The header is made of a 32bits sequence number followed by
 * a 64bits time stamp.
 */
class NuoxusHeader : public Header
{
public:
  NuoxusHeader ();

  /**
   * \param seq the sequence number
   */
  void SetSeq (uint32_t seq);
  /**
   * \return the sequence number
   */
  uint32_t GetSeq (void) const;
  /**
   * \return the time stamp
   */
  Time GetTs (void) const;
  
  void SetContentId(uint32_t contentId);
  void SetPacketNumber(uint32_t number);
  void SetRequestId(uint64_t requestId);

  uint32_t GetContentId(void);
  uint32_t GetPacketNumber(void);
  uint64_t GetRequestId(void);

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
  uint32_t m_seq; //!< Sequence number
  uint64_t m_ts; //!< Timestamp
  uint32_t m_content_id; //!< Content ID
  uint32_t m_packet_number; //!< Packet Number
  uint64_t m_request_id;
};

} // namespace ns3

#endif /* NUOXUS_HEADER_H */
