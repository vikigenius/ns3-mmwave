 /* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
 /*
 *   Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *   Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York University
 *   Copyright (c) 2016, University of Padova, Dep. of Information Engineering, SIGNET lab. 
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *   Author: Vikash Kathirvel <master.bvik@gmail.com>
 */

#include "mmwave-tcp-rto-avoider.h"
#include <ns3/log.h>
#include <ns3/packet.h>
#include <ns3/sequence-number.h>
#include <ns3/lte-rlc-sequence-number.h>
#include <ns3/lte-rlc-am-header.h>
#include <ns3/lte-pdcp-header.h>
#include <ns3/ipv4-header.h>
#include <ns3/tcp-header.h>
#include <ns3/node.h>
#include <ns3/ptr.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MmWaveTcpRtoAvoider");

NS_OBJECT_ENSURE_REGISTERED (MmWaveTcpRtoAvoider);

MmWaveTcpRtoAvoider::MmWaveTcpRtoAvoider ()
{
  NS_LOG_FUNCTION(this);
}

MmWaveTcpRtoAvoider::~MmWaveTcpRtoAvoider()
{
  NS_LOG_FUNCTION(this);
}

TypeId
MmWaveTcpRtoAvoider::GetTypeId () 
{
  static TypeId tid =
    TypeId ("ns3::MmWaveTcpRtoAvoider")
    .SetParent<Object> ()
    .AddConstructor<MmWaveTcpRtoAvoider> ()
    .SetGroupName("Lte")
    ;
  return tid;
}

void
MmWaveTcpRtoAvoider::DoDispose()
{
  NS_LOG_FUNCTION(this);
}

void
MmWaveTcpRtoAvoider::SetUe(Ptr<Node> ueNode)
{
  NS_LOG_FUNCTION(this);
  m_ueNode = ueNode;    
}

void
MmWaveTcpRtoAvoider::NotifyRlcBuffering(std::string device, uint16_t cellId, uint64_t imsi, uint16_t rnti, uint8_t lcid, Ptr<const Packet> packet, uint16_t bufferPackets, uint64_t bufferSize, SequenceNumber10 vrR, SequenceNumber10 vrH)
{
  LteRlcAmHeader rlcHeader;
  packet->PeekHeader(rlcHeader);
  if(rlcHeader.GetSequenceNumber().GetValue() > vrR.GetValue())
    {
      DoDpi(packet);      
    }
}

void
MmWaveTcpRtoAvoider::DoDpi(Ptr<const Packet> packet)
{
  // Copy packet just in case to avoid any issues
  Ptr<Packet> copiedPacket = packet->Copy();
  //Remove Headers in order one by one
  LteRlcAmHeader rlcHeader;
  LtePdcpHeader pdcpHeader;
  Ipv4Header ipv4Header;
  TcpHeader tcpHeader;

  copiedPacket->RemoveHeader(rlcHeader);

  //Check for Data Packet, only then remove IP headers
  if (rlcHeader.IsDataPdu()) {
    copiedPacket->RemoveHeader(pdcpHeader);
    copiedPacket->RemoveHeader(ipv4Header);

    //Now Check for the TCP header
    copiedPacket->PeekHeader(tcpHeader);
    if (tcpHeader.GetDestinationPort())
      {
        m_bufferedList.push_back(tcpHeader.GetSequenceNumber());
      }

    // Now put back the IP and PDCP headers
    copiedPacket->AddHeader(ipv4Header);
    copiedPacket->AddHeader(pdcpHeader);
  }
  // Put back the RLC Header
  copiedPacket->AddHeader(rlcHeader);

  //Inform TCP about the packet
  Ptr<TcpSocketBase> ueSockPtr = m_ueNode->GetObject<TcpSocketBase>();
}

}//namespace ns3
