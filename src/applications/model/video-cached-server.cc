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
#include "ns3/object-factory.h"
#include "packet-loss-counter.h"
#include "ns3/nuoxus-header-proactive.h"

#include "nuoxus-header.h"
#include "video-cached-server.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VideoCachedServer");

NS_OBJECT_ENSURE_REGISTERED (VideoCachedServer);


TypeId
VideoCachedServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VideoCachedServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<VideoCachedServer> ()
    .AddAttribute ("LocalAddress",
                   "The address of the local application (video cache)",
                   AddressValue (),
                   MakeAddressAccessor (&VideoCachedServer::m_local_address),
                   MakeAddressChecker ())
    .AddAttribute ("ClientAddress",
                   "The address of the client",
                   AddressValue (),
                   MakeAddressAccessor (&VideoCachedServer::m_client_address),
                   MakeAddressChecker ())
    .AddAttribute ("ServerAddress",
                   "The address of the remote servers",
                   AddressValue (),
                   MakeAddressAccessor (&VideoCachedServer::m_server_address),
                   MakeAddressChecker ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (512),
                   MakeUintegerAccessor (&VideoCachedServer::m_packet_size),
                   MakeUintegerChecker<uint32_t> (12,1500))                   
    .AddAttribute("VideoLibrary",
                  "The Video Library",
                  PointerValue(),
                  MakePointerAccessor (&VideoCachedServer::m_video_library),
                  MakePointerChecker<VideoLibrary> ())       
    .AddAttribute("Nuoxus",
                  "The Nuoxus Instance",
                  PointerValue(),
                  MakePointerAccessor (&VideoCachedServer::m_nuoxus),
                  MakePointerChecker<Nuoxus> ())                                           
  ;
  return tid;
}

VideoCachedServer::VideoCachedServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket_client_request_receive = 0;
  m_socket_server_video_receive   = 0;
  m_socket_server_request_send    = 0;
  m_socket_client_video_send      = 0;
  m_video_library                 = 0;
  m_nuoxus                        = 0;
}

VideoCachedServer::~VideoCachedServer ()
{
  NS_LOG_FUNCTION (this);
}

void
VideoCachedServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}
 
// 1
void
VideoCachedServer::StartApplication (void)
{
  // Create socket to listen for requests
  NS_LOG_FUNCTION (this);

  // Socket for receiving video requests (listen)
  m_socket_client_request_receive = Socket::CreateSocket(GetNode( ), TypeId::LookupByName ("ns3::TcpSocketFactory"));
  m_socket_client_request_receive->Bind(m_local_address);  
  m_socket_client_request_receive->Listen ();
  m_socket_client_request_receive->ShutdownSend();
  m_socket_client_request_receive->SetAcceptCallback (MakeNullCallback<bool, Ptr<Socket>, const Address &> (), MakeCallback(&VideoCachedServer::HandleAcceptForVideoRequestClient, this));
  NS_LOG_INFO("Socket Created to listen for video requests from client "
              << InetSocketAddress::ConvertFrom(m_local_address).GetIpv4()
              << " port " << InetSocketAddress::ConvertFrom(m_local_address).GetPort());
}

void
VideoCachedServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  if (m_socket_client_request_receive != 0)
  {
    m_socket_client_request_receive->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  }

  if (m_socket_server_request_send != 0)
  {
    m_socket_server_request_send->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  }
  if(m_socket_server_video_receive != 0)
  {
    m_socket_server_video_receive->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  }
}


void
VideoCachedServer::HandleReceiveVideoFromServer(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);  

  // Get content from the received packet
  Address from;
  Ptr<Packet> packet = socket->RecvFrom (from);

  if(packet == 0  || packet->GetSize () <= 0 || m_current_video.video == 0)  
  {
    // Error in this content. Request Next.
    m_current_video.video = 0;
    ScheduleNextVideo();
    return;    
  }

  NuoxusHeader header;
  packet->RemoveHeader(header);
  // Make sure we are processing the same video
  if(m_current_video.video->GetId() != header.GetContentId())
  {
    ScheduleNextVideo();
    return;    
  }

  m_current_video.sentPackets = header.GetPacketNumber();
  m_current_video.requestId   = header.GetRequestId();  

  NS_LOG_INFO ("TraceDelay: RX "     << packet->GetSize () <<
                " bytes from "       << InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
                " Sequence Number: " << header.GetSeq() <<
                " Uid: "             << packet->GetUid () <<
                " TXtime: "          << header.GetTs () <<
                " RXtime: "          << Simulator::Now () <<
                " Delay: "           << Simulator::Now () - header.GetTs () <<
                " Video: "           << header.GetContentId() <<
                " Packet: "          << header.GetPacketNumber() );

  // Let nuoxus know that this packet has been received, in case it should be stored it would also store it
  m_nuoxus->NotifyReceived(m_current_video.video->GetId(), m_current_video.sentPackets); // + Packet

  // Forward this packet to the client
  Ptr<Packet> packetfwd = Create<Packet> (m_packet_size);
  packetfwd->AddHeader(header);
  m_socket_client_video_send->Send(packetfwd);

  NS_LOG_INFO("Received " << m_current_video.sentPackets << " out of " <<  m_current_video.video->GetNumberOfPackets(m_packet_size) << " packets");

  // In case the server has finished sending this video
  if(m_current_video.sentPackets >= m_current_video.video->GetNumberOfPackets(m_packet_size))
  {
    m_current_video.video = 0;
    ScheduleNextVideo();
    return;
  }  
}

void
VideoCachedServer::HandleVideoRequestFromClient (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
  {    
    if (packet->GetSize () <= 0) continue;

    NuoxusHeader header;
    packet->RemoveHeader (header);

    video_process_t video_process;
    video_process.video       = m_video_library->GetVideo(header.GetContentId());
    video_process.sentPackets = header.GetPacketNumber();
    video_process.requestId   = header.GetRequestId();
    m_video_queue.push(video_process);
    
    NS_LOG_INFO ("TraceDelay: RX "     << packet->GetSize () <<
                  " bytes from "       << InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
                  " Sequence Number: " << header.GetSeq() <<
                  " Uid: "             << packet->GetUid () <<
                  " TXtime: "          << header.GetTs () <<
                  " RXtime: "          << Simulator::Now () <<
                  " Delay: "           << Simulator::Now () - header.GetTs () <<
                  " Video: "           << video_process.video->GetId() <<
                  " Packet: "          << video_process.sentPackets );                    

    ScheduleNextVideo();
  }
}

void VideoCachedServer::ScheduleNextVideo()
{
  NS_LOG_FUNCTION (this);
  // If the connections have not been established, wait 1 second to try again
  if(!m_connections_created) 
  {
    NS_LOG_INFO("Not all connections have been created yet. Scheduling for a next time.");
    Simulator::Schedule(Seconds(1.0), &VideoCachedServer::ScheduleNextVideo, this);
  }

  // If there is no current video being sent or all the packages of the current one have been sent
  if(m_current_video.video != 0 )
  {
    NS_LOG_INFO("Video " << m_current_video.video->GetId() << " already processing");
    return;
  }

  // Select a new video from the queue
  if(m_video_queue.size() <= 0)
  {
    NS_LOG_LOGIC ("No Video in Queue");
    return;      
  }
  NS_LOG_LOGIC ("New Video");
  m_current_video = m_video_queue.front();
  m_video_queue.pop();                           
  
  // Register this request in Nuoxus
  m_nuoxus->RegisterRequest(m_current_video.video->GetId(), InetSocketAddress::ConvertFrom(m_client_address).GetIpv4 (), m_current_video.video->GetNumberOfPackets(m_packet_size));

  // Checks if the video is in cache
  if(m_nuoxus->IsContentCached(m_current_video.video->GetId()))
  {
    // Retrieve from the cache
    NextPacketFromNuoxus();
  }
  // Request it from the server  
  else
  {
    NextVideoFromServer();
  }             
}


void VideoCachedServer::NextPacketFromNuoxus()
{
  NS_LOG_FUNCTION (this);
  m_current_video.sentPackets++;
  // In case the server has finished sending this video
  if(m_current_video.sentPackets > m_current_video.video->GetNumberOfPackets(m_packet_size))
  {
    m_current_video.video = 0;
    ScheduleNextVideo();
    return;
  }
  
  // Get the packet from nuoxus
  Ptr<Packet> packet = m_nuoxus->GetContent(m_current_video.video->GetId(), m_current_video.sentPackets);
  
  // It might happen that the requested packet is not in cache (the video was partially saved).
  // In this case, the packets come from the server
  if(packet == 0)
  {
    m_current_video.sentPackets--;
    NextVideoFromServer();
    return;    
  }

  // Add the header
  NuoxusHeader header;
  header.SetPacketNumber(m_current_video.sentPackets);
  header.SetContentId(m_current_video.video->GetId());
  header.SetRequestId(m_current_video.requestId);
  packet->AddHeader(header);

  // Send the packet retrieved from nuoxus to the client
  NS_LOG_INFO ("At time "                  << Simulator::Now ().GetSeconds() << 
               "s the application sent "   <<  packet->GetSize () << 
              " bytes to "                 << InetSocketAddress::ConvertFrom(m_client_address).GetIpv4 () <<
              " port "                     << InetSocketAddress::ConvertFrom(m_client_address).GetPort () <<
              " from Nuoxus cache. Video " << m_current_video.video->GetId() <<
              " packet "                   << m_current_video.sentPackets <<
              " out of "                   << m_current_video.video->GetNumberOfPackets(m_packet_size) );

  m_socket_client_video_send->Send (packet);
  // Schedule next packet  
  uint32_t bits = m_packet_size * 8;
  NS_LOG_LOGIC ("bits = " << bits);
  Time nextTime = Seconds(bits / static_cast<double>(DataRate ("5000kb/s").GetBitRate ())); // Time till next packet
  NS_LOG_LOGIC ("nextTime = " << nextTime);
  Simulator::Schedule (nextTime, &VideoCachedServer::NextPacketFromNuoxus, this);
}

void VideoCachedServer::NextVideoFromServer()
{
  NS_LOG_FUNCTION (this);
  Ptr<Packet> packet = Create<Packet>(m_packet_size); 

  // Add the header
  NuoxusHeader header;
  header.SetPacketNumber(m_current_video.sentPackets);
  header.SetContentId(m_current_video.video->GetId());
  header.SetRequestId(m_current_video.requestId);
  header.SetSeq(1);          // Only 1 package is required
  packet->AddHeader(header);
  
  // Log - video was requested
  if ((m_socket_server_request_send->Send(packet)) >= 0)
  {
    NS_LOG_INFO ("TraceDelay TX "     << m_packet_size << 
                 " bytes to "         << InetSocketAddress::ConvertFrom(m_client_address).GetIpv4 () << 
                 " Uid: "             << packet->GetUid () << 
                 " Time: "            << (Simulator::Now ()).GetSeconds () <<
                 " Requested Video: " << m_current_video.video->GetId() );    
  }
  else
  {
    NS_LOG_INFO ("Error while sending " << m_packet_size << 
                 " bytes to "           << InetSocketAddress::ConvertFrom(m_client_address).GetIpv4()
                );
  }
}

void VideoCachedServer::ProactiveCacheRequest(uint32_t contentId, double_t score)
{
  NS_LOG_FUNCTION (this << contentId << score);
  // This function will send the proactive request to the client
  Ptr<Packet> packet = Create<Packet>(m_packet_size); 
  NuoxusProactiveHeader header;
  header.SetContentId(contentId);
  header.SetScore(score);
  packet->AddHeader(header);
  m_socket_client_video_send->Send (packet);
  NS_LOG_INFO ("Sent proactive request of " << contentId << 
               " to "                       << InetSocketAddress::ConvertFrom(m_client_address).GetIpv4 () << 
               " Uid: "                     << packet->GetUid () << 
               " Time: "                    << (Simulator::Now ()).GetSeconds () );      
}


// 3
void VideoCachedServer::ConnectionToServerRequestSucceeded(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  if(m_socket_server_video_receive != 0) return;
  // Socket to receive videos from the server (listen)
  m_socket_server_video_receive = Socket::CreateSocket(GetNode(), TypeId::LookupByName ("ns3::TcpSocketFactory"));
  // + 1000 is a hack to avoiding passing multiple ports
  m_socket_server_video_receive->Bind(InetSocketAddress(Ipv4Address::GetAny (), InetSocketAddress::ConvertFrom(m_local_address).GetPort() + 1000)); 
  m_socket_server_video_receive->Listen();
  m_socket_server_video_receive->ShutdownSend ();
  m_socket_server_video_receive->SetAcceptCallback (MakeNullCallback<bool , Ptr<Socket>, const Address &> (), MakeCallback(&VideoCachedServer::HandleAcceptForVideoReceiveServer, this));  
  NS_LOG_INFO("Socket Created to receive videos from server "
              << InetSocketAddress::ConvertFrom(m_local_address).GetIpv4 ()
              << " port " << InetSocketAddress::ConvertFrom(m_local_address).GetPort () + 1000);   
}

void VideoCachedServer::ConnectionToServerRequestFailed(Ptr<Socket> socket)
{
   NS_LOG_FUNCTION (this << socket);
   Simulator::Schedule (Seconds(2), &VideoCachedServer::RetryConnectionToServerRequest, this);
}

void VideoCachedServer::RetryConnectionToServerRequest()
{
   NS_LOG_FUNCTION (this);
   m_socket_server_request_send->Connect(m_server_address);
   m_socket_server_request_send->SetConnectCallback (
    MakeCallback (&VideoCachedServer::ConnectionToServerRequestSucceeded, this),
    MakeCallback (&VideoCachedServer::ConnectionToServerRequestFailed, this)
  );
}

// 2
void VideoCachedServer::HandleAcceptForVideoRequestClient (Ptr<Socket> socket, const Address& from)
{
  NS_LOG_FUNCTION (this << socket);  

  m_socket_client_request_receive = socket;
  m_socket_client_request_receive->SetRecvCallback (MakeCallback (&VideoCachedServer::HandleVideoRequestFromClient, this));


  if(m_socket_server_request_send != 0) return;
  // Socket to request videos to the video server
  m_socket_server_request_send = Socket::CreateSocket(GetNode(), TypeId::LookupByName ("ns3::TcpSocketFactory"));
  m_socket_server_request_send->Bind();
  m_socket_server_request_send->Connect(m_server_address);
  m_socket_server_request_send->ShutdownRecv ();
  m_socket_server_request_send->SetAllowBroadcast(true);
  m_socket_server_request_send->SetConnectCallback (
    MakeCallback (&VideoCachedServer::ConnectionToServerRequestSucceeded, this),
    MakeCallback (&VideoCachedServer::ConnectionToServerRequestFailed, this)
  );
  NS_LOG_INFO("Socket Created to send requests to Server "
              << InetSocketAddress::ConvertFrom(m_server_address).GetIpv4 ()
              << " port " << InetSocketAddress::ConvertFrom(m_server_address).GetPort ()); 

}

// 4  
void VideoCachedServer::HandleAcceptForVideoReceiveServer (Ptr<Socket> socket, const Address& from)
{
  // Socket to send videos to the client
  NS_LOG_FUNCTION (this << socket << from);
  m_socket_server_video_receive = socket;  
  m_socket_server_video_receive->SetRecvCallback (MakeCallback (&VideoCachedServer::HandleReceiveVideoFromServer, this));

  if(m_socket_client_video_send != 0) return;
  m_socket_client_video_send = Socket::CreateSocket(GetNode(), TypeId::LookupByName ("ns3::TcpSocketFactory"));
  m_socket_client_video_send->Bind();
  m_socket_client_video_send->Connect(m_client_address);
  m_socket_client_video_send->SetAllowBroadcast (true);
  m_socket_client_video_send->ShutdownRecv ();
  m_socket_client_video_send->SetConnectCallback (
    MakeCallback (&VideoCachedServer::ConnectionToClientSendSucceeded, this),
    MakeCallback (&VideoCachedServer::ConnectionToClientSendFailed, this)
  );
  NS_LOG_INFO("Socket Created to send videos to client "
    << InetSocketAddress::ConvertFrom(m_client_address).GetIpv4 ()
    << " port " << InetSocketAddress::ConvertFrom(m_client_address).GetPort ()
  );
}

// 5
void VideoCachedServer::ConnectionToClientSendSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connections_created = true;
}

void VideoCachedServer::ConnectionToClientSendFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Simulator::Schedule (Seconds(2), &VideoCachedServer::RetryConnectionToClientSend, this);
}  

void VideoCachedServer::RetryConnectionToClientSend()
{
  NS_LOG_FUNCTION (this);
  m_socket_client_video_send->Connect(m_client_address);
  m_socket_client_video_send->SetConnectCallback (
    MakeCallback (&VideoCachedServer::ConnectionToClientSendSucceeded, this),
    MakeCallback (&VideoCachedServer::ConnectionToClientSendFailed, this)
  );
}

void VideoCachedServer::RegisterAsNuoxusProactiveCachingCallback()
{
  NS_LOG_FUNCTION (this);
  m_nuoxus->AddProactiveCallback(InetSocketAddress::ConvertFrom(m_client_address).GetIpv4(), this);
}

} // Namespace ns3

