/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Adrian Sai-wah Tam
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

#include "mmwave-tcp-sack-buffer.h"
#include "ns3/packet.h"
#include "ns3/fatal-error.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MmWaveTcpSackBuffer");

NS_OBJECT_ENSURE_REGISTERED (MmWaveTcpSackBuffer);

TypeId
MmWaveTcpSackBuffer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MmWaveTcpSackBuffer")
    .SetParent <Object> ()
    .SetGroupName ("Lte")
    .AddConstructor <MmWaveTcpSackBuffer> ()
  ;
  return tid;
}

MmWaveTcpSackBuffer::MmWaveTcpSackBuffer (uint32_t n)
  :m_nextRxSeq(n)
{
  NS_LOG_FUNCTION (this);
}

MmWaveTcpSackBuffer::~MmWaveTcpSackBuffer ()
{
  NS_LOG_FUNCTION (this);
}

void MmWaveTcpSackBuffer::TraceNextRxSequence(ns3::SequenceNumber32 oldval, ns3::SequenceNumber32 newval)
{
  //Set the nextRxSequence as new value
  SetNextRxSequence(newval);
}

void MmWaveTcpSackBuffer::SetNextRxSequence(ns3::SequenceNumber32 nextRxSequence)
{
  m_nextRxSeq = nextRxSequence;
}
} //namespace ns3

