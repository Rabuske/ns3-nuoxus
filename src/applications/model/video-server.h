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

#ifndef VIDEO_SERVER_H
#define VIDEO_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/address.h"
#include "ns3/video-library.h"
#include "ns3/traced-callback.h"
#include "packet-loss-counter.h"
#include <queue>
#include <map>

namespace ns3 {
/**
 * \ingroup applications
 * \defgroup VideoServer VideoServer
 */

/**
 * \ingroup VideoServer
 *
 * \brief A Video Server
 *
 * It receives requests for videos and send the requested packages
 */

struct video_process_t{
  Ptr<VideoData> video;
  uint32_t sentPackets;
  uint64_t requestId;
};

class VideoServer : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  VideoServer ();
  virtual ~VideoServer ();

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \Send a packet of the current video
   */
  void Send (void);

  /**
   * \brief Auxiliar function to create a socket
   *
   * This client has a TCP and a UDP socket, to avoid having repeated code, 
   * this function was created.
   *
   * \param TypeID tid - The type ID of the SocketFactory
   */
  //Ptr<Socket>CreateSocket(TypeId tid, Address address, uint16_t port);

  /**
   * \brief Handle a packet reception.
   *
   * This function is called by lower layers.
   *
   * \param socket the socket the packet was received to.
   */
  void HandleRead (Ptr<Socket> socket);

  /**
   * \brief Schedule Next Packet
   *
   * Helper function to schedule the next packet
   *
   */        
  void ScheduleNextPacket( );

  /**
   * \brief Handle a Connection Succeed event
   * \param socket the connected socket
   */
  void ConnectionSucceeded (Ptr<Socket> socket);
  /**
   * \brief Handle a Connection Failed event
   * \param socket the not connected socket
   */
  void ConnectionFailed (Ptr<Socket> socket);

  void RetryConnect ();

  void HandleAccept(Ptr<Socket> socket, const Address& from);

  Address m_local_address;                  //!< Local address
  Address m_remote_address;                 //!< Remote peer address
  Ptr<Socket> m_socket_video;               //!< Videos are send through this socket
  Ptr<Socket> m_socket_request;             //!< Video Requests are received in this socket
  DataRate m_data_rate;                     //!< Rate that data is sent
  uint32_t m_packet_size;                   //!< Size of packets (including the SeqTsHeader)
  Ptr<VideoLibrary> m_video_library;         //!< Video Library 

  EventId   m_nextPacketEvent;              //!< Event to send the next packet
  bool      m_has_next_event;               //!< Indicates if there is a next scheduled event
  video_process_t m_current_video;           //!< Video being sent
  std::queue<video_process_t> m_video_queue; //!< Queue containing the videos to be sent (that have been requested)
  TracedCallback<Ptr<const Packet> > m_txTrace;
  bool      m_connected;

};

} // namespace ns3

#endif /* UDP_SERVER_H */
