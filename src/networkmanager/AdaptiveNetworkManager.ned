
package src.networkmanager;

simple AdaptiveNetworkManager like INetworkManager
{
     parameters:
        
        int numberOfVehiclesType0 = default(0);
        int numberOfVehiclesType1 = default(1);
        int numberOfVehicles = default(numberOfVehiclesType0+numberOfVehiclesType1);
        int numberOfNodes;      
        int dropOffNodeDist = default(4);  
        bool onlineRouting = default(false);
        string nodeTypesFile = default("../src/common/nodeTypes.txt");   
        string destNodesRequestMatchingFile = "../src/common/destNodesRequestMatching.txt"; 
        string riskLevelsFile = "../src/common/riskLevels.txt"; 
        string riskDelIncrFactFile = "../src/common/riskLevelIncrDelMatching.txt"; 
        @display("i=block/network2_l");      
}
