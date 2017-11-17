/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
 /*
 *   Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *   Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York University
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
 *
 */

#include "ns3/tag.h"
#include "ns3/point-to-point-module.h"
#include "ns3/mmwave-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include <ns3/buildings-helper.h>
#include <ns3/buildings-module.h>
#include <ns3/tcp-header.h>
#include <ns3/lte-rlc-am-header.h>

using namespace ns3;

/**
 * A script to simulate the DOWNLINK TCP data over mmWave links
 * with the mmWave devices and the LTE EPC.
 */
NS_LOG_COMPONENT_DEFINE ("mmWaveL2BOproblem");

class TimestampTag : public Tag {
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);

  // these are our accessors to our tag structure
  void SetTimestamp (Time time);
  Time GetTimestamp (void) const;

  void Print (std::ostream &os) const;

private:
  Time m_timestamp;

  // end class TimestampTag
};

//----------------------------------------------------------------------
//-- TimestampTag
//------------------------------------------------------
TypeId 
TimestampTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("TimestampTag")
    .SetParent<Tag> ()
    .AddConstructor<TimestampTag> ()
    .AddAttribute ("Timestamp",
                   "Some momentous point in time!",
                   EmptyAttributeValue (),
                   MakeTimeAccessor (&TimestampTag::GetTimestamp),
                   MakeTimeChecker ())
  ;
  return tid;
}
TypeId 
TimestampTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t 
TimestampTag::GetSerializedSize (void) const
{
  return 8;
}
void 
TimestampTag::Serialize (TagBuffer i) const
{
  int64_t t = m_timestamp.GetNanoSeconds ();
  i.Write ((const uint8_t *)&t, 8);
}
void 
TimestampTag::Deserialize (TagBuffer i)
{
  int64_t t;
  i.Read ((uint8_t *)&t, 8);
  m_timestamp = NanoSeconds (t);
}

void
TimestampTag::SetTimestamp (Time time)
{
  m_timestamp = time;
}
Time
TimestampTag::GetTimestamp (void) const
{
  return m_timestamp;
}

void 
TimestampTag::Print (std::ostream &os) const
{
  os << "t=" << m_timestamp;
}

class MyApp : public Application
{
public:

  MyApp ();
  virtual ~MyApp();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  static int send_num = 1;
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  TimestampTag tag;
  tag.SetTimestamp(Simulator::Now());
  packet->AddByteTag(tag);
  m_socket->Send (packet);
  NS_LOG_DEBUG ("Sending:    "<<send_num++ << "\t" << Simulator::Now ().GetSeconds ());

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
	*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}


static void
RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
{
	*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldRtt.GetSeconds () << "\t" << newRtt.GetSeconds () << std::endl;
}

static void Rx (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> packet, const Address &from)
{
  TimestampTag tag;
  if(packet->FindFirstMatchingByteTag(tag))
    {
      Time initial(tag.GetTimestamp());
      Time curr(Simulator::Now());
      Time delay(curr - initial);
      //TODO: Add delay to stream output
      *stream->GetStream () << curr.GetSeconds () << "," << delay << "," << packet->GetSize() << std::endl;      
    }
}

static void RlcRx (Ptr<OutputStreamWrapper> stream, uint16_t rnti, uint8_t lcid, Ptr<const Packet> packet, uint64_t delay)
{
  LteRlcAmHeader rlcHeader;
  TcpHeader tcpHeader;
  packet->PeekHeader(rlcHeader);
  packet->PeekHeader(tcpHeader);
}

static void OnTcpRTO (Ptr<OutputStreamWrapper> stream, Time oldval, Time newval)
{
  *stream->GetStream() << (Simulator::Now ()).GetSeconds () << " " << oldval << " " << newval << std::endl;
}

void
ChangeSpeed(Ptr<Node>  n, Vector speed)
{
	n->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (speed);
}

//Scenario: 6 random located small building, simulate tree and human blockage
int main (int argc, char* argv[])
{
  double stopTime = 50;
  double simStopTime = 50;

  CommandLine cmd;
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simStopTime);
  cmd.Parse(argc, argv);

  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1400));
  Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (MilliSeconds(5)));
  Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (1024 * 1024));
  Config::SetDefault ("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue(true));
  Config::SetDefault ("ns3::MmWaveHelper::HarqEnabled", BooleanValue(true));
  Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue(true));
  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::HarqEnabled", BooleanValue(true));
  Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue(true));
  Config::SetDefault ("ns3::MmWaveBeamforming::LongTermUpdatePeriod", TimeValue (MilliSeconds (100.0)));
  Config::SetDefault ("ns3::LteRlcAm::PollRetransmitTimer", TimeValue(MilliSeconds(4.0)));
  Config::SetDefault ("ns3::LteRlcAm::ReorderingTimer", TimeValue(MilliSeconds(2.0)));
  Config::SetDefault ("ns3::LteRlcAm::StatusProhibitTimer", TimeValue(MilliSeconds(1.0)));
  Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MilliSeconds(4.0)));
  
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));

  Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper> ();

  mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::BuildingsObstaclePropagationLossModel"));
  mmwaveHelper->Initialize();
  mmwaveHelper->SetHarqEnabled(true);

  Ptr<MmWavePointToPointEpcHelper>  epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
  mmwaveHelper->SetEpcHelper (epcHelper);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds(50)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr;
  remoteHostAddr = internetIpIfaces.GetAddress (1);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
  
  Ptr<Building> building1;
  building1 = Create<Building> ();
  building1->SetBoundaries (Box(69.5,71.0,4.5,6.0,0.0,2.5));

  Ptr<Building> building2;
  building2 = Create<Building> ();
  building2->SetBoundaries (Box(60.0,61.5,9.5,12.0,0.0,2.5));

  Ptr<Building> building3;
  building3 = Create<Building> ();
  building3->SetBoundaries (Box(54.0,55.5,6.0,8.5,0.0,2.5));
  
  Ptr<Building> building4;
  building4 = Create<Building> ();
  building4->SetBoundaries (Box(60.0,61.5,6.0,8.5,0.0,2.5));

  Ptr<Building> building5;
  building5 = Create<Building> ();
  building5->SetBoundaries (Box(70.0,71.5,0.0,2.5,0.0,2.5));

  Ptr<Building> building6;
  building6 = Create<Building> ();
  building6->SetBoundaries (Box(50.0,51.5,4.0,6.5,0.0,2.5));

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(1);
  ueNodes.Create(1);

  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  enbPositionAlloc->Add (Vector (0.0, 0.0, 3.0));
  MobilityHelper enbmobility;
  enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbmobility.SetPositionAllocator(enbPositionAlloc);
  enbmobility.Install (enbNodes);
  BuildingsHelper::Install (enbNodes);

  MobilityHelper uemobility;
  uemobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  uemobility.Install (ueNodes);

  ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (150, -0.2, 1));
  ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (0, 0, 0));

  Simulator::Schedule (Seconds (2), &ChangeSpeed, ueNodes.Get (0), Vector (0, 1.5, 0));
  Simulator::Schedule (Seconds (23), &ChangeSpeed, ueNodes.Get (0), Vector (0, 0.3, 0));

  BuildingsHelper::Install (ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbDevs = mmwaveHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueDevs = mmwaveHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  // Assign IP address to UEs, and install applications
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

  mmwaveHelper->AttachToClosestEnb (ueDevs, enbDevs);
  mmwaveHelper->EnableTraces ();

  // Set the default gateway for the UE
  Ptr<Node> ueNode = ueNodes.Get (0);
  Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
  ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

  // Install and start applications on UEs and remote host
  uint16_t sinkPort = 20000;

  Address sinkAddress (InetSocketAddress (ueIpIface.GetAddress (0), sinkPort));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (ueNodes.Get (0));

  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (simStopTime));

  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (remoteHostContainer.Get (0), TcpSocketFactory::GetTypeId ());
  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 1400, 50000000, DataRate ("1000Mb/s"));

  remoteHostContainer.Get (0)->AddApplication (app);
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-window-newreno.txt");
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream1));

  Ptr<OutputStreamWrapper> stream4 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-rtt-newreno.txt");
  ns3TcpSocket->TraceConnectWithoutContext ("RTT", MakeBoundCallback (&RttChange, stream4));

  Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-data-newreno.txt");
  sinkApps.Get(0)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream2));

  Ptr<OutputStreamWrapper> stream3 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-rtoevents.txt");
  ns3TcpSocket->TraceConnectWithoutContext ("RTO", MakeBoundCallback (&OnTcpRTO, stream3));

  Ptr<OutputStreamWrapper> stream5 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-data-newreno.txt");
  Config::Connect("/NodeList/*/DeviceList/*/MmWaveUeRrc/*/DataRadioBearerMap/*/LteRlc/$ns3::LteRlcAm/RxPacket", MakeBoundCallback(&RlcRx, stream5));  
  app->SetStartTime (Seconds (0.1));
  app->SetStopTime (Seconds (stopTime));

  p2ph.EnablePcapAll("mmwave-sgi-capture");
  
  BuildingsHelper::MakeMobilityModelConsistent ();  
  Simulator::Stop (Seconds (simStopTime));
  Simulator::Run ();
  Simulator::Destroy ();
  
  return 0;
}
