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
#include "ns3/tcp-header.h"
#include "ns3/tcp-option-sack.h"

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

MmWaveTcpSackBuffer::MmWaveTcpSackBuffer ()
{
  NS_LOG_FUNCTION (this);
}

MmWaveTcpSackBuffer::~MmWaveTcpSackBuffer ()
{
  NS_LOG_FUNCTION (this);
}

TcpOptionSack::SackList
MmWaveTcpSackBuffer::GetSackList() const
{
  return m_sackList;
}

void
MmWaveTcpSackBuffer::MirrorSackList(TcpOptionSack::SackList sackList)
{
  m_sackList = sackList;
}

void
MmWaveTcpSackBuffer::AddSackBlock(const ns3::SequenceNumber32 &head, const ns3::SequenceNumber32 &tail)
{
  UpdateSackList(head, tail);
}

void
MmWaveTcpSackBuffer::UpdateSackList (const SequenceNumber32 &head, const SequenceNumber32 &tail)
{
  NS_LOG_FUNCTION (this << head << tail);
  TcpOptionSack::SackBlock current;
  current.first = head;
  current.second = tail;

  m_sackList.push_front(current);

  // We have inserted the block at the beginning of the list. Now, we should
  // check if any existing blocks overlap with that.
  bool updated = false;
  TcpOptionSack::SackList::iterator it = m_sackList.begin ();
  TcpOptionSack::SackBlock begin = *it;
  TcpOptionSack::SackBlock merged;
  ++it;

  // Iterates until we examined all blocks in the list (maximum 4)
  while (it != m_sackList.end ())
    {
      current = *it;

      // This is a left merge:
      // [current_first; current_second] [beg_first; beg_second]
      if (begin.first == current.second)
        {
          NS_ASSERT (current.first < begin.second);
          merged = TcpOptionSack::SackBlock (current.first, begin.second);
          updated = true;
        }
      // while this is a right merge
      // [begin_first; begin_second] [current_first; current_second]
      else if (begin.second == current.first)
        {
          NS_ASSERT (begin.first < current.second);
          merged = TcpOptionSack::SackBlock (begin.first, current.second);
          updated = true;
        }

      // If we have merged the blocks (and the result is in merged) we should
      // delete the current block (it), the first block, and insert the merged
      // one at the beginning.
      if (updated)
        {
          m_sackList.erase (it);
          m_sackList.pop_front ();
          m_sackList.push_front (merged);
          it = m_sackList.begin ();
          begin = *it;
          updated = false;
        }

      ++it;
    }

  // Since the maximum blocks that fits into a TCP header are 4, there's no
  // point on maintaining the others.
  if (m_sackList.size () > 4)
    {
      m_sackList.pop_back ();
    }
}

void
MmWaveTcpSackBuffer::ClearSackList (const SequenceNumber32 &seq)
{

}
} //namespace ns3

