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

#include "AdaptiveNetworkManager.h"

Define_Module(AdaptiveNetworkManager);

void AdaptiveNetworkManager::initialize()
{
    bool onlineRouting = par("onlineRouting").boolValue();
    numberOfVehicles = par("numberOfVehicles");
    numberOfNodes = par("numberOfNodes");
    additionalTravelTime = setAdditionalTravelTime(getParentModule()->par("speed"), getParentModule()->par("acceleration"));

    for(int i=0; i<numberOfVehicles; i++)
        vehiclesPerNode[intuniform(0, numberOfNodes-1, 4)]+=1;

    topo = new cTopology("topo");

    std::vector<std::string> nedTypes;
    nedTypes.push_back("src.node.Node");
    topo->extractByNedTypeName(nedTypes);

 //   setWeight();


    for (int i=0; i<topo->getNumNodes(); i++)
    {
        int address = topo->getNode(i)->getModule()->par("address");

       indexTable[address]=i;
        if(onlineRouting)
            break;

        cTopology::Node* thisNode = NULL;
        int thisAddress;
        topo->calculateUnweightedSingleShortestPathsTo(topo->getNode(i));

        for(int j=0; j<topo->getNumNodes(); j++)
        {
            if(i==j) continue;
            thisNode = topo->getNode(j);
            thisAddress = thisNode->getModule()->par("address");
            if (thisNode->getNumPaths()==0) continue; // not connected

            cGate *parentModuleGate = thisNode->getPath(0)->getLocalGate();
            int gateIndex = parentModuleGate->getIndex();

            rtable[thisAddress].insert(std::make_pair(address, gateIndex));
            dtable[thisAddress].insert(std::make_pair(address, timeDistanceToTarget(thisNode)));
            sdtable[thisAddress].insert(std::make_pair(address, spaceDistanceToTarget(thisNode)));
            cltable[thisAddress].insert(std::make_pair(gateIndex, parentModuleGate->getChannel()->par("length").doubleValue()));

        }
    }
    setDropOffNodes();

   // updateConnectionsMessage = new cMessage("updateConnectionsMessage");
  //  scheduleAt(simTime(), updateConnectionsMessage);


}








AdaptiveNetworkManager::~AdaptiveNetworkManager()
{
    delete topo;
}

/**
 * Evaluate time distance from current node to target one.
 *
 * @param thisNode
 * @return
 */
double AdaptiveNetworkManager::timeDistanceToTarget(cTopology::Node *thisNode)
{
    double hopsToTarget = thisNode->getDistanceToTarget(); //get the hops to reach the target
    double timeDistance = 0.0;
    //double weight = 0.0; //Extra weight parameter

    for (int i=0; i<hopsToTarget; i++)
    {
        cTopology::LinkOut *linkOut = thisNode->getPath(0);
        timeDistance += linkOut->getLocalGate()->getChannel()->par("delay").doubleValue();
        //weight += linkOut->getWeight();

        thisNode = linkOut->getRemoteNode();
    }

    if(timeDistance != 0)
        timeDistance+=additionalTravelTime;

    return timeDistance;
}

/**
 * Evaluate space distance from current node to target one.
 *
 * @param thisNode
 * @return
 */
double AdaptiveNetworkManager::spaceDistanceToTarget(cTopology::Node *thisNode)
{
    double distToTarget = thisNode->getDistanceToTarget(); //get the hops to reach the target
    double spaceDistance = 0.0;
    //double weight = 0.0; //Extra weight parameter

    for (int i=0; i<distToTarget; i++)
    {
        cTopology::LinkOut *linkOut = thisNode->getPath(0);
        spaceDistance += linkOut->getLocalGate()->getChannel()->par("length").doubleValue();

        //weight += linkOut->getWeight();
        thisNode = linkOut->getRemoteNode();
    }

    return spaceDistance;
}

/**
 * Return the space distance from current node to target one.
 *
 * @param dstAddress
 * @return
 */
double AdaptiveNetworkManager::getSpaceDistance(int srcAddr, int dstAddr)
{
    if(sdtable.find(srcAddr) == sdtable.end() || (sdtable[srcAddr].find(dstAddr) == sdtable[srcAddr].end()))
        updateTables(dstAddr);
    return sdtable[srcAddr].find(dstAddr)->second;
}

/**
 * Return the time distance from current node to target one.
 *
 * @param dstAddress
 * @return
 */
double AdaptiveNetworkManager::getTimeDistance(int srcAddr, int dstAddr)
{
    if(dtable.find(srcAddr) == dtable.end() || (dtable[srcAddr].find(dstAddr) == dtable[srcAddr].end()))
        updateTables(dstAddr);
    return dtable[srcAddr].find(dstAddr)->second;
}

/**
 * Return the outputGate index.
 *
 * @param dstAddress
 * @return
 */
int AdaptiveNetworkManager::getOutputGate(int srcAddr, int dstAddr)
{
    if(rtable.find(srcAddr) == rtable.end() || (rtable[srcAddr].find(dstAddr) == rtable[srcAddr].end()))
        updateTables(dstAddr);
    return rtable[srcAddr].find(dstAddr)->second;
}

/**
 * Return the length of the channel connected to the specified gate.
 *
 * @param dstAddress
 * @param gateIndex
 * @return
 */
double AdaptiveNetworkManager::getChannelLength(int nodeAddr, int gateIndex)
{
    return cltable[nodeAddr].find(gateIndex)->second;
}

/**
 * Update routing and distance tables.
 *
 * @param target address
 */
void AdaptiveNetworkManager::updateTables(int destAddress)
{

    cTopology::Node* thisNode = NULL;
    int thisAddress;
    int i = indexTable[destAddress];
    topo->calculateWeightedSingleShortestPathsTo(topo->getNode(i));

    for(int j=0; j<topo->getNumNodes(); j++)
    {
        if(i==j) continue;
        thisNode = topo->getNode(j);
        thisAddress = thisNode->getModule()->par("address");
        if (thisNode->getNumPaths()==0) continue; // not connected

        cGate *parentModuleGate = thisNode->getPath(0)->getLocalGate();
        int gateIndex = parentModuleGate->getIndex();

        rtable[thisAddress].insert(std::make_pair(destAddress, gateIndex));
        dtable[thisAddress].insert(std::make_pair(destAddress, timeDistanceToTarget(thisNode)));
        sdtable[thisAddress].insert(std::make_pair(destAddress, spaceDistanceToTarget(thisNode)));
        cltable[thisAddress].insert(std::make_pair(gateIndex, parentModuleGate->getChannel()->par("length").doubleValue()));
    }
}

/**
 * Return the vehicles started from nodeAddr.
 *
 * @param nodeAddr
 * @return
 */
int AdaptiveNetworkManager::getVehiclesPerNode(int nodeAddr)
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
bool AdaptiveNetworkManager::isValidAddress(int nodeAddr)
{
    if(indexTable.find(nodeAddr) != indexTable.end())
        return true;
    return false;
}


void AdaptiveNetworkManager::setWeight() {
    EV<<"SETTING WEIGHT"<<endl;
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


 bool AdaptiveNetworkManager::isValidDestinationAddress(int requestTypeId,int destAddr) {
     std::vector<cModule *> *destNodes = getAllDestinationNodes();
         for(int i = 0; i< destNodes->size(); i++) {
             if((*destNodes)[i]->par("address").longValue() == destAddr && (*destNodes)[i]->par("typeId").longValue() == requestTypeId ) {
                 return true;
             }
         }
         return  false;

 }



 bool AdaptiveNetworkManager::isValidDestinationAddress(int destAddr) {

     std::vector<cModule *> *destNodes = getAllDestinationNodes();
     for(int i = 0; i< destNodes->size(); i++) {
         if((*destNodes)[i]->par("address").longValue() == destAddr) {
             return true;
         }
     }
     return  false;
 }


 int AdaptiveNetworkManager::getValidDestinationAddress(int requestTypeId) {
     int destAddr;
     std::vector<cModule *> *destNodes = getAllDestinationNodes();
         for(int i = 0; i< destNodes->size(); i++) {
             if((*destNodes)[i]->par("typeId").longValue() == requestTypeId) {
                 destAddr = (*destNodes)[i]->par("address").longValue();
             }
         }
         return  destAddr;
     }



  void AdaptiveNetworkManager::setDropOffNodes() {

      std::map<int, std::string> *nodeTypes(new std::map<int, std::string>());
      readAllNodeTypes(nodeTypes, par("nodeTypesFile").stringValue());

      std::vector<cModule *> *destNodes = getAllDestinationNodes();
      int typeID;
      std::string type;
      for(int i = 0; i<destNodes->size(); i++) {
          if(i == 0) {
          /*    destNodes[i]->par("typeID").setLongValue((*nodeTypes).)
              destNodes[i]->par("type").setStringValue((*nodeTypes)[(*nodeTypes).size()-1]);
          }
          else {

             if((*nodeTypes).size() > 3) {
               typeID = intuniform(1, (*nodeTypes).size()-2, 3);
              destNodes[i]->par("type").setStringValue((*nodeTypes)[(*nodeTypes).size()-1]);
              }
              else {
                  destNodes[i]->par("type").setStringValue(1);
              }
          }*/

                       for(std::map<int, std::string>::iterator it = nodeTypes->begin(); it!= nodeTypes->end(); it++) {
                             if((*it).second.compare("Hospital")==0) {
                                 (*destNodes)[i]->par("typeId").setLongValue((*it).first);
                                 (*destNodes)[i]->par("type").setStringValue((*it).second);
                                 cPar &reqGen = (*destNodes)[i]->par("isRequestGenerator");
                                 reqGen.setBoolValue(false);

                                 EV<<"Destination Node x"<<(*destNodes)[i]->par("x").longValue()<<" y "<<(*destNodes)[i]->par("y").longValue()<<": Hospital"<<endl;
                             }

                         }
                   }
                   else {
                       for(std::map<int, std::string>::iterator it = nodeTypes->begin(); it!= nodeTypes->end(); it++) {
                                        EV<<"TypeName "<<(*it).second<<endl;
                                        if((*it).second.compare("FirstAid")==0) {
                                         cPar &typeId = (*destNodes)[i]->par("typeId");
                                         cPar &type =  (*destNodes)[i]->par("type");
                                         cPar &reqGen = (*destNodes)[i]->par("isRequestGenerator");
                                         typeId.setLongValue((*it).first);
                                         type.setStringValue((*it).second);
                                         reqGen.setBoolValue(false);

                                         EV<<"Destination Node x"<<(*destNodes)[i]->par("x").longValue()<<" y "<<(*destNodes)[i]->par("y").longValue()<<": Type"<<type.stdstringValue()<<endl;
                                     }
                                 }
                   }

               }

      EV<<"Nodes readed: "<<nodeTypes->size()<<endl;



  }






 std::vector<cModule *> * AdaptiveNetworkManager::getAllDestinationNodes(int nodeTypeId) {

     std::map<cModule *, double> *nodesEcc = new std::map<cModule *, double>();
     std::vector<cModule *> *destNodes = new std::vector<cModule *>();
     EV<<"GET DEST NODES!"<<endl;
      for(int j=0; j<topo->getNumNodes(); j++)
         {
          cTopology::Node *n = topo->getNode(j);
          cModule *m = n->getModule();
          int nAddress = m->par("address").longValue();
          double ecc = calculateEccentricity(nAddress);
          nodesEcc->insert(std::pair<cModule *, double>(m, ecc));
         }
      double eccMax = calculateEccMax(nodesEcc);
      for(auto const &nEcc : (*nodesEcc)) {
          if(nEcc.second == eccMax) {
             destNodes->push_back(nEcc.first);
          }
      }
     return destNodes;
}

 std::vector<cModule *> * AdaptiveNetworkManager::getAllDestinationNodes() {

     std::map<cModule *, double> *nodesEcc = new std::map<cModule *, double>();
        std::vector<cModule *> *destNodes = new std::vector<cModule *>();
        EV<<"GET DEST NODES!"<<endl;
         for(int j=0; j<topo->getNumNodes(); j++)
            {
             cTopology::Node *n = topo->getNode(j);
             cModule *m = n->getModule();
             int nAddress = m->par("address").longValue();
             double ecc = calculateEccentricity(nAddress);
             EV<<"Node Address "<<nAddress<<" Eccentricity"<<ecc<<endl;
             nodesEcc->insert(std::pair<cModule *, double>(m, ecc));
            }
         double eccMax = calculateEccMax(nodesEcc);
         for(auto const &nEcc : (*nodesEcc)) {
             if(nEcc.second == eccMax) {
                destNodes->push_back(nEcc.first);
             }
         }
        return destNodes;


 }





 double AdaptiveNetworkManager::calculateEccentricity(int srcAddr) {
     double ecc;
     int i=0;
     for(int j=0; j<topo->getNumNodes(); j++) {
         cTopology::Node *n = topo->getNode(j);
         cModule *m = n->getModule();
         int destAddr = m->par("address").longValue();
         if(i == 0)
         ecc = sdtable[srcAddr].find(destAddr)->second;
         else {
             if(ecc < sdtable[srcAddr].find(destAddr)->second) {
                 ecc = sdtable[srcAddr].find(destAddr)->second;
             }
         }
         i++;
     }
     return ecc;
 }



 double AdaptiveNetworkManager::calculateEccMax( std::map<cModule *, double> *nodesEcc) {
     double eccMax;
     int i=0;
     for(auto const &x : (*nodesEcc)) {
      if(i == 0) {
          eccMax = x.second;
      }
      else {
          if( eccMax < x.second) {
              eccMax = x.second;
          }
      }
     }
     return eccMax;
 }












void AdaptiveNetworkManager::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
    EV << "Scheduling new message  : "<< endl;
       if (!msg->isSelfMessage())
           error("This module does not process messages.");
    //   EV << " update message id  : "<< updateConnectionsMessage->getId() << endl;
   //   const char* name = msg->getName();
       if(msg == updateConnectionsMessage) {
           EV << "UpdateConnectionsMessage arrived  : "<< endl;
        //   destroyConnections();
        //   rndDisconnection();
       //    int res = disconnectNodes(0, 1, 0);

       }


       scheduleAt(simTime()+1000, updateConnectionsMessage);

}


 cModule *AdaptiveNetworkManager::getNodeFromCoords(int x, int y) {

 }



 std::vector<std::pair<int,int>> *AdaptiveNetworkManager::getCenteredSquare(int mult) {

 }



/*


void AdaptiveNetworkManager::updateTopology() {

      std::vector<std::string> nedTypes;
      nedTypes.push_back("src.node.Node");
      topo->extractByNedTypeName(nedTypes);


}


void AdaptiveNetworkManager::destroyConnections() {

  // cTopology* topo = new cTopology("topo");
  // std::vector<std::string> nedTypes;
  // nedTypes.push_back("node.Node");
//   topo->extractByNedTypeName(nedTypes);

   for(int i=0; i<topo->getNumNodes(); i++) {
          cTopology::Node* n = topo->getNode(i);
          EV << "Num Out Link" << n->getNumOutLinks() << endl;
          int j=0;
          while(j<n->getNumOutLinks()) {
          cGate *g = n->getLinkOut(j)->getLocalGate();
          g->disconnect();
          j++;
          }

      }
   updateTopology();

}


int AdaptiveNetworkManager::disconnectNodes( int srcNodeIdx, int destNodeIdx, int count=0) {
    if(srcNodeIdx == destNodeIdx)
        return -1;
    cTopology::Node* srcNode = getNodeByIdx(srcNodeIdx);
               cTopology::Node* destNode = getNodeByIdx(destNodeIdx);
               for(int j = 0; j<srcNode->getNumOutLinks(); j++) {
                         cTopology::LinkOut* srcLinkOut = srcNode->getLinkOut(j);
                         for(int i=0; i<destNode->getNumInLinks(); i++) {
                             cTopology::LinkIn* destLinkIn = destNode->getLinkIn(i);
                             EV <<"Dest local gate %d Src remote gate %d"<< destLinkIn->getLocalGateId() << srcLinkOut->getRemoteGateId() << endl;
                             if(destLinkIn->getLocalGateId() == srcLinkOut->getRemoteGateId()) {
                                 EV <<"Disconnection src dest " << endl;
                                 srcLinkOut->getRemoteGate()->disconnect();
                                 updateTopology();
                                 count++;
                           //      if(count == 1) {
                                 //  disconnectNodes(destNodeIdx, srcNodeIdx, count);
                           //      }
                                 if(count == 2) {
                                     return 0;
                                 }
                               //  return;
                             }
                         }

               }
               return -1;
}



void AdaptiveNetworkManager::rndDisconnection() {
        int srcNodeIdx;
        int destNodeIdx;
        int numNodes =topo->getNumNodes();
        if(numNodes>1) {
        do {
            int numNodes =topo->getNumNodes();
            srcNodeIdx = intuniform(0, numNodes-1, 1);
            destNodeIdx = intuniform(0, numNodes-1, 1);
            EV <<"SRCID DESTID"<< srcNodeIdx << destNodeIdx << endl;
        }  while(disconnectNodes(srcNodeIdx, destNodeIdx) == -1);
    }
}



void AdaptiveNetworkManager::connectNodes(cGate *src, cGate *dest, double delay, double ber, double datarate)
{
    cDatarateChannel *channel = NULL;


    if (delay>0 || ber>0 || datarate>0)
    {
        channel = cDatarateChannel::create("channel");
        if (delay>0)
            channel->setDelay(delay);
        if (ber>0)
            channel->setBitErrorRate(ber);
        if (datarate>0)
            channel->setDatarate(datarate);


    }
    src->connectTo(dest, channel);


}


cChannel* AdaptiveNetworkManager::checkForChannel() {

       cChannel* c = NULL;
       for(int i=0; i<topo->getNumNodes(); i++) {
           cTopology::Node* n = topo->getNode(i);
           for(int j=0; j<n->getNumOutLinks(); j++) {
                c = n->getLinkOut(j)->getLocalGate()->findTransmissionChannel();
                if(c != NULL){
                    return c;
                }
           }
       }
       return c;

}




 cTopology::Node* AdaptiveNetworkManager::getNodeByIdx(int idx) {


       for(int i=0; i<topo->getNumNodes(); i++) {
           cTopology::Node* n = topo->getNode(i);
           EV << "Node id" << n->getModuleId() <<endl;
           if(i == idx)
               return n;
       }
       return NULL;

   }


 bool AdaptiveNetworkManager::existDirectConnection(int srcNodeId, int destNodeId) {
     bool r = false;
     if(srcNodeId != destNodeId ) {
            cTopology::Node* srcNode = getNodeByIdx(srcNodeId);
            cTopology::Node* destNode = getNodeByIdx(destNodeId);
            for(int j = 0; j<srcNode->getNumOutLinks(); j++) {
                     cTopology::LinkOut* srcLinkOut = srcNode->getLinkOut(j);
            for(int i=0; i<destNode->getNumInLinks(); i++) {
                cTopology::LinkIn* destLinkIn = destNode->getLinkIn(i);
                EV <<"Dest local gate %d Src remote gate %d"<< destLinkIn->getLocalGateId() << srcLinkOut->getRemoteGateId() << endl;
                if(destLinkIn->getLocalGateId() == srcLinkOut->getRemoteGateId()) {
                    cTopology::LinkOut* destLinkOut = destNode->getLinkOut(i);
                    cTopology::LinkIn* srcLinkIn = srcNode->getLinkIn(i);
                    if(destLinkOut->getRemoteGateId() == srcLinkIn->getLocalGateId()) {
                        r =  true;
                    }
                }

            }
                     }
 }
           return r;
 }


*/











