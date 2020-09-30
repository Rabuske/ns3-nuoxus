/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
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
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 */
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/video-client.h" 
#include "ns3/buffer.h"
#include "nuoxus-header.h"
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/nuoxus-header-proactive.h"
#include <cstdlib>
#include <cstdio>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VideoClient");

NS_OBJECT_ENSURE_REGISTERED (VideoClient);

TypeId
VideoClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VideoClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<VideoClient> ()
    .AddAttribute ("LocalAddress",
                   "Local Address (video consumer)",
                   AddressValue (),
                   MakeAddressAccessor (&VideoClient::m_local_address),
                   MakeAddressChecker ())
    .AddAttribute ("RemoteAddress",
                   "Remote Address (video sender)",
                   AddressValue (),
                   MakeAddressAccessor (&VideoClient::m_remote_address),
                   MakeAddressChecker ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (512),
                   MakeUintegerAccessor (&VideoClient::m_packet_size),
                   MakeUintegerChecker<uint32_t> (12,1024))
    .AddAttribute("ZipfDistribution",
                  "The distribution used to request video files",
                  PointerValue(),
                  MakePointerAccessor (&VideoClient::m_zipf),
                  MakePointerChecker<ZipfRandomVariable> ())
    .AddAttribute ("PacketWindowSize",
                   "The size of the window used to compute the packet loss. This value should be a multiple of 8.",
                   UintegerValue (32),
                   MakeUintegerAccessor (&VideoClient::GetPacketWindowSize,
                                         &VideoClient::SetPacketWindowSize),
                   MakeUintegerChecker<uint16_t> (8,1024))                  
    .AddAttribute("VideoLibrary",
                  "The Video Library",
                  PointerValue(),
                  MakePointerAccessor (&VideoClient::m_video_library),
                  MakePointerChecker<VideoLibrary> ())
    .AddAttribute("Nuoxus",
                  "Nuoxus Instance for this client",
                  PointerValue(),
                  MakePointerAccessor (&VideoClient::m_nuoxus),
                  MakePointerChecker<Nuoxus> ())
    .AddTraceSource("VideoRequest",
                    "A request made by this client",
                    MakeTraceSourceAccessor(&VideoClient::m_traced_request))
  ;
  return tid;
}

VideoClient::VideoClient () : m_packet_loss_counter(0)
{
  NS_LOG_FUNCTION (this);
  m_nextVideoEvent = EventId ();
  m_video_library  = 0;
  m_zipf           = 0;
  m_nuoxus         = 0;  
}

VideoClient::~VideoClient ()
{
  NS_LOG_FUNCTION (this);
}


uint16_t
VideoClient::GetPacketWindowSize () const
{
  NS_LOG_FUNCTION (this);
  return m_packet_loss_counter.GetBitMapSize( );
}

void
VideoClient::SetPacketWindowSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_window_size = size;
  m_packet_loss_counter.SetBitMapSize(size);
}

void
VideoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}


void
VideoClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  // This socket will be used to send video requests
  m_socket_request = Socket::CreateSocket(GetNode(), TypeId::LookupByName ("ns3::TcpSocketFactory"));
  m_socket_request->Bind();
  m_socket_request->Connect(m_remote_address);
  m_socket_request->ShutdownRecv ();
  m_socket_request->SetAllowBroadcast(true);
  m_socket_request->SetConnectCallback (
     MakeCallback (&VideoClient::ConnectionSucceeded, this),
     MakeCallback (&VideoClient::ConnectionFailed, this));
  NS_LOG_INFO("Socket Created to request videos "
               << InetSocketAddress::ConvertFrom(m_remote_address).GetIpv4 ()
               << " port " << InetSocketAddress::ConvertFrom(m_remote_address).GetPort ());  
}

void
VideoClient::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_nextVideoEvent);
}

void VideoClient::Request (uint32_t videoId)
{
  NS_LOG_FUNCTION (this);

  Ptr<VideoData> video = m_video_library->GetVideo(videoId);
  NS_LOG_INFO("Video Requested: " << video->GetId());

  // If the content is cached by nuoxus, do not send the request
  if(m_nuoxus != 0)
  {
    if(m_nuoxus->IsContentCached(videoId))
    {
      // Trace this 
      VideoClientRequestInfo nonTracedRequest(videoId, InetSocketAddress::ConvertFrom(m_local_address).GetIpv4());
      nonTracedRequest.SetEndTime(Simulator::Now());
      m_traced_request = nonTracedRequest;
      return;
    }    
  }
  
  // Create package with the payload
  Ptr<Packet> packet = Create<Packet>(m_packet_size); 
  
  // Add the header
  NuoxusHeader header;
  header.SetContentId(video->GetId());
  header.SetRequestId(VideoClient::GetNextRequestId());
  packet->AddHeader(header);

  // Add for tracing info
  m_non_traced_requests[header.GetRequestId()] = VideoClientRequestInfo(video->GetId(), InetSocketAddress::ConvertFrom(m_remote_address).GetIpv4 ());
  
  // Log - video was requested
  if ((m_socket_request->Send (packet)) >= 0)
  {
    NS_LOG_INFO ("TraceDelay TX " << m_packet_size << " bytes to "
                                  << InetSocketAddress::ConvertFrom(m_remote_address).GetIpv4 () << " Uid: "
                                  << packet->GetUid () << " Time: "
                                  << (Simulator::Now ()).GetSeconds ()
                                  << " Requested Video: " << video->GetId());

  }
  else
  {
    NS_LOG_INFO ("Error while sending " << m_packet_size << " bytes to "
                                        << InetSocketAddress::ConvertFrom(m_remote_address).GetIpv4());
  }
}

void VideoClient::RequestRandomVideo(void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_nextVideoEvent.IsExpired ());

  Ptr<VideoData> video = m_video_library->GetVideo(m_zipf->GetInteger());
  NS_LOG_INFO("Video Requested: " << video->GetId());
  
  // Send the request to the server
  Request(video->GetId());

  // Schedule the next request for when this video has been "watched"
  uint32_t duration = video->GetDurationSeconds();
  if(duration != 0) duration = duration / 2;
  ScheduleNextVideoRequest(Seconds(duration));  

}

void VideoClient::ScheduleNextVideoRequest(Time const & delay)
{
  NS_LOG_FUNCTION (this);
  m_nextVideoEvent = Simulator::Schedule(delay, &VideoClient::RequestRandomVideo, this);  
}


void VideoClient::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
  { 
    // Proactive Request
    if(m_nuoxus != 0)
    {
      NuoxusProactiveHeader proactiveHeader;
      if(packet->RemoveHeader(proactiveHeader) != 0)
      {
        // This means this video should be cached
        if(m_nuoxus->NotifyProactiveCacheRequest(proactiveHeader.GetContentId(), proactiveHeader.GetScore(), m_video_library->GetVideo(proactiveHeader.GetContentId())->GetNumberOfPackets(m_packet_size)))
        {
          Request(proactiveHeader.GetContentId());
        }
        // TODO: Add LOG for this ;)
        return;
      }
    }

    // Receive the content
    if (packet->GetSize () <= 0) continue;      
    NuoxusHeader header;
    if(packet->RemoveHeader (header) == 0 ) return;

    if(header.GetRequestId() == 0 || header.GetContentId() == 0) continue;

    uint32_t videoId = header.GetContentId();
    uint64_t requestId = header.GetRequestId();        
    NS_LOG_INFO ("TraceDelay: RX " << packet->GetSize () <<
                  " bytes from "<< InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
                  " Sequence Number: " << header.GetSeq()  <<
                  " Uid: " << packet->GetUid () <<
                  " TXtime: " << header.GetTs () <<
                  " RXtime: " << Simulator::Now () <<
                  " Delay: " << Simulator::Now () - header.GetTs () <<
                  " Video: " << header.GetContentId() <<
                  " PacketNumber: " << header.GetPacketNumber());
    // When all the packages are received, if the duration of the video has already passed we request a new video now
    m_non_traced_requests[requestId].AddDelay(Simulator::Now () - header.GetTs ());
    if(header.GetPacketNumber() >= m_video_library->GetVideo(videoId)->GetNumberOfPackets(m_packet_size))
    {
      m_non_traced_requests[requestId].SetEndTime(Simulator::Now());
      m_traced_request = m_non_traced_requests[requestId];
    }      
    
    // Let Nuoxus know in case this content is being cached
    if(m_nuoxus != 0)
    {
      m_nuoxus->NotifyReceived(header.GetContentId(), header.GetPacketNumber());
    }
  }   
}

void VideoClient::HandleAccept (Ptr<Socket> socket, const Address& from)
{
  NS_LOG_FUNCTION (this << socket << from);
  m_socket_receive = socket;
  m_socket_receive->SetRecvCallback(MakeCallback (&VideoClient::HandleRead, this));
  ScheduleNextVideoRequest(Seconds (0.0));
}

void VideoClient::RetryConnect()
{
  NS_LOG_FUNCTION (this); 
  m_socket_request->Connect(m_remote_address);
  m_socket_request->SetConnectCallback (
     MakeCallback (&VideoClient::ConnectionSucceeded, this),
     MakeCallback (&VideoClient::ConnectionFailed, this));  
}

void VideoClient::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  // This socket will be used to receive the videos packets
  m_socket_receive = Socket::CreateSocket(GetNode(), TypeId::LookupByName ("ns3::TcpSocketFactory"));
  m_socket_receive->Bind(InetSocketAddress(Ipv4Address::GetAny (), InetSocketAddress::ConvertFrom(m_local_address).GetPort ()));
  m_socket_receive->Listen();
  m_socket_receive->ShutdownSend ();
  m_socket_receive->SetAcceptCallback (MakeNullCallback<bool, Ptr<Socket>,const Address &> (),MakeCallback(&VideoClient::HandleAccept, this));
  NS_LOG_INFO("Socket Created to receive videos "
               << InetSocketAddress::ConvertFrom(m_local_address).GetIpv4 ()
               << " port " << InetSocketAddress::ConvertFrom(m_local_address).GetPort ());

}

void VideoClient::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Simulator::Schedule (Seconds(2), &VideoClient::RetryConnect, this);
  
}
uint64_t VideoClient::g_requestId = 0;

/* ----------------- Traced Request -----------------*/
VideoClientRequestInfo::VideoClientRequestInfo() {}

VideoClientRequestInfo::VideoClientRequestInfo(uint32_t videoId, Ipv4Address requester)
{
  m_start_time = Simulator::Now();
  m_video_id   = videoId;
  m_requester  = requester;
}

void VideoClientRequestInfo::SetEndTime(Time time)
{
  m_end_time = time;
}

void VideoClientRequestInfo::AddDelay(Time delay)
{
  m_total_delay = m_total_delay + delay;
  m_number_of_packets++;
}

Time VideoClientRequestInfo::GetStartTime(void)
{
  return m_start_time;
}

Time VideoClientRequestInfo::GetEndTime(void)
{
  return m_end_time;
}

Time VideoClientRequestInfo::GetAverageDelay(void)
{
  if(m_number_of_packets == 0) m_number_of_packets = 1;
  return m_total_delay / (uint64_t) m_number_of_packets;
}

uint32_t VideoClientRequestInfo::GetVideoId(void)
{
  return m_video_id;
}

Ipv4Address VideoClientRequestInfo::GetRequester(void)
{
  return m_requester;
}

bool VideoClientRequestInfo::operator==(const VideoClientRequestInfo& comp)
{
 
  return (this->m_start_time         == comp.m_start_time &&  
          this->m_end_time           == comp.m_end_time &&         
          this->m_total_delay        == comp.m_total_delay && 
          this->m_number_of_packets  == comp.m_number_of_packets &&
          this->m_video_id           == comp.m_video_id &&
          this->m_requester          == comp.m_requester
          );   
}

bool VideoClientRequestInfo::operator!=(const VideoClientRequestInfo& comp)
{
  return !(*this == comp);
}

} // Namespace ns3
