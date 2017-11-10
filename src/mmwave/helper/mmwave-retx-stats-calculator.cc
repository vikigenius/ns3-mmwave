/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Copyright (c) 2017 SIGNET Lab, Dept. of Information Engineering, UNIPD
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
 * Author: Vikash Kathirvel <master.bvik@gmail.com>
 */

#include "ns3/mmwave-retx-stats-calculator.h"
#include "ns3/string.h"
#include "ns3/nstime.h"
#include "ns3/lte-rlc-am-header.h"
#include <ns3/packet.h>
#include <ns3/log.h>
#include <vector>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MmWaveRetxStatsCalculator");

NS_OBJECT_ENSURE_REGISTERED ( MmWaveRetxStatsCalculator);

MmWaveRetxStatsCalculator::MmWaveRetxStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}


MmWaveRetxStatsCalculator::~MmWaveRetxStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
MmWaveRetxStatsCalculator::GetTypeId (void)
{
  static TypeId tid =
    TypeId ("ns3::MmWaveRetxStatsCalculator")
    .SetParent<Object> ().AddConstructor<MmWaveRetxStatsCalculator> ()
    .SetGroupName("Lte")
    .AddAttribute ("DlRlcRetxFilename",
                   "Name of the file where the downlink retx results will be saved.",
                   StringValue ("DlRlcRetxStats.txt"),
                   MakeStringAccessor (&MmWaveRetxStatsCalculator::m_retxDlFilename),
                   MakeStringChecker ())
    .AddAttribute ("UlRlcRetxFilename",
                   "Name of the file where the uplink retx results will be saved.",
                   StringValue ("UlRlcRetxStats.txt"),
                   MakeStringAccessor (&MmWaveRetxStatsCalculator::m_retxUlFilename),
                   MakeStringChecker ())
    .AddAttribute ("RlcDropFilename",
                   "Name of the file where the RX RLC drop results will be saved.",
                   StringValue ("RxRlcPktDropsStats.txt"),
                   MakeStringAccessor (&MmWaveRetxStatsCalculator::m_pktDropFilename),
                   MakeStringChecker ())
    ;
  return tid;
}

void
MmWaveRetxStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);
}

void
MmWaveRetxStatsCalculator::RegisterRetxDl(uint64_t imsi, uint16_t cellId, 
	uint16_t rnti, uint8_t lcid, uint32_t packetSize, uint32_t numRetx)
{
	if(!m_retxDlFile.is_open())
	{
	    m_retxDlFile.open(m_retxDlFilename.c_str());
	    NS_LOG_LOGIC("File opened");
  	}
	m_retxDlFile << Simulator::Now().GetSeconds() << " " << cellId << " " << imsi << " "
		<< rnti << " " << (uint16_t) lcid << " " << packetSize << " " << numRetx << std::endl;
}

void
MmWaveRetxStatsCalculator::RegisterRetxUl(uint64_t imsi, uint16_t cellId, 
	uint16_t rnti, uint8_t lcid, uint32_t packetSize, uint32_t numRetx)
{
	if(!m_retxUlFile.is_open())
	{
	    m_retxUlFile.open(m_retxUlFilename.c_str());
	    NS_LOG_LOGIC("File opened");
  	}
	m_retxUlFile << Simulator::Now().GetSeconds() << " " << cellId << " " << imsi << " "
		<< rnti << " " << (uint16_t) lcid << " " << packetSize << " " << numRetx << std::endl;
}

void
MmWaveRetxStatsCalculator::RegisterRxDrop(uint64_t imsi, uint16_t cellId, 
                                          uint16_t rnti, uint8_t lcid, Ptr<const Packet> p)
{
	if(!m_pktDropFile.is_open())
	{
	    m_pktDropFile.open(m_pktDropFilename.c_str());
	    NS_LOG_LOGIC("File opened");
  	}
        LteRlcAmHeader rlcAmHeader;
        p->PeekHeader (rlcAmHeader);
        SequenceNumber10 seqNumber = rlcAmHeader.GetSequenceNumber ();
	m_pktDropFile << Simulator::Now().GetSeconds() << " " << cellId << " " << imsi << " "
                      << rnti << " " << (uint16_t) lcid << " " << seqNumber << " " << p->GetSize() << std::endl;
}

}
