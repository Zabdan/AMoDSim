package src.networkmanager;

moduleinterface INetworkManager
{
    parameters:
        int numberOfVehicles;
        int numberOfNodes;
        string nodeTypesFile;  
        string destNodesRequestMatchingFile;
		@display("i=block/network2_l");    
}