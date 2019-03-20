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

    rows = getParentModule()->par("width");
    columns = getParentModule()->par("height");
    MAX_DELAY = getParentModule()->par("maxTravelTime");
    MAX_TRY = 0;

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

    setAllWeight();


    for (int i=0; i<topo->getNumNodes(); i++)
    {
        int address = topo->getNode(i)->getModule()->par("address");

       indexTable[address]=i;
        if(onlineRouting)
            break;

        cTopology::Node* thisNode = NULL;
        int thisAddress;
        topo->calculateWeightedSingleShortestPathsTo(topo->getNode(i));

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
    updateWeightMessage = new cMessage("up");
 //   scheduleAt(0, updateWeightMessage);

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
    /*   double hopsToTarget = thisNode->getDistanceToTarget(); //get the hops to reach the target
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
     */

    double timeDistance = 0.0;

    cTopology::LinkOut *linkOut = thisNode->getPath(0);

    while(linkOut!= NULL) {
        // EV<<"DEST NODE IS "<<linkOut->getRemoteNode()->getModule()->par("address").doubleValue()<<endl;
        timeDistance += linkOut->getLocalGate()->getChannel()->par("delay").doubleValue();
        linkOut = linkOut->getRemoteNode()->getPath(0);
    }



    timeDistance += additionalTravelTime;

    //    EV<<"TIME DIST TO TARGET FOR NODE "<<thisNode->getModuleId()<<" "<<timeDistance<<endl;

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
    //   double distToTarget = thisNode->getDistanceToTarget(); //get the hops to reach the target
    //  double spaceDistance = 0.0;
    //double weight = 0.0; //Extra weight parameter
    /*
    for (int i=0; i<distToTarget; i++)
    {
        cTopology::LinkOut *linkOut = thisNode->getPath(0);
        spaceDistance += linkOut->getLocalGate()->getChannel()->par("length").doubleValue();

        //weight += linkOut->getWeight();
        thisNode = linkOut->getRemoteNode();

    }*/

    double spaceDistance = 0.0;
    cTopology::LinkOut *linkOut = thisNode->getPath(0);
    while(linkOut!= NULL) {
        // EV<<"DEST NODE IS "<<linkOut->getRemoteNode()->getModule()->par("address").doubleValue()<<endl;
        spaceDistance += linkOut->getLocalGate()->getChannel()->par("length").doubleValue();
        linkOut = linkOut->getRemoteNode()->getPath(0);
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
    /*  if(sdtable.find(srcAddr) == sdtable.end() || (sdtable[srcAddr].find(dstAddr) == sdtable[srcAddr].end()))
        updateTables(dstAddr);
    return sdtable[srcAddr].find(dstAddr)->second;*/
    cTopology::Node *n = calculatePath(srcAddr, dstAddr);
    double spaceDistance = spaceDistanceToTarget(n);
    return spaceDistance;

}

/**
 * Return the time distance from current node to target one.
 *
 * @param dstAddress
 * @return
 */
double AdaptiveNetworkManager::getTimeDistance(int srcAddr, int dstAddr)
{
    /*   if(dtable.find(srcAddr) == dtable.end() || (dtable[srcAddr].find(dstAddr) == dtable[srcAddr].end()))
        updateTables(dstAddr);
    EV<<"TIME DISTANCE SRC "<<srcAddr<< " to "<<" DEST"<<dstAddr<<" "<<dtable[srcAddr].find(dstAddr)->second<<endl;
    return dtable[srcAddr].find(dstAddr)->second;*/

    cTopology::Node *n = calculatePath(srcAddr, dstAddr);
    double timeDistance = timeDistanceToTarget(n);
    return timeDistance;

}

/**
 * Return the outputGate index.
 *
 * @param dstAddress
 * @return
 */
int AdaptiveNetworkManager::getOutputGate(int srcAddr, int dstAddr)
{
    /*  if(rtable.find(srcAddr) == rtable.end() || (rtable[srcAddr].find(dstAddr) == rtable[srcAddr].end()))
        updateTables(dstAddr);
    return rtable[srcAddr].find(dstAddr)->second;*/

    cTopology::Node *n = calculatePath(srcAddr, dstAddr);
    return n->getPath(0)->getLocalGate()->getIndex();
    //  if(rtable.find(srcAddr) == rtable.end() || (rtable[srcAddr].find(dstAddr) == rtable[srcAddr].end()))
    //       updateTables(dstAddr);
    //  rtable[srcAddr].insert(std::make_pair(dstAddr, n->getModule()->par("address").doubleValue()));
    //  return rtable[srcAddr].find(dstAddr)->second;


}


cTopology::Node *AdaptiveNetworkManager::calculatePath(int srcAddr, int destAddr) {




    setAllWeight();
    //cModule *startNode = getNodeFromAddress(srcAddr);
    // cModule *destNode = getNodeFromAddress(destAddr);
    int indexSrc = indexTable[srcAddr];
    int indexDest = indexTable[destAddr];
    cTopology::Node *source;
    //  if(destNode != NULL) {
    topo->calculateWeightedSingleShortestPathsTo(topo->getNode(indexDest));



    /*

        for(int i = 0; i<topo->getNumNodes(); i++) {

        cTopology::Node *thisNode = topo->getNode(i);
        if(topo->getNode(indexDest)->getModuleId() == thisNode->getModuleId()) continue;

        cGate *parentModuleGate = thisNode->getPath(0)->getLocalGate();
        int gateIndex = parentModuleGate->getIndex();
        int thisAddress = thisNode->getModule()->par("address").doubleValue();

        rtable[thisAddress].insert(std::make_pair(destAddr, gateIndex));
        dtable[thisAddress].insert(std::make_pair(destAddr, timeDistanceToTarget(thisNode)));
        sdtable[thisAddress].insert(std::make_pair(destAddr, spaceDistanceToTarget(thisNode)));
        cltable[thisAddress].insert(std::make_pair(gateIndex, parentModuleGate->getChannel()->par("length").doubleValue()));
        }*/


    return topo->getNode(indexSrc);

    /*
         //   EV<<"CHECK NODE"<<endl;
            cTopology::Node *n = topo->getNode(i);
         //   if(destNode->par("address").doubleValue() == n->getModule()->par("address").doubleValue()) {
          //      EV<<"FOUNDED DEST NODE"<<endl;


         //     }
            if(startNode->par("address").doubleValue() == n->getModule()->par("address").doubleValue()) {
                source = n;
             //   cTopology::Node *next = source;

            }



     //   }


   }*/
    //   return source;

}


void AdaptiveNetworkManager::setWeight(cTopology *topo) {
    //  EV<<"SETTING WEIGHT"<<endl;
    std::map<int, double> *rDMatch = new std::map<int, double>();
    readAllRiskDelayFactorMatching(rDMatch, par("riskDelIncrFactFile").stringValue());

    for(int i = 0; i<topo->getNumNodes(); i++) {

        cTopology::Node *n = topo->getNode(i);

        for(int j = 0; j<n->getNumOutLinks(); j++) {
            cTopology::LinkOut *l = n->getLinkOut(j);
            double riskLevel = l->getLocalGate()->getTransmissionChannel()->par("riskLevel").doubleValue();
            // double delay = l->getLocalGate()->getTransmissionChannel()->par("delay").doubleValue();
            // if(riskLevel > 0)
            /*   if((n->getModule()->getIndex() == 12 && l->getRemoteNode()->getModule()->getIndex() == 7) || (n->getModule()->getIndex() == 7 && l->getRemoteNode()->getModule()->getIndex() == 12)) {
                setMaxRiskAndWeight(n,l->getRemoteNode() );

            }*/
            //    else {
            for(std::map<int, double>::iterator it = rDMatch->begin(); it!= rDMatch->end(); it++) {
                if((*it).first == riskLevel) {
                    double speed = (*it).second;
                    double length = l->getLocalGate()->getTransmissionChannel()->par("length").doubleValue();
                    double time = 0;
                    if(speed == 0)
                        time = MAX_DELAY;   // da settare in AMOD ned file
                    else {
                        time = length/speed;
                    }
                    if(riskLevel == 0)
                        l->setWeight(time);
                    else
                        l->setWeight(time*riskLevel);
                    l->getLocalGate()->getTransmissionChannel()->par("delay").setDoubleValue(time);
                    //  EV<<"RiskLevel "<<riskLevel<<" delay"<<time<<endl;

                    // else
                    //   l->setWeight(delay);
                }
            }
            // }
        }

    }
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
        if(thisNode->getModule()->getIndex() == 12 && destAddress == 2)  {
            EV<< "FROM "<<"12"<<" to "<<"2"<<"gate index "<<gateIndex<<endl;
        }

        rtable[thisAddress].insert(std::make_pair(destAddress, gateIndex));
        dtable[thisAddress].insert(std::make_pair(destAddress, timeDistanceToTarget(thisNode)));
        sdtable[thisAddress].insert(std::make_pair(destAddress, spaceDistanceToTarget(thisNode)));
        cltable[thisAddress].insert(std::make_pair(gateIndex, parentModuleGate->getChannel()->par("length").doubleValue()));
    }
}


/*
cTopology * AdaptiveNetworkManager::getNewTopo() {



}
 */

/*
void AdaptiveNetworkManager::updateAllTables() {

       cTopology *newTopo = new cTopology("topo");
       std::vector<std::string> nedTypes;
       nedTypes.push_back("src.node.Node");
       newTopo->extractByNedTypeName(nedTypes);

       for(int j=0; j<topo->getNumNodes(); j++)
        {

        cTopology::Node *nodeT = topo->getNode(j);
           for(int i=0; i<newTopo->getNumNodes(); i++) {
               cTopology::Node *nodeNew = topo->getNode(i);
               if(nodeNew->getModuleId() == nodeT->getModuleId()) {
               for(int k = 0; k<nodeT->getNumOutLinks(); k++) {
                   cTopology::LinkOut *lT = nodeT->getLinkOut(k);
                   for(int l = 0; l<nodeNew->getNumOutLinks(); l++) {
                       cTopology::LinkOut *lnewT = nodeNew->getLinkOut(l);
                       if(lT->getRemoteNode()->getModuleId() == lnewT->getRemoteNode()->getModuleId()) {
                           lnewT->setWeight(lT->getWeight());
                           EV<<"Weight of channel "<<nodeNew->getModule()->getIndex()<<"-->"<<lnewT->getRemoteNode()->getModule()->getIndex()<<" has updated to "<<lnewT->getWeight()<<endl;

                       }

                   }
               }

               }
           }
        }


     // topo = newTopo;
    setAllWeight();

    for(std::map<int,int>::iterator it = indexTable.begin(); it != indexTable.end(); it++) {
         updateTables((*it).first);
       //  EV<<"PATH TO ADDRESS "<<(*it).first<<" updated"<<endl;
    }


}*/

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
/*

void AdaptiveNetworkManager::setWeight(cTopology::Node *srcNode, cTopology::Node *destNode, double weight) {

    for(int j = 0; j<srcNode->getNumOutLinks(); j++) {
               cTopology::LinkOut *l = srcNode->getLinkOut(j);

             if(destNode->getModuleId() ==  l->getRemoteNode()->getModuleId()) {
                   l->setWeight(weight);
             }
}
}

 */

void AdaptiveNetworkManager::updateWeight(cTopology::Node *srcNode, cTopology::Node *destNode) {
    std::map<int, double> *rDMatch = new std::map<int, double>();
    readAllRiskDelayFactorMatching(rDMatch, par("riskDelIncrFactFile").stringValue());

    for(int j = 0; j<srcNode->getNumOutLinks(); j++) {
        cTopology::LinkOut *l = srcNode->getLinkOut(j);

        if(destNode->getModuleId() ==  l->getRemoteNode()->getModuleId()) {


            double riskLevel = l->getLocalGate()->getTransmissionChannel()->par("riskLevel").doubleValue();
            double delay = l->getLocalGate()->getTransmissionChannel()->par("delay").doubleValue();
            for(std::map<int, double>::iterator it = rDMatch->begin(); it!= rDMatch->end(); it++) {
                if((*it).first == riskLevel) {

                    if(riskLevel == 0)
                        l->setWeight(delay);
                    else
                        l->setWeight(delay*riskLevel);
                }



            }


        }
    }
}




void AdaptiveNetworkManager::setRiskLevel(cTopology::Node *srcNode, cTopology::Node *destNode, double riskLevel) {
    std::map<int, std::string> *riskLevels = new std::map<int, std::string>();
    readAllRiskLevels(riskLevels, par("riskLevelsFile").stringValue());

    for(int j = 0; j<srcNode->getNumOutLinks(); j++) {
        cTopology::LinkOut *l = srcNode->getLinkOut(j);

        if(destNode->getModuleId() ==  l->getRemoteNode()->getModuleId()) {
            l->getLocalGate()->getTransmissionChannel()->par("riskLevel").setDoubleValue(riskLevel);
            cDisplayString &s  = l->getLocalGate()->getTransmissionChannel()->getDisplayString();
            std::string  sT("ls=");
            sT.append((*riskLevels)[riskLevel].c_str());
            s.parse(sT.c_str());
        }
    }
}


void AdaptiveNetworkManager:: updateChannelDelay(cTopology::Node *srcNode, cTopology::Node *destNode) {
    std::map<int, double> *rDMatch = new std::map<int, double>();
    readAllRiskDelayFactorMatching(rDMatch, par("riskDelIncrFactFile").stringValue());
    for(int j = 0; j<srcNode->getNumOutLinks(); j++) {
        cTopology::LinkOut *l = srcNode->getLinkOut(j);

        if(destNode->getModuleId() ==  l->getRemoteNode()->getModuleId()) {
            double riskLevel = l->getLocalGate()->getTransmissionChannel()->par("riskLevel").doubleValue();
            for(std::map<int, double>::iterator it = rDMatch->begin(); it!= rDMatch->end(); it++) {
                if((*it).first == riskLevel) {
                    double speed = (*it).second;
                    double length = l->getLocalGate()->getTransmissionChannel()->par("length").doubleValue();
                    double time = 0;
                    if(speed == 0)
                        time = MAX_DELAY;
                    else {
                        time = length/speed;
                    }

                    l->getLocalGate()->getTransmissionChannel()->par("delay").setDoubleValue(time);
                }

            }
        }

    }
}


/*
void AdaptiveNetworkManager::setChannelDelay(cTopology::Node *srcNode, cTopology::Node *destNode) {


}
 */


void AdaptiveNetworkManager::setAllWeight() {
    //EV<<"SETTING WEIGHT"<<endl;
    std::map<int, double> *rDMatch = new std::map<int, double>();
    readAllRiskDelayFactorMatching(rDMatch, par("riskDelIncrFactFile").stringValue());

    for(int i = 0; i<topo->getNumNodes(); i++) {

        cTopology::Node *n = topo->getNode(i);

        for(int j = 0; j<n->getNumOutLinks(); j++) {
            cTopology::LinkOut *l = n->getLinkOut(j);
            double riskLevel = l->getLocalGate()->getTransmissionChannel()->par("riskLevel").doubleValue();

            for(std::map<int, double>::iterator it = rDMatch->begin(); it!= rDMatch->end(); it++) {
                if((*it).first == riskLevel) {
                    double speed = (*it).second;
                    double length = l->getLocalGate()->getTransmissionChannel()->par("length").doubleValue();
                    double time = 0;
                    if(speed == 0)
                        time = MAX_DELAY;
                    else {
                        time = length/speed;
                    }
                    if(riskLevel == 0)
                        l->setWeight(time);
                    else
                        l->setWeight(time*riskLevel);
                    l->getLocalGate()->getTransmissionChannel()->par("delay").setDoubleValue(time);

                }
            }
            // }
        }

    }
}

//void  AdaptiveNetworkManager::setRiskLevel()


int AdaptiveNetworkManager::getMaxRisk() {
    std::map<int, std::string> *rLevels = new std::map<int, std::string>();
    readAllRiskLevels(rLevels, par("riskLevelsFile").stringValue());
    int maxRisk = 0;
    int i = 0;
    for(std::map<int, std::string>::iterator it = rLevels->begin(); it!= rLevels->end(); it++) {
        int val = (*it).first;
        if (i == 0) {
            maxRisk = val;
        }
        else if( maxRisk <  val) {
            maxRisk = val;
        }

    }
    return maxRisk;

}






/*
void AdaptiveNetworkManager::setMaxRiskAndWeight(cTopology::Node *src, cTopology::Node *dest) {


    std::map<int, double> *rDMatch = new std::map<int, double>();
    readAllRiskDelayFactorMatching(rDMatch, par("riskDelIncrFactFile").stringValue());


        for(int i = 0; i<src->getNumOutLinks(); i++) {
            cTopology::LinkOut *l = src->getLinkOut(i);
            if(l->getRemoteNode()->getModuleId() == dest->getModuleId()) {
             double maxRisk = getMaxRisk();
             EV<<"MAX_RISK"<<maxRisk<<endl;
             int riskLevel = l->getLocalGate()->getTransmissionChannel()->par("riskLevel").doubleValue();
             if(riskLevel < maxRisk) {
              l->getLocalGate()->getTransmissionChannel()->par("riskLevel").setDoubleValue(maxRisk);
              l->getLocalGate()->getTransmissionChannel()->par("delay").setDoubleValue(MAX_DELAY);
            //  l->setWeight(maxRisk*MAX_DELAY);
              cDisplayString &s  = l->getLocalGate()->getTransmissionChannel()->getDisplayString();
                       s.parse("ls=white");
              //      EV<<"Weight of channel "<<src->getModuleId()<<"-->"<<l->getRemoteNode()->getModuleId()<<" has updated to "<<l->getWeight()<<endl;
             }

            }
        }
    }
 */






bool AdaptiveNetworkManager::isValidDestinationAddress(int requestTypeId,int destAddr) {
    bool reqVal = false;
    bool nodeVal = false;
    std::map<int, int> *nRMatch(new std::map<int, int>());
    readAlldestNodesRequestsMatching(nRMatch, par("destNodesRequestMatchingFile").stringValue());
    //   int nodeTypeId;
    for(std::map<int, int>::iterator it = nRMatch->begin(); it != nRMatch->end(); it++) {
        if(it->first == requestTypeId) {
            reqVal = true;
            //   return true;
        }
    }

    std::vector<cModule *> destNodes = getAllDestinationNodes();
    for(std::vector<cModule *>::iterator it = destNodes.begin(); it != destNodes.end(); it++) {
        //   EV<<"Address check  nodeT"<<(*it)->par("address").longValue()<<"destAddr"<<destAddr<<endl;
        int addr = (*it)->par("address").longValue();
        if(addr == destAddr)
            nodeVal = true;
    }



    return reqVal && nodeVal;

}



bool AdaptiveNetworkManager::isValidDestinationAddress(int destAddr) {

    std::vector<cModule *> destNodes = getAllDestinationNodes();
    for(int i = 0; i< destNodes.size(); i++) {
        if(destNodes[i]->par("address").longValue() == destAddr) {
            return true;
        }
    }
    return  false;
}



void AdaptiveNetworkManager::setDropOffNodes() {
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




std::vector<cModule *>  AdaptiveNetworkManager::getAllDestinationNodes(int nodeTypeId) {

    std::vector<cModule *> destNodes(0);
    //   EV<<"GET DEST NODES!"<<endl;
    if(nodeTypeId != 0 ) {
        for(int j=0; j<topo->getNumNodes(); j++)
        {
            cTopology::Node *n = topo->getNode(j);
            cModule *m = n->getModule();
            if(m->par("typeId").doubleValue() == nodeTypeId)
                destNodes.push_back(m);

        }
    }
    return destNodes;

}



std::vector<cModule *>  AdaptiveNetworkManager::getAllDestinationNodes() {
    std::map<int, std::string> *nodeTypes = new std::map<int, std::string>();
    std::vector<cModule *> destNodes(0);
    readAllNodeTypes(nodeTypes, par("nodeTypesFile").stringValue());
    for(std::map<int, std::string>::iterator it = nodeTypes->begin(); it != nodeTypes->end(); it++) {
        for(int j=0; j<topo->getNumNodes(); j++)
        {
            cTopology::Node *n = topo->getNode(j);
            cModule *m = n->getModule();
            if(m->par("typeId").doubleValue() == (*it).first && m->par("typeId").doubleValue()!=0)
                destNodes.push_back(m);


        }
    }
    return destNodes;
}



void AdaptiveNetworkManager::checkAndSetMaxRiskAndWeight(cTopology::Node *srcNode, cTopology::Node *destNode) {


    //std::vector<cModule *> destNodes =  getAllDestinationNodes();
    bool isMaxRiskN = false;
    int numLinks = 0;
    int maxRisk = getMaxRisk();
    double weight;
    double riskLevel;
    for(int j = 0; j<srcNode->getNumOutLinks(); j++) {
        cTopology::LinkOut *l = srcNode->getLinkOut(j);
        if(l->getRemoteNode()->getModuleId() == destNode->getModuleId()) {
            weight = l->getWeight();
            riskLevel = l->getLocalGate()->getTransmissionChannel()->par("riskLevel");
        }
    }
    setRiskLevel(srcNode, destNode, maxRisk);
    updateChannelDelay(srcNode, destNode);
    updateWeight(srcNode, destNode);
    //  setWeight(destNode, srcNode, maxRisk*MAX_DELAY);
    bool isGoodP = true;
    std::vector<cModule *> destNodes = getAllDestinationNodes();

    for(int i = 0; i<destNodes.size(); i++) {

        calculatePath(srcNode->getModule()->par("address"), destNodes[i]->par("address"));
        cTopology::LinkOut *linkOut = srcNode->getPath(0);
        while(linkOut!= NULL) {
            if(linkOut->getWeight()  == maxRisk*MAX_DELAY) {
                isGoodP = false;
                break;
            }
            linkOut = linkOut->getRemoteNode()->getPath(0);
        }
    }



    for(int i = 0; i<destNodes.size(); i++) {
        int idNode = indexTable[destNodes[i]->par("address")];
        calculatePath(destNodes[i]->par("address"),destNode->getModule()->par("address"));
        cTopology::LinkOut *linkOut = topo->getNode(idNode)->getPath(0);
        while(linkOut!= NULL) {
            if(linkOut->getWeight()  == maxRisk*MAX_DELAY) {
                isGoodP = false;
                break;
            }
            linkOut = linkOut->getRemoteNode()->getPath(0);
        }
    }



    if(!isGoodP) {
        setRiskLevel(srcNode, destNode, riskLevel);
        updateChannelDelay(srcNode, destNode);
        updateWeight(srcNode, destNode);
        //  setRiskLevel(srcNode, destNode, riskLevel);

        //setWeight(destNode, srcNode, weight);
    }


}





void AdaptiveNetworkManager::handleMessage(cMessage *msg)
{
    double maxRiskChann = getParentModule()->par("maxRiskChannelsPerc");
    // TODO - Generated method body
    EV << "Scheduling new message  : "<< endl;
    if (!msg->isSelfMessage())
        error("This module does not process messages.");
    //   EV << " update message id  : "<< updateConnectionsMessage->getId() << endl;
    //   const char* name = msg->getName();
    if(msg == updateWeightMessage) {
        EV << "UpdateConnectionsMessage arrived  : "<< endl;
        if(MAX_TRY <= maxRiskChann) {
            std::pair<cTopology::Node*, cTopology::Node*> nodes = getCenteredSquareRndLinkedNodes();
            // if(isMaxRiskNode(nodes.first)) {
            //  setMaxRiskAndWeight(nodes.first, nodes.second);
            //   EV<<"SECOND NODE"<<nodes.second->getModuleId()<<"FIRST NODE "<<nodes.first->getModule()<<endl;
            //  if(nodes.second != NULL)
            // setMaxRiskAndWeight(nodes.second, nodes.first);
            checkAndSetMaxRiskAndWeight(nodes.first, nodes.second);
            checkAndSetMaxRiskAndWeight(nodes.second, nodes.first);
        }
        //   updateAllTables();
        MAX_TRY++;
    }




    //   scheduleAt(simTime()+1000, updateWeightMessage);

}


cModule *AdaptiveNetworkManager::getNodeFromCoords(int x, int y) {
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


std::pair< cTopology::Node*, cTopology::Node*> AdaptiveNetworkManager::getCenteredSquareRndLinkedNodes() {
    int riskZoneExp = this->getParentModule()->par("riskZoneExp").doubleValue();
    std::vector<std::pair<int,int> > *coords = getCenteredSquare(riskZoneExp);
    std::vector<cTopology::Node *> nodes(0);
    for(const auto &x : (*coords)) {
        //   EV<<"COORDS "<<x.first<<" "<<x.second;
        cModule *node = getNodeFromCoords(x.first, x.second);
        for(int i = 0; i<topo->getNumNodes(); i++) {
            cTopology::Node *tNode = topo->getNode(i);
            if(node->getId() == tNode->getModuleId()) {
                nodes.push_back(tNode);
            }
        }
    }
    int indexNode = intuniform(0, nodes.size()-1,3);
    cTopology::Node *sNode = nodes[indexNode];
    int indexLink;
    cTopology::LinkOut* sLinkOut;
    cTopology::Node *dNode;

    indexLink = intuniform(0, sNode->getNumOutLinks()-1,3);
    sLinkOut = sNode->getLinkOut(indexLink);
    while(sLinkOut->getRemoteNode()->getModuleId() != dNode->getModuleId()) {
        indexLink = intuniform(0, sNode->getNumOutLinks()-1,3);
        sLinkOut = sNode->getLinkOut(indexLink);

        for(int k = 0; k<nodes.size(); k++) {
            cTopology::Node *nT = nodes[k];
            if(sLinkOut->getRemoteNode()->getModuleId() == nT->getModuleId()) {
                dNode = nT;

                // return std::make_pair(sNode, dNode);
            }
        }

    }


    return std::make_pair(sNode, dNode);
}


std::vector<std::pair<int,int>> *AdaptiveNetworkManager::getCenteredSquare(int mult) {
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







int AdaptiveNetworkManager::getCloserValidDestinationAddress(int srcAddress, int requestTypeId) {

    //  updateAllTables();
    std::map<int, int> *nRMatch(new std::map<int, int>());
    readAlldestNodesRequestsMatching(nRMatch, par("destNodesRequestMatchingFile").stringValue() );
    int nodeTypeId;
    for(std::map<int, int>::iterator it = nRMatch->begin(); it != nRMatch->end(); it++) {
        if((*it).first == requestTypeId) {
            nodeTypeId = (*it).second;
        }
    }
    std::vector<cModule *> nodes = getAllDestinationNodes(nodeTypeId);
    if(nodes.size() == 0 )
        return -1;
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







