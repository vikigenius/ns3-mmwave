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
#include "mmwave-tcp-sack-buffer.h"
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
#include <ns3/packet-sink.h>
#include <ns3/application.h>
#include <ns3/address.h>
#include <ns3/inet-socket-address.h>
#include <map>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MmWaveTcpRtoAvoider");

NS_OBJECT_ENSURE_REGISTERED (MmWaveTcpRtoAvoider);

static Address
GetSockAddress (Ipv4Address srcAddr, uint16_t srcPort)
{
  auto inetaddr = InetSocketAddress(srcAddr, srcPort);
  return Address (inetaddr);
}

struct
MmWaveTcpRtoAvoider::SockInfo
{
  Ptr<TcpSocketBase> sockPtr;
  SequenceNumber32 nextRxSequence;
  Ptr<MmWaveTcpSackBuffer> sackBuffer;
  struct PduBuffer
  {
    std::list<Ptr<Packet>> byteSegments;
    bool isComplete;
    uint32_t currSize;
    uint32_t totalSize;
  };
  typedef std::map<SequenceNumber32, PduBuffer> PacketBuffer;
  PacketBuffer packetBuffer;
  bool isBuffered;
};

MmWaveTcpRtoAvoider::MmWaveTcpRtoAvoider (Ptr<Node> ueNode)
  :m_ueNode(ueNode)
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
MmWaveTcpRtoAvoider::HandleRlcBuffering(std::string device, uint16_t cellId, uint64_t imsi, uint16_t rnti, uint8_t lcid, Ptr<const Packet> packet, uint16_t bufferPackets, uint64_t bufferSize, SequenceNumber10 vrR, SequenceNumber10 vrH)
{
  LteRlcAmHeader rlcHeader;
  packet->PeekHeader(rlcHeader);
  if(rlcHeader.GetSequenceNumber().GetValue() <= vrR.GetValue())
    {
      return; //In Sequence or Recovery: Nothing to see here
    }
  
  DoDpi(packet);
  if (m_curSockInfo)
    {
      //Update Sack Buffer
      UpdateSackBuffer();
      m_curSockInfo->sockPtr->SendCustomSack (MakeCallback(&MmWaveTcpRtoAvoider::BuildSackOption, this));
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
    uint32_t actualPayloadSize = copiedPacket->GetSize();
    copiedPacket->RemoveHeader(tcpHeader);
    
    //Now Check for the TCP header
    if (tcpHeader.GetDestinationPort())
      {
        SequenceNumber32 headSeq = tcpHeader.GetSequenceNumber();
        //auto sockAdd = GetSockAddress(ipv4Header.GetSource() , tcpHeader.GetSourcePort());
        // Use Destination address because UE Rlc is the receiver
        auto sockAdd = GetSockAddress(ipv4Header.GetDestination() , tcpHeader.GetDestinationPort());
        auto sockIter = m_sockInfoMap.find(sockAdd);
        if (sockIter != m_sockInfoMap.end())
          {
            auto curSockPtr = sockIter->second->sockPtr;
            auto rxBuffer = curSockPtr->GetRxBuffer ();
            sockIter->second->nextRxSequence = rxBuffer->NextRxSequence();
            sockIter->second->sackBuffer->MirrorSackList(rxBuffer->GetSackList());
            m_curSockInfo = sockIter->second;
          }
        else
          {
            m_curSockInfo = std::make_shared<SockInfo>(GetSocketInfo(sockAdd));
            m_sockInfoMap[sockAdd] = m_curSockInfo;
          }
        if (ipv4Header.GetPayloadSize() > actualPayloadSize)
          {
            ;
          }
        else
          {
            m_bufferedList.emplace(headSeq, headSeq + copiedPacket->GetSize()); //Note that we have removed all haders upto and including TCP, so size won't include them
          }
      }
    else
      {
        m_curSockInfo.reset();
      }

    // Now put back the TCP, IP and PDCP headers
    copiedPacket->AddHeader(tcpHeader);
    copiedPacket->AddHeader(ipv4Header);
    copiedPacket->AddHeader(pdcpHeader);
  }
  // Put back the RLC Header
  copiedPacket->AddHeader(rlcHeader);
}

MmWaveTcpRtoAvoider::SockInfo
MmWaveTcpRtoAvoider::GetSocketInfo (Address sockAddr)
{
  SockInfo sockInfo;
  uint32_t numApps = m_ueNode->GetNApplications();
  for (uint32_t appid = 0; appid < numApps; appid++) {
    Ptr<Application> app = m_ueNode->GetApplication(appid);
    auto socketPtrList = app->GetObject<PacketSink>()->GetAcceptedSockets();
    for (auto const socket : socketPtrList)
      {
        Address realSockAddr;
        socket->GetSockName(realSockAddr);
        if (sockAddr == realSockAddr) {
          sockInfo.sockPtr = socket->GetObject<TcpSocketBase>();
          sockInfo.nextRxSequence = sockInfo.sockPtr->GetRxBuffer()->NextRxSequence();
          sockInfo.sackBuffer = CreateObject<MmWaveTcpSackBuffer>();
          sockInfo.sackBuffer->MirrorSackList (sockInfo.sockPtr->GetRxBuffer()->GetSackList());
        }
      }
  }
  return sockInfo;
}

void
MmWaveTcpRtoAvoider::UpdateSackBuffer ()
{
  while (!m_bufferedList.empty())
    {
      auto seqPair = m_bufferedList.front();
      if (m_curSockInfo->nextRxSequence < seqPair.first)
        {
          m_curSockInfo->sackBuffer->AddSackBlock(seqPair.first, seqPair.second);
        }
      m_bufferedList.pop();
    }
}

void
MmWaveTcpRtoAvoider::BuildSackOption (TcpHeader &header)
{
  NS_LOG_FUNCTION (this << header);

  // Calculate the number of SACK blocks allowed in this packet
  uint8_t optionLenAvail = header.GetMaxOptionLength () - header.GetOptionLength ();
  uint8_t allowedSackBlocks = (optionLenAvail - 2) / 8;

  TcpOptionSack::SackList sackList = m_curSockInfo->sackBuffer->GetSackList();
  
  if (allowedSackBlocks == 0 || sackList.empty ())
    {
      NS_LOG_LOGIC ("No space available or sack list empty, not adding sack blocks");
      return;
    }

  // Append the allowed number of SACK blocks
  Ptr<TcpOptionSack> option = CreateObject<TcpOptionSack> ();
  TcpOptionSack::SackList::iterator i;
  for (i = sackList.begin (); allowedSackBlocks > 0 && i != sackList.end (); ++i)
    {
      NS_LOG_LOGIC ("Left edge of the block: " << (*i).first << " Right edge of the block: " << (*i).second);
      option->AddSackBlock (*i);
      allowedSackBlocks--;
    }

  header.AppendOption (option);
}
}//namespace ns3
