#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/propagation-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

void PropagationModel(uint32_t nodeCount) {
    // Enable Logging (optional)
    LogComponentEnable("PropagationLossModel", LOG_LEVEL_INFO);

    // Create nodes
    NodeContainer nodes;
    nodes.Create(nodeCount);

    // Configure Mobility Model
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

    // Configure Propagation Model
    Ptr<LogDistancePropagationLossModel> lossModel = CreateObject<LogDistancePropagationLossModel>();
    lossModel->SetPathLossExponent(3.0);
    lossModel->SetReference(1.0, 46.6777);

    // Transmission simulation
    double txPower = 20.0; // Transmit power in dBm

    for (uint32_t i = 0; i < nodeCount - 1; ++i) {
        Ptr<MobilityModel> txMobility = nodes.Get(i)->GetObject<MobilityModel>();
        Ptr<MobilityModel> rxMobility = nodes.Get(i + 1)->GetObject<MobilityModel>();

        double rxPower = lossModel->CalcRxPower(txPower, txMobility, rxMobility);

        std::cout << "Node " << i << " sends to Node " << i + 1 << std::endl;
        std::cout << "Transmit Power: " << txPower << " dBm" << std::endl;
        std::cout << "Received Power at Node " << i + 1 << ": " << rxPower << " dBm" << std::endl;
        std::cout << "Path Loss: " << txPower - rxPower << " dB" << std::endl;
        std::cout << "-------------------------------------" << std::endl;
    }
}

int main() {
    for (uint32_t n = 2; n <= 30; ++n) {
        std::cout << "Running simulation for " << n << " nodes" << std::endl;
        PropagationModel(n);
    }
    return 0;
}
