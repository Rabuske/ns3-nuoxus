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
#include "packet-loss-counter.h"

#include "nuoxus-header.h"
#include "video-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VideoServer");

NS_OBJECT_ENSURE_REGISTERED (VideoServer);


TypeId
VideoServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VideoServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<VideoServer> ()
    .AddAttribute ("LocalAddress",
                   "The address of the local application (video sender)",
                   AddressValue (),
                   MakeAddressAccessor (&VideoServer::m_local_address),
                   MakeAddressChecker ())
    .AddAttribute ("RemoteAddress",
                   "The address of the remote application (video receiver)",
                   AddressValue (),
                   MakeAddressAccessor (&VideoServer::m_remote_address),
                   MakeAddressChecker ())
    .AddAttribute ("DataRate", "The data rate",
                   DataRateValue (DataRate ("5000kb/s")),
                   MakeDataRateAccessor (&VideoServer::m_data_rate),
                   MakeDataRateChecker ())                   
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (512),
                   MakeUintegerAccessor (&VideoServer::m_packet_size),
                   MakeUintegerChecker<uint32_t> (12,1500))
    .AddAttribute("VideoLibrary",
                  "The Video Library",
                  PointerValue(),
                  MakePointerAccessor (&VideoServer::m_video_library),
                  MakePointerChecker<VideoLibrary> ())                   
  ;
  return tid;
}

VideoServer::VideoServer ()
{
  NS_LOG_FUNCTION (this);
  m_has_next_event = false;
  m_video_library = 0;
  m_connected = false;
}

VideoServer::~VideoServer ()
{
  NS_LOG_FUNCTION (this);
}

void
VideoServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}
 
void
VideoServer::StartApplication (void)
{
  // Create socket to listen for requests
  NS_LOG_FUNCTION (this);
  
  // Socket for video request (listen)
  m_socket_request = Socket::CreateSocket(GetNode( ), TypeId::LookupByName ("ns3::TcpSocketFactory"));
  m_socket_request->Bind(m_local_address);  
  m_socket_request->Listen ();
  m_socket_request->ShutdownSend ();
  m_socket_request->SetAcceptCallback (MakeNullCallback<bool, Ptr<Socket>,const Address &> (),MakeCallback(&VideoServer::HandleAccept, this));
  NS_LOG_INFO("Socket Created to listen for videos "
               << InetSocketAddress::ConvertFrom(m_local_address).GetIpv4()
               << " port " << InetSocketAddress::ConvertFrom(m_local_address).GetPort());
}

void
VideoServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  if (m_socket_request != 0)
  {
    m_socket_request->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  }
}

void VideoServer::ScheduleNextPacket ()
{
  NS_LOG_FUNCTION (this);
  // If there is no current video being sent
  if(m_current_video.video == 0)
  {
    // Select a new video from the queue
    if(m_video_queue.size() > 0)
    {
      NS_LOG_LOGIC ("New Video");
      m_current_video = m_video_queue.front();
      m_video_queue.pop(); // Remove from the queue
    }
    else
    {
      NS_LOG_LOGIC ("No Video in Queue");
      m_has_next_event = false;
      return;
    }
  }
  
  // Schedule next packet  
  uint32_t bits = m_packet_size * 8;
  NS_LOG_LOGIC ("bits = " << bits);
  Time nextTime = Seconds(bits / static_cast<double>(m_data_rate.GetBitRate ())); // Time till next packet
  NS_LOG_LOGIC ("nextTime = " << nextTime);
  m_nextPacketEvent = Simulator::Schedule (nextTime, &VideoServer::Send, this);
  NS_LOG_INFO("Sent " << m_current_video.sentPackets << " out of " <<  m_current_video.video->GetNumberOfPackets(m_packet_size) << " packets");
  m_current_video.sentPackets++;  
  if(m_current_video.sentPackets > m_current_video.video->GetNumberOfPackets(m_packet_size))
  {
    m_current_video.video = 0;  // Stop from sending further
  }
  else
  {
    m_has_next_event = true;
  } 
}

void VideoServer::Send()
{  
  NS_LOG_FUNCTION (this);
  if(m_current_video.video == 0) return;

  Ptr<Packet> packet = Create<Packet> (m_packet_size);
  NuoxusHeader header;
  header.SetSeq(1);
  header.SetContentId(m_current_video.video->GetId());
  header.SetPacketNumber(m_current_video.sentPackets);
  header.SetRequestId(m_current_video.requestId);
  packet->AddHeader(header);
  m_socket_video->Send (packet);
  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds()
               << "s on-off application sent "
               <<  packet->GetSize () << " bytes to "
               << InetSocketAddress::ConvertFrom(m_remote_address).GetIpv4 ()
               << " port " << InetSocketAddress::ConvertFrom(m_remote_address).GetPort ());
  m_has_next_event = false;
  ScheduleNextPacket();
}

void
VideoServer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
  {    
    if (packet->GetSize () > 0)
    {

        NuoxusHeader header;
        packet->RemoveHeader (header);

        // Extract the video ID from the tag    
        uint32_t videoId = header.GetContentId();     
        
        NS_LOG_INFO ("TraceDelay: RX " << packet->GetSize () <<
                     " bytes from "<< InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
                     " Sequence Number: " << header.GetSeq () <<
                     " Uid: " << packet->GetUid () <<
                     " TXtime: " << header.GetTs () <<
                     " RXtime: " << Simulator::Now () <<
                     " Delay: " << Simulator::Now () - header.GetTs ());
 
        NS_LOG_INFO("Video Request Received: " << videoId );   
        video_process_t video_process;     
        video_process.video       = m_video_library->GetVideo(videoId);
        video_process.sentPackets = header.GetPacketNumber();
        video_process.requestId   = header.GetRequestId();
        m_video_queue.push(video_process);
          
        // If there is not a next event schedule, than the next package will come from this video
        if(!m_has_next_event && m_connected) ScheduleNextPacket();
        
     }
  }
}

void VideoServer::ConnectionSucceeded (Ptr<Socket> socket)
{
  // Socket for video send (connect)
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
  if(!m_has_next_event) ScheduleNextPacket();
}

void VideoServer::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Simulator::Schedule (Seconds(2), &VideoServer::RetryConnect, this);
}

void VideoServer::RetryConnect ()
{
  NS_LOG_FUNCTION (this);
  m_socket_video->Connect(m_remote_address);
  m_socket_video->SetConnectCallback (
     MakeCallback (&VideoServer::ConnectionSucceeded, this),
     MakeCallback (&VideoServer::ConnectionFailed, this));       
}

void VideoServer::HandleAccept (Ptr<Socket> socket, const Address& from)
{
  NS_LOG_FUNCTION (this << socket << from);
  m_socket_request = socket;
  m_socket_request->SetRecvCallback (MakeCallback (&VideoServer::HandleRead, this));

  // Socket for video send (connect)
  m_socket_video = Socket::CreateSocket(GetNode(), TypeId::LookupByName ("ns3::TcpSocketFactory"));
  m_socket_video->Bind();
  m_socket_video->Connect(InetSocketAddress(InetSocketAddress::ConvertFrom(m_remote_address).GetIpv4(), InetSocketAddress::ConvertFrom(m_remote_address).GetPort() + 1000));
  m_socket_video->SetAllowBroadcast (true);
  m_socket_video->ShutdownRecv ();
  m_socket_video->SetConnectCallback (
     MakeCallback (&VideoServer::ConnectionSucceeded, this),
     MakeCallback (&VideoServer::ConnectionFailed, this));
  NS_LOG_INFO("Socket Created to send videos "
               << InetSocketAddress::ConvertFrom(m_remote_address).GetIpv4 ()
               << " port " << InetSocketAddress::ConvertFrom(m_remote_address).GetPort () + 1000);
}

} // Namespace ns3
