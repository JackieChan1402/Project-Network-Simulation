#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiAdhocSimulation");

int main(int argc, char *argv[]) {
    uint32_t packetSize = 1024;
    uint32_t numPackets = 100;
    double simulationTime = 10.0;
    double interval = simulationTime / numPackets;

    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2000"));
    Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2200")); // Setting Fragmentation Threshold

    for (uint32_t numNodes = 2; numNodes <= 30; numNodes += 2) {
        std::cout << "Running simulation with " << numNodes << " nodes" << std::endl;

        NodeContainer nodes;
        nodes.Create(numNodes);

        WifiHelper wifi;
        wifi.SetStandard(WIFI_STANDARD_80211b);
        wifi.SetRemoteStationManager("ns3::AarfWifiManager"); // Using AARF Rate Control Algorithm

        YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
        YansWifiPhyHelper phy;
        phy.SetChannel(channel.Create());

        WifiMacHelper mac;
        mac.SetType("ns3::AdhocWifiMac");

        NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

        MobilityHelper mobility;
        mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                      "MinX", DoubleValue(0.0),
                                      "MinY", DoubleValue(0.0),
                                      "DeltaX", DoubleValue(5.0),
                                      "DeltaY", DoubleValue(5.0),
                                      "GridWidth", UintegerValue(5),
                                      "LayoutType", StringValue("RowFirst"));
        mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
        mobility.Install(nodes);

        InternetStackHelper internet;
        internet.Install(nodes);

        Ipv4AddressHelper ipv4;
        ipv4.SetBase("192.168.1.0", "255.255.255.0");
        Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

        UdpEchoServerHelper echoServer(9);
        ApplicationContainer serverApps = echoServer.Install(nodes.Get(0));
        serverApps.Start(Seconds(1.0));
        serverApps.Stop(Seconds(simulationTime + 2));

        UdpEchoClientHelper echoClient(interfaces.GetAddress(0), 9);
        echoClient.SetAttribute("MaxPackets", UintegerValue(numPackets));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(interval)));
        echoClient.SetAttribute("PacketSize", UintegerValue(packetSize));

        ApplicationContainer clientApps = echoClient.Install(nodes.Get(numNodes - 1));
        clientApps.Start(Seconds(2.0));
        clientApps.Stop(Seconds(simulationTime + 2));

        FlowMonitorHelper flowmon;
        Ptr<FlowMonitor> monitor = flowmon.InstallAll();

        Simulator::Stop(Seconds(simulationTime + 2));
        Simulator::Run();

        monitor->CheckForLostPackets();
        Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
        FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

        double totalTxPackets = 0, totalRxPackets = 0, totalThroughput = 0, totalDelay = 0;

        for (auto &flow : stats) {
            totalTxPackets += flow.second.txPackets;
            totalRxPackets += flow.second.rxPackets;
            totalThroughput += (flow.second.rxBytes * 8.0 / simulationTime) / 1e6; // Mbps
            totalDelay += flow.second.delaySum.GetSeconds();
        }

        double packetLoss = ((totalTxPackets - totalRxPackets) / totalTxPackets) * 100;
        double avgDelay = (totalRxPackets > 0) ? (totalDelay / totalRxPackets) * 1000 : 0; // ms

        std::cout << "Throughput: " << totalThroughput << " Mbps\n";
        std::cout << "Packet Loss: " << packetLoss << " %\n";
        std::cout << "Average Delay: " << avgDelay << " ms\n";

        Simulator::Destroy();
    }
    return 0;
}
