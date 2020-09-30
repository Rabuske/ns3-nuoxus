/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "nuoxus-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NuoxusHeader");

NS_OBJECT_ENSURE_REGISTERED (NuoxusHeader);

NuoxusHeader::NuoxusHeader ()
  : m_seq (0), 
    m_ts (Simulator::Now ().GetTimeStep ()),
    m_content_id (0),
    m_packet_number (0),
    m_request_id(0)
{
  NS_LOG_FUNCTION (this);
}

void
NuoxusHeader::SetSeq (uint32_t seq)
{
  NS_LOG_FUNCTION (this << seq);
  m_seq = seq;
}
uint32_t
NuoxusHeader::GetSeq (void) const
{
  NS_LOG_FUNCTION (this);
  return m_seq;
}

Time
NuoxusHeader::GetTs (void) const
{
  NS_LOG_FUNCTION (this);
  return TimeStep (m_ts);
}

TypeId
NuoxusHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NuoxusHeader")
    .SetParent<Header> ()
    .SetGroupName("Applications")
    .AddConstructor<NuoxusHeader> ()
  ;
  return tid;
}
TypeId
NuoxusHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
NuoxusHeader::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "(seq=" << m_seq << " time=" << TimeStep (m_ts).GetSeconds () << " content=" << m_content_id << " packet=" << m_packet_number << " request=" << m_request_id << ")";
}
uint32_t
NuoxusHeader::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return sizeof(uint32_t) * 2 + sizeof(uint64_t) *2;
}

void
NuoxusHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtonU64 (m_ts);
  i.WriteHtonU32 (m_content_id);
  i.WriteHtonU32 (m_packet_number);
  i.WriteHtonU64 (m_request_id);
}
uint32_t
NuoxusHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_ts = i.ReadNtohU64 ();
  m_content_id = i.ReadNtohU32 ();
  m_packet_number = i.ReadNtohU32 ();
  m_request_id = i.ReadNtohU64 ();
  return GetSerializedSize ();
}

void NuoxusHeader::SetContentId(uint32_t contentId) 
{ 
  NS_LOG_FUNCTION (this << contentId);  
  m_content_id = contentId;
}

void NuoxusHeader::SetPacketNumber(uint32_t number) 
{ 
  NS_LOG_FUNCTION (this << number);  
  m_packet_number = number;
}

void NuoxusHeader::SetRequestId(uint64_t requestId) 
{ 
  NS_LOG_FUNCTION (this << requestId);  
  m_request_id = requestId;
}

uint32_t NuoxusHeader::GetContentId(void) 
{ 
  NS_LOG_FUNCTION (this);  
  return m_content_id;
}

uint32_t NuoxusHeader::GetPacketNumber(void) 
{ 
  NS_LOG_FUNCTION (this);  
  return m_packet_number;
}

uint64_t NuoxusHeader::GetRequestId(void)
{
  return m_request_id;
}

} // namespace ns3
