/*
########################################################
##           __  __       _____   _____ _             ##
##     /\   |  \/  |     |  __ \ / ____(_)            ##
##    /  \  | \  / | ___ | |  | | (___  _ _ __ ___    ##
##   / /\ \ | |\/| |/ _ \| |  | |\___ \| | '_ ` _ \   ##
##  / ____ \| |  | | (_) | |__| |____) | | | | | | |  ##
## /_/    \_\_|  |_|\___/|_____/|_____/|_|_| |_| |_|  ##
##                                                    ##
## Author:                                            ##
##    Andrea Di Maria                                 ##
##    <andrea.dimaria90@gmail.com>                    ##
########################################################
*/

#include "ManhattanNetworkManager.h"

Define_Module(ManhattanNetworkManager);

void ManhattanNetworkManager::initialize()
{
    cModule* parentModule = getParentModule();
    rows = parentModule->par("width");
    columns = parentModule->par("height");

    numberOfVehicles = par("numberOfVehicles");
    numberOfNodes = par("numberOfNodes");

    for(int i=0; i<numberOfVehicles; i++)
        vehiclesPerNode[intuniform(0, numberOfNodes-1, 4)]+=1;

    xChannelLength = parentModule->par("xNodeDistance");
    yChannelLength = parentModule->par("yNodeDistance");

    xTravelTime = parentModule->par("xTravelTime");
    yTravelTime = parentModule->par("yTravelTime");

    additionalTravelTime = setAdditionalTravelTime(parentModule->par("speed"), parentModule->par("acceleration"));
    setDropOffNodes();

    EV<<"SPEEEED "<<parentModule->par("speed").doubleValue()<<endl;
}

/**
 * Return the space distance from current node to target one.
 *
 * @param srcAddr
 * @param dstAddress
 * @return
 */
double ManhattanNetworkManager::getSpaceDistance(int srcAddr, int dstAddr)
{
    cTopology::Node *n = calculatePath( srcAddr,  dstAddr);
    double spaceDistance = 0.0;
    cTopology::LinkOut *linkOut = n->getPath(0);
    while(linkOut!= NULL) {
       // EV<<"DEST NODE IS "<<linkOut->getRemoteNode()->getModule()->par("address").doubleValue()<<endl;
        spaceDistance += linkOut->getLocalGate()->getChannel()->par("length").doubleValue();
        linkOut = linkOut->getRemoteNode()->getPath(0);
    }



       return spaceDistance;



}







/**
 * Return the time distance from current node to target one.
 *
 * @param dstAddress
 * @return
 */

double ManhattanNetworkManager::getTimeDistance(int srcAddr, int dstAddr)
{

    double timeDistance = 0.0;

    cTopology::Node *n = calculatePath( srcAddr,  dstAddr);
    cTopology::LinkOut *linkOut = n->getPath(0);

    while(linkOut!= NULL) {
       // EV<<"DEST NODE IS "<<linkOut->getRemoteNode()->getModule()->par("address").doubleValue()<<endl;
        timeDistance += linkOut->getLocalGate()->getChannel()->par("delay").doubleValue();
        linkOut = linkOut->getRemoteNode()->getPath(0);
    }







           // while(i<dist) {
              //  if(i == dist )
                 //   EV <<"DEST NODE"<<n->getModule()->par("address").doubleValue()<<endl;
                    //       cTopology::LinkOut *linkOut = n->getPath(0);
                   //       if(linkOut != NULL) {
                  //         timeDistance += linkOut->getLocalGate()->getChannel()->par("delay").doubleValue();


                           //        length = linkOut->getLocalGate()->getChannel()->par("length").doubleValue();
                                //   i += length;
                                  // i = i +length;
                                //   EV<<"Space Distance "<<timeDistance<<" Length increased to "<<i<<endl;
                       //         }

        /*   else {
                break;
                       }
                          n = linkOut->getRemoteNode();

                           i++;
            }*/

           timeDistance += additionalTravelTime;


           EV<<"TIME DIST TO TARGET FOR NODE "<<n->getModuleId()<<" "<<timeDistance<<endl;


    return timeDistance;



}









cTopology::Node *ManhattanNetworkManager::calculatePath(int srcAddr, int destAddr) {


    cTopology *topo = new cTopology("graph");
        std::vector<std::string> nedTypes;
        nedTypes.push_back("src.node.Node");
        topo->extractByNedTypeName(nedTypes);

        setWeight(topo);
        cModule *startNode = getNodeFromAddress(srcAddr);
        cModule *destNode = getNodeFromAddress(destAddr);
        cTopology::Node *source;
        if(destNode != NULL) {
        for(int i = 0; i<topo->getNumNodes(); i++) {
            cTopology::Node *n = topo->getNode(i);
         //   EV<<"CHECK NODE"<<endl;
            if(destNode->par("address").doubleValue() == n->getModule()->par("address").doubleValue()) {
          //      EV<<"FOUNDED DEST NODE"<<endl;
                 topo->calculateWeightedSingleShortestPathsTo(n);

              }
            if(startNode->par("address").doubleValue() == n->getModule()->par("address").doubleValue()) {
                source = n;
             //   cTopology::Node *next = source;

            }



        }


    }
    return source;
}



/**
 * Return the vehicles started from nodeAddr.
 *
 * @param nodeAddr
 * @return
 */
int ManhattanNetworkManager::getVehiclesPerNode(int nodeAddr)
{
    int nVehicles = 0;
    std::map<int,int>::iterator it;

    it = vehiclesPerNode.find(nodeAddr);
    if (it != vehiclesPerNode.end())
       nVehicles = it->second;

    return nVehicles;
}


/**
 * Check if the specified address is valid.
 *
 * @param dstAddress
 * @return
 */
bool ManhattanNetworkManager::isValidAddress(int nodeAddr)
{
    if(nodeAddr >= 0 && nodeAddr < numberOfNodes)
        return true;
    return false;
}



bool ManhattanNetworkManager::isValidDestinationAddress(int requestTypeId,int destAddr) {
    std::map<int, int> *nRMatch(new std::map<int, int>());
       readAlldestNodesRequestsMatching(nRMatch, par("destNodesRequestMatchingFile").stringValue());
       int nodeTypeId;
       for(std::map<int, int>::iterator it = nRMatch->begin(); it != nRMatch->end(); it++) {
           if(it->first == requestTypeId) {
               nodeTypeId = (*it).second;
            //   return true;
           }
       }
     //  EV<<"Request type id check nodeT"<<nodeTypeId<<"req id"<<requestTypeId<<endl;
       std::vector<cModule *> nodes = getAllDestinationNodes(nodeTypeId);
       for(std::vector<cModule *>::iterator it = nodes.begin(); it != nodes.end(); it++) {
        //   EV<<"Address check  nodeT"<<(*it)->par("address").longValue()<<"destAddr"<<destAddr<<endl;
           int addr = (*it)->par("address").longValue();
           if(addr == destAddr)
               return true;
       }

  return false;
}






bool ManhattanNetworkManager::isValidDestinationAddress(int destAddr) {
    std::vector<std::pair<int,int>> coords((columns-2)*2 + rows*2);
    std::vector<std::pair<int,int>>::iterator it;

    for(int i= 0; i<rows; i++) {
           coords.push_back({0,i});
           coords.push_back({columns-1, i});
       }

    for(int i = 1; i<columns-1; i++) {
        coords.push_back({i, 0});
        coords.push_back({i, rows-1});
    }


    int xDest = destAddr % rows;
    int yDest = destAddr / rows;


    for(it = coords.begin(); it != coords.end(); it++) {
        if((*it).first == xDest && (*it).second == yDest)
            return true;

    }


   return false;

}

/*
 *
 * Get risk zone nodes
 */

std::vector<std::pair<int,int>> * ManhattanNetworkManager::getCenteredSquare(int mult) {
  std::vector<std::pair<int,int>> *coords;
  int dim = this->getParentModule()->par("width").doubleValue();
 // EV<<"DIM"<<dim<<endl;
  int expansion = 0;
  if(dim > 4) {
  int par = dim % 2;
  if(par == 0 ) {
      int startN = dim/2-1;
      int endN = dim/2;
      if(mult > 0 && mult < dim/2-1 )
          expansion = mult;
      coords = new std::vector<std::pair<int,int>>(0);
      for(int i=startN-expansion; i<=endN + expansion; i++) {
          for(int j=startN-expansion; j<=endN + expansion; j++) {
              coords->push_back({i,j});
          }
      }

  }
  else {
      int startN = ((dim+1)/2)-2;
      int endN = startN+2;

      if(mult > 0 && mult < ((dim-1)/2)-1 )
          expansion = mult;
      coords = new std::vector<std::pair<int,int> >(0);
      for(int i=startN-expansion; i<=endN + expansion; i++) {
               for(int j=startN-expansion; j<=endN + expansion; j++) {
                   coords->push_back({i,j});
               }
           }
  }

  }



return coords;


}



/*

int ManhattanNetworkManager::getValidDestinationAddress(int requestTypeId) {
    std::map<int, int> *nRMatch(new std::map<int, int>());
    readAlldestNodesRequestsMatching(nRMatch, par("destNodesRequestMatchingFile").stringValue() );
    int nodeTypeId;
    for(std::map<int, int>::iterator it = nRMatch->begin(); it != nRMatch->end(); it++) {
        if((*it).first == requestTypeId) {
            nodeTypeId = (*it).second;
        }
    }
    std::vector<cModule *> nodes = getAllDestinationNodes(nodeTypeId);
    std::vector<cModule *>::iterator it = nodes->begin();
    if(nodes.size() > 1) {
        int num = intuniform(0, nodes.size()-1);
        it+= num;
        return (*it)->par("address").longValue();

       }
    else
        return (*it)->par("address").longValue();

}

*/

int ManhattanNetworkManager::getCloserValidDestinationAddress(int srcAddress, int requestTypeId) {
    std::map<int, int> *nRMatch(new std::map<int, int>());
    readAlldestNodesRequestsMatching(nRMatch, par("destNodesRequestMatchingFile").stringValue() );
    int nodeTypeId;
    for(std::map<int, int>::iterator it = nRMatch->begin(); it != nRMatch->end(); it++) {
        if((*it).first == requestTypeId) {
            nodeTypeId = (*it).second;
        }
    }
    std::vector<cModule *> nodes = getAllDestinationNodes(nodeTypeId);
    std::vector<cModule *>::iterator it = nodes.begin();

    cModule *nodeCloser;
    int dist;
    int addr;
    int i=0;

    while(it!= nodes.end()) {

        int distT = getTimeDistance(srcAddress, (*it)->par("address").doubleValue());


        if(i == 0) {
            dist = distT;
            nodeCloser = *it;
        }
        else {
            if(dist > distT) {
                dist = distT;
               nodeCloser = *it;
            }
        }
        i++;
        it++;
    }
 //   if(n != NULL)
  //  EV<<"ADRRRRRRRr"<<n->getId()<<endl;
    return nodeCloser->par("address").doubleValue();

   //   return 1;

}







/**
 * Return the outputGate index.
 *
 * @param dstAddress
 * @return
 */
int ManhattanNetworkManager::getOutputGate(int srcAddr, int destAddr)
{
    cTopology::Node *n = calculatePath(srcAddr, destAddr);
      return n->getPath(0)->getLocalGate()->getIndex();
}






void ManhattanNetworkManager::setWeight(cTopology *topo) {

    for(int i = 0; i<topo->getNumNodes(); i++) {

        cTopology::Node *n = topo->getNode(i);

        for(int j = 0; j<n->getNumOutLinks(); j++) {
            cTopology::LinkOut *l = n->getLinkOut(j);
            double riskLevel = l->getLocalGate()->getTransmissionChannel()->par("riskLevel").doubleValue();
            double lenght = l->getLocalGate()->getTransmissionChannel()->par("length").doubleValue();
            if(riskLevel > 0)
                l->setWeight(lenght*5);
            else
                l->setWeight(lenght);
        }
    }

}







std::vector<cModule *>  ManhattanNetworkManager::getAllDestinationNodes(int nodeTypeId) {

    std::vector<cModule *> destNodes(0);


       for(int i= 0; i<rows; i++) {
              cModule *node = getNodeFromCoords(0,i);
              if(node->par("typeId").longValue() == nodeTypeId)
                destNodes.push_back(node);
              node = getNodeFromCoords(columns-1, i);
              if(node->par("typeId").longValue() == nodeTypeId)
                destNodes.push_back(node);
          }

       for(int i = 1; i<columns-1; i++) {
           cModule *node = getNodeFromCoords(i,0);
           if(node->par("typeId").longValue() == nodeTypeId)
             destNodes.push_back(node);
           node = getNodeFromCoords(i, rows-1);
           if(node->par("typeId").longValue() == nodeTypeId)
              destNodes.push_back(node);
       }

       return destNodes;

}





void ManhattanNetworkManager::setDropOffNodes() {
    std::map<int, std::string> *nodeTypes(new std::map<int, std::string>());
    readAllNodeTypes(nodeTypes, par("nodeTypesFile").stringValue());
    //EV<<"Nodes readed: "<<nodeTypes->size()<<endl;

    std::vector<std::pair<int,int>> coords(0);
    //EV <<"Node vector size "<<coords.size()<<endl;
   // std::vector<std::pair<int,int>>::iterator it;

    for(int i= 0; i<rows; i++) {
           coords.push_back({0,i});
           coords.push_back({columns-1, i});
       }

    for(int i = 1; i<columns-1; i++) {
        coords.push_back({i, 0});
        coords.push_back({i, rows-1});
    }

  int i = 0;
  for(auto &c:coords) {
    //  EV<< "X "<<"Y "<<c.first<<" "<<c.second<<endl;
      cModule *node = getNodeFromCoords(c.first, c.second);
     // EV<<"Destination Node x"<<node->par("x").longValue()<<" y "<<node->par("y").longValue()<<endl;
      if (i ==  0) {
          for(std::map<int, std::string>::iterator it = nodeTypes->begin(); it!= nodeTypes->end(); it++) {
                if((*it).second.compare("Hospital")==0) {
                    node->par("typeId").setLongValue((*it).first);
                    node->par("type").setStringValue((*it).second);
                    cPar &reqGen = node->par("isRequestGenerator");
                    reqGen.setBoolValue(false);

                //    EV<<"Destination Node x"<<node->par("x").longValue()<<" y "<<node->par("y").longValue()<<": Hospital"<<endl;
                }

            }
      }
      else {
          for(std::map<int, std::string>::iterator it = nodeTypes->begin(); it!= nodeTypes->end(); it++) {
                       //    EV<<"TypeName "<<(*it).second<<endl;
                           if((*it).second.compare("FirstAid")==0) {
                            cPar &typeId =  node->par("typeId");
                            cPar &type =  node->par("type");
                            cPar &reqGen = node->par("isRequestGenerator");
                            typeId.setLongValue((*it).first);
                            type.setStringValue((*it).second);
                            reqGen.setBoolValue(false);

                        //    EV<<"Destination Node x"<<node->par("x").longValue()<<" y "<<node->par("y").longValue()<<": Type"<<type.stdstringValue()<<endl;
                        }
                    }
      }
      i++;
  }





}

/*
void  ManhattanNetworkManager::readAllRequestTypes(std::map<int, std::string> *nodeTypes) {

    std::string line;
     std::fstream nodeTypesFile(par("nodeTypesFile").stringValue(), std::ios::in);
     EV<<"File opened"<<nodeTypesFile.is_open()<<endl;
        while(getline(nodeTypesFile, line, '\n'))
        {
           // EV<<"Line "<<endl;
            if (line.empty() || line[0] == '#')
                continue;
          //  EV<<"Line "<< line<<endl;
            std::vector<std::string> tokens = cStringTokenizer(line.c_str()).asVector();
            if (tokens.size() != 2)
                throw cRuntimeError("wrong line in module file: 2 items required, line: \"%s\"", line.c_str());

            // get fields from tokens
            int nodeTypeId = atoi(tokens[0].c_str());
            const char *nodeTypeName = tokens[1].c_str();
           // EV<<"Node type id"<<nodeTypeid<<" name"<<nodeTypeName<<endl;
            nodeTypes->insert(std::pair<int, std::string>(nodeTypeId, nodeTypeName));


 }



}
*/





cModule * ManhattanNetworkManager::getNodeFromCoords(int x, int y) {
    int i = 0;
    cModule *node = getParentModule()->getSubmodule("n",i);
    while(node != NULL) {
         if(node->par("x").longValue() == x && node->par("y").longValue() == y) {
             return node;
         }
        i++;
        node = getParentModule()->getSubmodule("n",i);
    }
     return NULL;

}


cModule * ManhattanNetworkManager::getNodeFromAddress(int address) {
    int i = 0;
       cModule *node = getParentModule()->getSubmodule("n",i);
       while(node != NULL) {
            if(node->par("address").longValue() == address) {
                return node;
            }
           i++;
           node = getParentModule()->getSubmodule("n",i);
       }
        return NULL;



}






/**
 * Return the length of the channel connected to the specified gate.
 *
 * @param dstAddress
 * @param gateIndex
 * @return
 */
double ManhattanNetworkManager::getChannelLength(int nodeAddr, int gateIndex)
{
    return -1;
}




void ManhattanNetworkManager::handleMessage(cMessage *msg)
{

}
