#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

/**
@Params:
    - txPower: 
        + represents the transmission power of a wireless node
        + It determines how strong the radio signal is when being transmitted
        + A higher transmission power allows signals to travel further and penetrate obstacles better.
 */

// Function to calculate packet delivery ratio
double CalculatePDR(uint64_t received, uint64_t sent) {
  if (sent == 0) return 0.0;
  return (double)received / sent;
}

int main(int argc, char* argv[]) {
  // Simulation parameters
  uint32_t minNodes = 2;
  uint32_t maxNodes = 30;
  uint32_t packetSize = 1024;  // bytes
  uint32_t numPackets = 1000;
  double interval = 1.0;       // seconds
  bool enableRtsCts = false;   // disable RTC/CTS
  double txPower = 16.0;       // dBm
  uint32_t rtsThreshold = 65535;
  double simTime = 10;      // seconds
  bool tracing = false;

  // Allow command line modification of parameters
  CommandLine cmd(__FILE__);
  cmd.AddValue("minNodes", "Minimum number of nodes", minNodes);
  cmd.AddValue("maxNodes", "Maximum number of nodes", maxNodes);
  cmd.AddValue("packetSize", "Packet size in bytes", packetSize);
  cmd.AddValue("simTime", "Simulation time in seconds", simTime);
  cmd.AddValue("tracing", "Enable pcap tracing", tracing);
  cmd.Parse(argc, argv);

  // Output file for results
  std::ofstream outFile;
  outFile.open("wifi-adhoc-csma-ca-results.csv");
  outFile << "Nodes,Throughput,PDR,Delay,Collisions" << std::endl;

  // Run simulation for different node counts
  for (uint32_t nNodes = minNodes; nNodes <= maxNodes; nNodes++) {
    std::cout << "Simulating with " << nNodes << " nodes..." << std::endl;

    // Create nodes
    NodeContainer adhocNodes;
    adhocNodes.Create(nNodes);

    // Set up Wi-Fi network
    /* WIFI_STANDARD_80211g properties:
        Frequency Band: 2.4 GHz
        Max Data Rate: 	54 Mbps 
    */
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211g);

    // Set up physical layer
    // models the physical layer (radio transmission).
    YansWifiPhyHelper wifiPhy;
    // defines the wireless channel (signal propagation).
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());
    // Wi-Fi signal strength and range.
    wifiPhy.Set("TxPowerStart", DoubleValue(txPower));
    wifiPhy.Set("TxPowerEnd", DoubleValue(txPower));

    // Configure MAC layer - Ad-hoc mode with CSMA/CA, no RTS/CTS
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    
    // Configure RTS threshold to disable RTS/CTS
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(rtsThreshold));
    
    // Set up station manager
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", 
                                "DataMode", StringValue("ErpOfdmRate24Mbps"),
                                "ControlMode", StringValue("ErpOfdmRate24Mbps"));

    // Install Wi-Fi on nodes
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, adhocNodes);

    // Set node positions - random positions within 100m x 100m area
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                 "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=100.0]"),
                                 "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=100.0]"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(adhocNodes);

    // Install internet stack
    InternetStackHelper internet;
    internet.Install(adhocNodes);

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    // Set up applications - each node sends to random destination
    uint16_t port = 50000;
    ApplicationContainer sinkApps;
    ApplicationContainer sourceApps;
    
    // Set up packet sinks on all nodes
    for (uint32_t i = 0; i < nNodes; i++) {
      PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
      sinkApps.Add(sink.Install(adhocNodes.Get(i)));
    }
    sinkApps.Start(Seconds(1.0));
    sinkApps.Stop(Seconds(simTime));
    
    // Set up sending applications
    for (uint32_t i = 0; i < nNodes; i++) {
      // Each node sends to a random destination
      uint32_t dest = i;
      while (dest == i) {
        dest = rand() % nNodes;
      }
      
      OnOffHelper source("ns3::UdpSocketFactory", 
                         InetSocketAddress(interfaces.GetAddress(dest), port));
      source.SetAttribute("PacketSize", UintegerValue(packetSize));
      source.SetAttribute("DataRate", DataRateValue(DataRate("1Mbps")));
      source.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
      source.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
      
      sourceApps.Add(source.Install(adhocNodes.Get(i)));
    }
    
    sourceApps.Start(Seconds(2.0));
    sourceApps.Stop(Seconds(simTime - 1.0));

    // Install FlowMonitor to collect statistics
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // Enable pcap traces if requested
    if (tracing) {
      wifiPhy.EnablePcap("wifi-adhoc-csma-ca", devices);
    }

    // Run the simulation
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // Collect and process statistics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double totalThroughput = 0.0;
    uint64_t totalPacketsReceived = 0;
    uint64_t totalPacketsSent = 0;
    uint64_t totalCollisions = 0;  // This would need a custom counter in real implementation
    double totalDelay = 0.0;
    uint32_t flowCount = 0;

    for (auto it = stats.begin(); it != stats.end(); ++it) {
      totalPacketsReceived += it->second.rxPackets;
      totalPacketsSent += it->second.txPackets;
      
      // Calculate throughput in Mbps
      double duration = (it->second.timeLastRxPacket - it->second.timeFirstTxPacket).GetSeconds();
      if (duration > 0) {
        double throughput = (it->second.rxBytes * 8.0) / (duration * 1000000);
        totalThroughput += throughput;
      }
      
      // Calculate delay in ms
      if (it->second.rxPackets > 0) {
        totalDelay += (it->second.delaySum.GetSeconds() * 1000) / it->second.rxPackets;
      }
      
      flowCount++;
    }

    // Average metrics
    double avgThroughput = totalThroughput;
    double pdr = CalculatePDR(totalPacketsReceived, totalPacketsSent);
    double avgDelay = (flowCount > 0) ? totalDelay / flowCount : 0;
    
    // This is a simplified collision estimation - in a real simulation we would need
    // to implement proper tracking of collisions
    double collisionEstimate = totalPacketsSent - totalPacketsReceived;
    
    // Output results to file
    outFile << nNodes << "," 
           << avgThroughput << "," 
           << pdr << "," 
           << avgDelay << "," 
           << collisionEstimate << std::endl;
    
    // Print summary
    std::cout << "Nodes: " << nNodes << std::endl;
    std::cout << "Aggregate Throughput: " << avgThroughput << " Mbps" << std::endl;
    std::cout << "Packet Delivery Ratio: " << pdr << std::endl;
    std::cout << "Average Delay: " << avgDelay << " ms" << std::endl;
    std::cout << "Estimated Collisions: " << collisionEstimate << std::endl;
    std::cout << "----------------------------------" << std::endl;
    
    // Reset for next iteration
    Simulator::Destroy();
  }

  outFile.close();
  return 0;
}