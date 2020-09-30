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
 *
 */

#ifndef VIDEO_CACHE_SERVER_H
#define VIDEO_CACHE_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/address.h"
#include "ns3/video-library.h"
#include "ns3/traced-callback.h"
#include "ns3/nuoxus.h"
#include "packet-loss-counter.h"
#include "ns3/video-server.h"
#include "ns3/callback.h"
#include "ns3/nuoxus-proactive-callback.h"
#include <queue>


namespace ns3 {
/**
 * \ingroup applications
 * \defgroup VideoCachedServer VideoCachedServer
 */

/**
 * \ingroup VideoCachedServer
 *
 * \brief A Video Server
 *
 * It receives requests for videos and send the requested packages
 */

class VideoCachedServer : public Application, public NuoxusProactiveCallback
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  VideoCachedServer ();
  virtual ~VideoCachedServer ();
  void ProactiveCacheRequest(uint32_t contentId, double_t score);
  void RegisterAsNuoxusProactiveCachingCallback();

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleReceiveVideoFromServer(Ptr<Socket> socket);
  void HandleVideoRequestFromClient (Ptr<Socket> socket);
  void ScheduleNextVideo();
  void NextPacketFromNuoxus();
  void NextVideoFromServer();
  void HandleAcceptForVideoRequestClient (Ptr<Socket> socket, const Address& from);
  void HandleAcceptForVideoReceiveServer (Ptr<Socket> socket, const Address& from);
  void ConnectionToServerRequestSucceeded(Ptr<Socket> socket);
  void ConnectionToServerRequestFailed(Ptr<Socket> socket);
  void RetryConnectionToServerRequest();
  void ConnectionToClientSendSucceeded(Ptr<Socket> socket);
  void ConnectionToClientSendFailed(Ptr<Socket> socket);
  void RetryConnectionToClientSend();

  Address m_local_address;                       //!< Local address
  Address m_client_address;                      //!< Remote client
  Address m_server_address;                      //!< Video Server Address  

  Ptr<Socket> m_socket_client_request_receive;   //!< Videos are requested by clientes using this socket
  Ptr<Socket> m_socket_server_video_receive;     //!< Videos received from the video server by this socket
  Ptr<Socket> m_socket_server_request_send;      //!< Videos requested to the video server by this socket
  Ptr<Socket> m_socket_client_video_send;        //!< Videos requested to the video server by this socket

  Ptr<Nuoxus> m_nuoxus;

  uint32_t m_packet_size;                        //!< Size of packets (including the SeqTsHeader)
  Ptr<VideoLibrary> m_video_library;             //!< Video Library 

  video_process_t m_current_video;               //!< Video being sent
  std::queue<video_process_t> m_video_queue;     //!< Queue containing the videos to be sent (that have been requested)

  bool m_connections_created;                    //!< Indicates that all connections are created
};

} // namespace ns3

#endif /* UDP_SERVER_H */
