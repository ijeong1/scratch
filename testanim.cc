#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include <ns3/flow-monitor-helper.h>
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/radio-bearer-stats-calculator.h"
#include "ns3/lte-global-pathloss-database.h"
#include "ns3/internet-module.h"
#include "ns3/global-value.h" 
#include "ns3/netanim-module.h"  
#include <iomanip>
#include <string>
#include <fstream>
#include "ns3/log.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/spectrum-module.h"
#include <ns3/buildings-helper.h>

using namespace ns3;



//ThroughputMonitor
void ThroughputMonitor (FlowMonitorHelper* fmhelper, Ptr<FlowMonitor> flowMon)
	{	
		flowMon->CheckForLostPackets(); 
		std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
		Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
		{	
			Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
			std::cout<<"Flow ID			: " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
			//std::cout<<"Tx Packets = " << stats->second.txPackets<<std::endl;
			//std::cout<<"Rx Packets = " << stats->second.rxPackets<<std::endl;
			std::cout<<"Duration		: "<<stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds()<<std::endl;
			std::cout<<"Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds()<<" Seconds"<<std::endl;
			std::cout<<"Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps"<<std::endl;
			std::cout<<"---------------------------------------------------------------------------"<<std::endl;
		}
		Simulator::Schedule(Seconds(1),&ThroughputMonitor, fmhelper, flowMon);
	}

NS_LOG_COMPONENT_DEFINE ("NetanimExample");

int main (int argc, char *argv[])
{
	LogComponentEnable ("MobilityHelper", LOG_LEVEL_INFO);
  
	double simTime = 1000.0;
	double distance = 30.0;
	double interPacketInterval = 1;
	double radius = 20;
	uint32_t numUes = 10;
	uint32_t numeNB = 1;

	// Command line arguments
	CommandLine cmd;
  
	Config::SetDefault ("ns3::ConfigStore::Filename", StringValue ("input-defaults.txt"));
	Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Load"));
	Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
	Config::SetDefault ("ns3::LteAmc::AmcModel", EnumValue (LteAmc::PiroEW2010));
	Config::SetDefault ("ns3::LteAmc::Ber", DoubleValue (0.00005));

	//The simulator provides two possible schemes for what concerns the selection of the MCSs and correspondingly the generation of the CQIs. The first one is based on the GSoC module [Piro2011] and works per RB basis. This model can be activated with the ns3 attribute system, as presented in the following:
	//Config::SetDefault ("ns3::LteAmc::AmcModel", EnumValue (LteAmc::PiroEW2010));
	//While, the solution based on the physical error model can be controlled with:
	//Config::SetDefault ("ns3::LteAmc::AmcModel", EnumValue (LteAmc::MiErrorModel));
	//Finally, the required efficiency of the PiroEW2010 AMC module can be tuned thanks to the Ber attribute (), for instance:
	//Config::SetDefault ("ns3::LteAmc::Ber", DoubleValue (0.00005));

	cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
	cmd.AddValue("distance", "Distance between eNBs [m]", distance);
	cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
	cmd.AddValue ("radius", "the radius of the disc where UEs are placed around an eNB", radius);
	cmd.AddValue ("numUes", "how many UEs are attached to each eNB", numUes);
	cmd.Parse(argc, argv);

	Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

	//Evolved Packet Core (EPC)
	Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
	lteHelper->SetEpcHelper (epcHelper);

	//Scheduler
	Ptr<PfFfMacScheduler> pfFfMacScheduler = CreateObject<PfFfMacScheduler> ();
	lteHelper->SetSchedulerType ("ns3::PfFfMacScheduler"); //PfFfMacScheduler

	//Pathloss Model
	lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::Cost231PropagationLossModel"));

	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults();

	// parse again so you can override default values from the command line
	cmd.Parse(argc, argv);


	// determine the string tag that identifies this simulation run
	// this tag is then appended to all filenames

	IntegerValue runValue;
	GlobalValue::GetValueByName ("RngRun", runValue);
	RngSeedManager::SetSeed(2);
	RngSeedManager::SetRun(7);
  
	Ptr<Node> pgw = epcHelper->GetPgwNode (); //pgateway
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
	p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
	NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
	Ipv4AddressHelper ipv4h;
	ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
	// interface 0 is localhost, 1 is the p2p device
	Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
	remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

	NodeContainer ueNodes;
	NodeContainer enbNodes;
	enbNodes.Create(numeNB);
	ueNodes.Create(numUes);
	NodeContainer allNodes = NodeContainer (enbNodes, ueNodes);
  

	// Position of  eNB
	//Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	//positionAlloc->Add (Vector (0, 0, 0.0)); // eNB 0

	// Install Mobility Model in eNB
	  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	  for (uint16_t i = 0; i < numeNB; i++)
	    {
	      Vector enbPosition (100, 100, 0);
	      positionAlloc->Add (enbPosition);
	    }
  
  
  MobilityHelper enbmobility;
  enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbmobility.SetPositionAllocator (positionAlloc);
  enbmobility.Install (enbNodes);


 // Position of UEs attached to eNB 

  MobilityHelper uemobility;

  uemobility.SetPositionAllocator ("ns3::GridPositionAllocator",
     "MinX", DoubleValue (0.0),
     "MinY", DoubleValue (0.0),
     "DeltaX", DoubleValue (5.0),
     "DeltaY", DoubleValue (10.0),
     "GridWidth", UintegerValue (3),
     "LayoutType", StringValue ("RowFirst"));

  uemobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
		    "Bounds", RectangleValue (Rectangle (-100, 100, -100, 1000)));

  uemobility.Install (ueNodes);
  /*
  uemobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");


  double min = 0.0;
  double max = 1000.0;
  for (uint16_t i = 0; i < numUes; i++)
    {
	  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
	  x->SetAttribute ("Min", DoubleValue (min));
	  x->SetAttribute ("Max", DoubleValue (max));

	  Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable> ();
	  y->SetAttribute ("Min", DoubleValue (min));
	  y->SetAttribute ("Max", DoubleValue (max));

	  double xval = x->GetValue ();
	  double yval = y->GetValue ();
	  ueNodes.Get (i)->GetObject<MobilityModel> ()->SetPosition (Vector (xval, yval, 0));
	  ueNodes.Get (i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (20, 0, 0));
	  //uemobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");

	  //ueNodes->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (20, 0, 0));
	}*/

  // Position of UEs attached to eNB 2
 
  

  
  // Install LTE Devices to the nodes
  
  NetDeviceContainer enbLteDevs;
  NetDeviceContainer ueLteDevs;
 
  enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  ueLteDevs = lteHelper->InstallUeDevice (ueNodes);


  
  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  
   // Attach all UEs to the first eNodeB
 
    for (uint16_t i = 0; i < numUes; i++)
      {
        lteHelper->Attach (ueLteDevs.Get (i),enbLteDevs.Get (0));
        // side effect: the default EPS bearer will be activated
      }
 
 //Ptr<LteEnbPhy> enb0Phy = enbLteDevs.Get (0)->GetObject<LteEnbNetDevice> ()->GetPhy ();
  //enb0Phy->SetTxPower (35);
//Ptr<LteEnbPhy> enb1Phy = enbLteDevs.Get (1)->GetObject<LteEnbNetDevice> ()->GetPhy ();
  //enb1Phy->SetTxPower (60);

  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  uint16_t otherPort = 3000;
 
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  
 
 for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
  
      ++ulPort;
      ++otherPort;

      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
      PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
      serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
      serverApps.Add (packetSinkHelper.Install (ueNodes.Get(u)));
              

      UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      dlClient.SetAttribute ("MaxPackets", UintegerValue(20000));
    
      
      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      ulClient.SetAttribute ("MaxPackets", UintegerValue(20000));
   

      UdpClientHelper client (ueIpIface.GetAddress (u), otherPort);
      client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      client.SetAttribute ("MaxPackets", UintegerValue(20000));
       
    
      clientApps.Add (dlClient.Install (remoteHost));
      clientApps.Add (ulClient.Install (ueNodes.Get(u)));
      if (u+1 < ueNodes.GetN ())
        {
          clientApps.Add (client.Install (ueNodes.Get(u+1)));
        }
      else
        {
          clientApps.Add (client.Install (ueNodes.Get(0)));
        }
        
    
    }
  serverApps.Start (Seconds (0.01));
  clientApps.Start (Seconds (0.01));
  
 
  // Uncomment to enable PCAP tracing
  //p2ph.EnablePcapAll("lena-epc-first");
  
  
  Simulator::Stop(Seconds(simTime));

 
  
  lteHelper->EnablePhyTraces ();
  lteHelper->EnableMacTraces ();
  lteHelper->EnableRlcTraces ();
  lteHelper->EnablePdcpTraces ();
  lteHelper->EnableUlMacTraces ();

  Config::SetDefault ("ns3::ConfigStore::Filename", StringValue ("output-attributes.txt"));
  Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
  Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Save"));
  ConfigStore outputConfig;
  outputConfig.ConfigureDefaults ();
  outputConfig.ConfigureAttributes ();
  
 FlowMonitorHelper flowmon;
 Ptr<FlowMonitor> monitor = flowmon.Install(ueNodes);
 monitor = flowmon.Install(remoteHost);
 monitor = flowmon.GetMonitor ();
 


  monitor->SetAttribute("DelayBinWidth", DoubleValue (0.001));

  monitor->SetAttribute("JitterBinWidth", DoubleValue (0.001));

  monitor->SetAttribute("PacketSizeBinWidth", DoubleValue (2000));

  monitor->SerializeToXmlFile("results.xml", true, true);

  

  AnimationInterface anim ("animation.xml");  // where "animation.xml" is any arbitrary filename
  
  


  Simulator::Run();

  //string xmlFileName = "flow-monitor-output.xml"
  monitor->SerializeToXmlFile ("results.xml",false,false); 

  //  Print per flow statistics
   monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      // first 2 FlowIds are for ECHO apps, we don't want to display them
      if (i->first > 0)
        {
          Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
          std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
          std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
          std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / simTime / 1024 / 1024  << " Mbps\n";
          std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
          std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
          std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / simTime / 1024 / 1024  << " Mbps\n";
          }
    }

 
//string xmlFileName = "flow-monitor-output.xml"

    ThroughputMonitor(&flowmon ,monitor);

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/
  
  // iterate our nodes and print their position.
  for (NodeContainer::Iterator j = allNodes.Begin ();
       j != allNodes.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Vector pos = position->GetPosition ();
      std::cout << "x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << std::endl;
    }

  Simulator::Destroy();
  return 0;

}
