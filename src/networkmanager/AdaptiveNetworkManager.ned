
package src.networkmanager;

simple AdaptiveNetworkManager like INetworkManager
{
     parameters:
        int numberOfVehicles = default(1);
        int numberOfNodes;        
        bool onlineRouting = default(false);
        string nodeTypesFile = default("../src/common/nodeTypes.txt");   
        string destNodesRequestMatchingFile = "../src/common/destNodesRequestMatching.txt"; 
        @display("i=block/network2_l");      
}
