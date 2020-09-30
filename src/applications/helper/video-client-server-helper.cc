/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */
#include "video-client-server-helper.h"
#include "ns3/video-server.h"
#include "ns3/video-client.h"
#include "ns3/video-trace-client.h"
#include "ns3/video-cached-server.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/string.h"

namespace ns3 {

VideoServerHelper::VideoServerHelper (Address localAddress,
                                      Address remoteAddress,
                                      DataRate datarate, 
                                      uint32_t packetSize,
                                      Ptr<VideoLibrary> videoLibrary) 
{
  m_factory.SetTypeId (VideoServer::GetTypeId ());
  SetAttribute("LocalAddress", AddressValue(localAddress));
  SetAttribute("RemoteAddress", AddressValue(remoteAddress));
  SetAttribute("DataRate", DataRateValue(datarate));
  SetAttribute("PacketSize", UintegerValue(packetSize)); 
  SetAttribute("VideoLibrary", PointerValue(videoLibrary));
}

void
VideoServerHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
VideoServerHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<VideoServer> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

Ptr<VideoServer>
VideoServerHelper::GetServer (void)
{
  return m_server;
}

VideoClientHelper::VideoClientHelper (Address localAddress, 
                                      Address remoteAddress, 
                                      uint32_t packetSize, 
                                      Ptr<ZipfRandomVariable> zipfRandomVariable, 
                                      uint16_t packetWindowSize,
                                      Ptr<VideoLibrary> videoLibrary)
{
  m_factory.SetTypeId (VideoClient::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue(localAddress));
  SetAttribute ("RemoteAddress", AddressValue(remoteAddress));
  SetAttribute ("PacketSize", UintegerValue (packetSize));
  SetAttribute ("ZipfDistribution", PointerValue(zipfRandomVariable));
  SetAttribute ("PacketWindowSize", UintegerValue (packetSize));
  SetAttribute("VideoLibrary", PointerValue(videoLibrary));
}

void
VideoClientHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
VideoClientHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<VideoClient> client = m_factory.Create<VideoClient> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}


VideoCachedServerHelper::VideoCachedServerHelper (Address localAddress, 
                                                  Address clientAddress, 
                                                  Address serverAddress, 
                                                  uint32_t packetSize, 
                                                  Ptr<Nuoxus> nuoxus,
                                                  Ptr<VideoLibrary> videoLibrary)
{
  m_factory.SetTypeId (VideoCachedServer::GetTypeId ());
  SetAttribute ("LocalAddress", AddressValue(localAddress));
  SetAttribute ("ClientAddress", AddressValue(clientAddress));
  SetAttribute ("ServerAddress", AddressValue(serverAddress));  
  SetAttribute ("PacketSize", UintegerValue (packetSize));
  SetAttribute ("Nuoxus", PointerValue(nuoxus));
  SetAttribute ("VideoLibrary", PointerValue(videoLibrary));
}

void
VideoCachedServerHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer VideoCachedServerHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<VideoCachedServer> cachedServer = m_factory.Create<VideoCachedServer> ();
      cachedServer->RegisterAsNuoxusProactiveCachingCallback();
      node->AddApplication (cachedServer);
      apps.Add (cachedServer);
    }
  return apps;
}


} // namespace ns3
