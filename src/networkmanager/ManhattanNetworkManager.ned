
package src.networkmanager;

simple ManhattanNetworkManager like INetworkManager
{
    parameters:
        int numberOfVehicles = default(1);  
        int numberOfNodes; 
        string nodeTypesFile = default("../src/common/nodeTypes.txt");    
        string destNodesRequestMatchingFile = "../src/common/destNodesRequestMatching.txt";   
        @display("i=block/network2_l");
        
}
