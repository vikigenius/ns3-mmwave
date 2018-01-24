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
 *
 */

#ifndef MMWAVE_TCP_RTO_AVOIDER_H
#define MMWAVE_TCP_RTO_AVOIDER_H

#include <ns3/traced-callback.h>
#include <ns3/object.h>
#include <ns3/string.h>
#include <ns3/sequence-number.h>
#include <ns3/lte-rlc-sequence-number.h>
#include <ns3/packet.h>
#include <ns3/tcp-socket-base.h>
#include <map>
#include <memory>
#include <ns3/ptr.h>
#include <ns3/node.h>
#include <ns3/application.h>

namespace ns3 {

class MmWaveTcpRtoAvoider : public Object
{
public:
  /// Constructor
  MmWaveTcpRtoAvoider (Ptr<Node> ueNode);

  /**
   * Class destructor
   */
  virtual
  ~MmWaveTcpRtoAvoider ();

  // Inherited from ns3::Object
  /**
   *  Register this type.
   *  \return The object TypeId.
   */
  static TypeId GetTypeId (void);
  void DoDispose ();

  /**
   * Notifies the RTO avoider that an uplink reception has occurred.
   * @param device Identifies entity as UE or ENB
   * @param cellId CellId of the attached Enb
   * @param imsi IMSI of the UE who received the PDU
   * @param rnti C-RNTI of the UE who received the PDU
   * @param lcid LCID through which the PDU has been received
   * @param packet The PDU which has been retransmitted
   * @param bufferSize The size of the RLC reception buffer
   * @param vrR 
   * @param vrH
   */  
  void
  HandleRlcBuffering (std::string device, uint16_t cellId, uint64_t imsi, uint16_t rnti, uint8_t lcid, Ptr<const Packet> packet, uint16_t bufferPackets, uint64_t bufferSize, SequenceNumber10 vrR, SequenceNumber10 vrH);
 
private:
  struct SockInfo;
  typedef std::map <Address, std::shared_ptr<SockInfo>> SocketMap; 
  
  /**
   * Does Deep Packet Inspection and gets required parameter values
   * @param packet The packet to be inspected
   */
  void DoDpi (Ptr<const Packet> packet);

  /*
   * \brief Gets the Information about the socket from Address
   * Fills the Socket Information
   * \param sockAddress the address of the socket to extract info from
   * \return the socket information
   */
  SockInfo GetSocketInfo (Address sockAddres);

  /*
   * \brief Updates the Information about the socket from Address
   * Fills the Socket Information
   * \param sockAddress the address of the socket to extract info from
   * \return the socket information
   */
  SockInfo UpdateSockInfo (SockInfo sockInfo);
  
  void SendSackInd ();

  std::list<SequenceNumber32> m_bufferedList;
  Ptr<Node> m_ueNode;
  SocketMap m_sockInfoMap;
  std::shared_ptr<SockInfo> m_curSockInfo;
};


}

#endif
