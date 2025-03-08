#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiAdhocSimulation");

int main(int argc, char *argv[]) {
    uint32_t packetSize = 1024;
    uint32_t numPackets = 100;
    double simulationTime = 10.0;
    double interval = simulationTime / numPackets;

    for (uint32_t numNodes = 2; numNodes <= 30; numNodes+=2) {
        std::cout << "Running simulation with " << numNodes << " nodes" << std::endl;

        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

        NodeContainer nodes;
        nodes.Create(numNodes);

        WifiHelper wifi;
        wifi.SetStandard(WIFI_STANDARD_80211b);
        YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
        YansWifiPhyHelper phy;
        phy.SetChannel(channel.Create());
        phy.Set("RtsCtsThreshold", UintegerValue(2348)); // turn off RTS/CTS

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
                                      // grid layout
        mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
                             // random walk
        mobility.Install(nodes);

        InternetStackHelper internet;
        internet.Install(nodes);

        Ipv4AddressHelper ipv4;
        ipv4.SetBase("192.168.1.0", "255.255.255.0");
        Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

        UdpEchoServerHelper echoServer(9);
        ApplicationContainer serverApps = echoServer.Install(nodes.Get(0));
        serverApps.Start(Seconds(1.0));
        serverApps.Stop(Seconds(simulationTime+2));

        UdpEchoClientHelper echoClient(interfaces.GetAddress(0), 9);
        echoClient.SetAttribute("MaxPackets", UintegerValue(numPackets));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(interval)));
        echoClient.SetAttribute("PacketSize", UintegerValue(packetSize));

        ApplicationContainer clientApps = echoClient.Install(nodes.Get(numNodes - 1));
        clientApps.Start(Seconds(2.0));
        clientApps.Stop(Seconds(simulationTime+2));

        std::ostringstream pcapFileName;
        pcapFileName << "wifi-adhoc-" << numNodes << "nodes";
        phy.EnablePcapAll(pcapFileName.str());

        Simulator::Stop(Seconds(simulationTime+2));
        Simulator::Run();
        Simulator::Destroy();
    }
    return 0;
}
