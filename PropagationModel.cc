#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/propagation-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

int PropagationModel() {
    // Enable Logging (optional)
    LogComponentEnable("PropagationLossModel", LOG_LEVEL_INFO);


    // Create nodes
    NodeContainer nodes;
    nodes.Create(2);  // Two nodes

    // Configure Mobility Model
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
    mobility.Install(nodes);

    // Configure Propagation Model
    Ptr<LogDistancePropagationLossModel> lossModel = CreateObject<LogDistancePropagationLossModel>();
    lossModel->SetPathLossExponent(3.0);  // Default is 3.0 (customizable)
    lossModel->SetReference(1.0, 46.6777);  // Reference loss at 1m (dB)

    // Get node mobility models
    Ptr<MobilityModel> txMobility = nodes.Get(0)->GetObject<MobilityModel>();
    Ptr<MobilityModel> rxMobility = nodes.Get(1)->GetObject<MobilityModel>();

    // Calculate received power
    double txPower = 20.0;  // Transmit power in dBm
    double rxPower = lossModel->CalcRxPower(txPower, txMobility, rxMobility);

    // Print results
    std::cout << "Transmit Power: " << txPower << " dBm" << std::endl;
    std::cout << "Received Power: " << rxPower << " dBm" << std::endl;
    std::cout << "Path Loss: " << txPower - rxPower << " dB" << std::endl;

    return 0;
    
}
int main(){
    PropagationModel();
}
