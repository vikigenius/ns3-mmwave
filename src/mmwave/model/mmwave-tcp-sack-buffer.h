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
#ifndef MMWAVE_TCP_SACK_BUFFER_H
#define MMWAVE_TCP_SACK_BUFFER_H

#include <map>
#include <ns3/ptr.h>
#include <ns3/object.h>
#include <ns3/tcp-header.h>
#include <ns3/tcp-option-sack.h>
#include <ns3/packet.h>

namespace ns3{
class MmWaveTcpSackBuffer : public Object
{
public:
  
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Constructor
   * \param n the initial size of buffer
   */
  MmWaveTcpSackBuffer (void);

  /**
   * \brief Virtual Destructor
   */
  virtual ~MmWaveTcpSackBuffer ();

  /*
   * \brief Mirror the contents of the provided sack list
   * \param sackList The sacklist to mirror
   */
  void
  MirrorSackList (TcpOptionSack::SackList sackList);
  
private:
  /**
   * \brief Update the sack list, with the block seq starting at the beginning
   *
   * Note: the maximum size of the block list is 4. Caller is free to
   * drop blocks at the end to accomodate header size; from RFC 2018:
   *
   * > The data receiver SHOULD include as many distinct SACK blocks as
   * > possible in the SACK option.  Note that the maximum available
   * > option space may not be sufficient to report all blocks present in
   * > the receiver's queue.
   *
   * In fact, the maximum amount of blocks is 4, and if we consider the timestamp
   * (or other) options, it is even less. For more detail about this function,
   * please see the source code and in-line comments.
   *
   * \param head sequence number of the block at the beginning
   * \param tail sequence number of the block at the end
   */
  void UpdateSackList (const SequenceNumber32 &head, const SequenceNumber32 &tail);

   /**
   * \brief Remove old blocks from the sack list
   *
   * Used to remove blocks already delivered to the application.
   *
   * After this call, in the SACK list there will be only blocks with
   * sequence numbers greater than seq; it is perfectly safe to call this
   * function with an empty sack list.
   *
   * \param seq Last sequence to remove
   */
  void ClearSackList (const SequenceNumber32 &seq);

  TcpOptionSack::SackList m_sackList; //!< Sack list (updated constantly)

  /// container for data stored in the buffer
  typedef std::map<SequenceNumber32, Ptr<Packet> >::iterator BufIterator;
};
}

#endif // MMWAVE_TCP_SACK_BUFFER_H
