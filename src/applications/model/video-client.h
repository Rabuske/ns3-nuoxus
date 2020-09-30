/*
 * Author: Felipe Rabuske Costa <rabuske@oulook.com>
 */

#ifndef VIDEO_CLIENT_H
#define VIDEO_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "video-library.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet-loss-counter.h"
#include "ns3/video-library.h"
#include "ns3/nuoxus.h"
#include "ns3/nuoxus-traced-values.h"
#include <map>

namespace ns3 {

class Socket;
class Packet;

class VideoClientRequestInfo
{
public:
  VideoClientRequestInfo();
  VideoClientRequestInfo(uint32_t videoId, Ipv4Address requester);

  void SetEndTime(Time time);
  void AddDelay(Time delay);

  Time GetStartTime(void);
  Time GetEndTime(void);
  Time GetAverageDelay(void);
  uint32_t GetVideoId(void);
  Ipv4Address GetRequester(void);

  bool operator==(const VideoClientRequestInfo& comp);    
  bool operator!=(const VideoClientRequestInfo& comp);  

private:
  Time m_start_time;
  Time m_end_time;
  Time m_total_delay;
  uint32_t m_number_of_packets;
  uint32_t m_video_id;
  Ipv4Address m_requester;
};


/**
 * \ingroup VideoClient
 *
 * \brief A Video application client. It requests to the video-server one of the videos available in the library.
 * 
 */
 typedef std::map<uint64_t, VideoClientRequestInfo> request_info_t;
 
class VideoClient : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  VideoClient ();

  virtual ~VideoClient ();

  static uint64_t GetNextRequestId(void)
  {
    return ++g_requestId;
  }

protected:
  virtual void DoDispose (void);

private:

  static uint64_t g_requestId;  

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \brief Request a video
   */
  void Request (uint32_t videoId);


  
  /**
   * \brief Handle a packet reception.
   *
   * This function is called by lower layers.
   *
   * \param socket the socket the packet was received to
   */
  void HandleRead (Ptr<Socket> socket);
  
  /**
   * \brief Schedule the next Video Request
   */
  void ScheduleNextVideoRequest(Time const & delay);

  /**
   * \brief Returns the size of the window used for checking loss.
   * \return the size of the window used for checking loss.
   */
  uint16_t GetPacketWindowSize () const;

  /**
   * \brief Set the size of the window used for checking loss. This value should
   *  be a multiple of 8
   * \param size the size of the window used for checking loss. This value should
   *  be a multiple of 8
   */
  void SetPacketWindowSize (uint16_t size);

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

  void RetryConnect();

  void HandleAccept (Ptr<Socket> socket, const Address& from);

  void RequestRandomVideo(void);

  uint32_t m_packet_size;         //!< Size of packets

  Ptr<Socket> m_socket_request;   //!< TCP Socket for request videos
  Ptr<Socket> m_socket_receive;   //!< UDP Socket for receiving videos
  Ptr<ZipfRandomVariable> m_zipf; //!< Zipf Distribution
  Address m_remote_address;       //!< Remote peer address
  Address m_local_address;        //!< Local address
  Ptr<VideoLibrary> m_video_library;   //!< Video Library 
  Ptr<Nuoxus> m_nuoxus;

  EventId m_nextVideoEvent;       //!< Event to request the next video

  PacketLossCounter m_packet_loss_counter; //!< Lost packet counter
  uint16_t m_window_size;                  //!< Window size to count packet loss

  TracedValue<VideoClientRequestInfo> m_traced_request;
  request_info_t m_non_traced_requests;                   // This will be manipulated until it is time to get it traced
};

} // namespace ns3

#endif /* VIDEO_CLIENT_H */
