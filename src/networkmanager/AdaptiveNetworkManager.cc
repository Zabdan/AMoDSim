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
    MAX_DELAY = getParentModule()->par("maxChDelay");
    MAX_TRY = 0;

    updateSchedulingS=registerSignal("updateSchedulingS");

    bool onlineRouting = par("onlineRouting").boolValue();

    int numVehiclesType1 = par("numberOfVehiclesType1");
    int numVehiclesType0 = par("numberOfVehiclesType0");

    numberOfVehicles = par("numberOfVehicles");
    numberOfNodes = par("numberOfNodes");
    additionalTravelTime = setAdditionalTravelTime(getParentModule()->par("speed"), getParentModule()->par("acceleration"));


    for(int i=0; i<numVehiclesType1; i++)
    {

        int numNode = intuniform(0, numberOfNodes-1, 4);
        vehiclesPerNode[numNode].push_back(1);


    }

    for(int i=0; i<numVehiclesType0; i++)
       {

           int numNode = intuniform(0, numberOfNodes-1, 4);
           vehiclesPerNode[numNode].push_back(0);

       }


    topo = new cTopology("topo");

    std::vector<std::string> nedTypes;
    nedTypes.push_back("src.node.Node");
    topo->extractByNedTypeName(nedTypes);

    setPickUpNodes();
    setDropOffNodes();
    setRedZone();
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
            (*dtable)[thisAddress].insert(std::make_pair(address, timeDistanceToTarget(thisNode)));
            (*sdtable)[thisAddress].insert(std::make_pair(address, spaceDistanceToTarget(thisNode)));
             cltable[thisAddress].insert(std::make_pair(gateIndex, parentModuleGate->getChannel()->par("length").doubleValue()));

        }
    }

    updateWeightMessage = new cMessage("up");
    scheduleAt(0, updateWeightMessage);



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



       bool goodPath = checkForGoodPath(srcAddr, dstAddr);
       if(goodPath) {
           return (*sdtable)[srcAddr].find(dstAddr)->second;

       }




    cTopology::Node *n = calculatePath(srcAddr, dstAddr);
    double spaceDistance = spaceDistanceToTarget(n);
    updateTables(dstAddr);
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


    bool goodPath = checkForGoodPath(srcAddr, dstAddr);
    if(goodPath) {
        return (*dtable)[srcAddr].find(dstAddr)->second;
    }

    cTopology::Node *n = calculatePath(srcAddr, dstAddr);

    double timeDistance = timeDistanceToTarget(n);
    updateTables(dstAddr);
    return timeDistance;


}


/**
 * Verifica se il percorso dal nodo sorgente al nodo destinazione già presente
 * nella tabella di routing non presenta canali con delay massimo
 */

bool AdaptiveNetworkManager::checkForGoodPath(int srcAddr, int dstAddr) {


    // double timeDistance;
       bool goodPath = true;
       int actualAddress = srcAddr;
       while(actualAddress!=dstAddr) {

       int gateIndex = rtable[actualAddress].find(dstAddr)->second;
       int indexNode =  indexTable[actualAddress];
       cTopology::Node *n = topo->getNode(indexNode);
       for(int i=0; i<n->getNumOutLinks(); i++) {
           cTopology::LinkOut *linkOut = n->getLinkOut(i);
           if(linkOut->getLocalGate()->getIndex() == gateIndex) {
               double tmpDistance = linkOut->getLocalGate()->getChannel()->par("delay").doubleValue();
               if(tmpDistance == MAX_DELAY) {
                   goodPath = false;

               }
            //   timeDistance += tmpDistance;
               actualAddress=linkOut->getRemoteNode()->getModule()->par("address");
           }


       }

       if(goodPath==false)
           break;

       }

    return goodPath;
}




/**
 * Return the outputGate index.
 *
 * @param dstAddress
 * @return
 */


int AdaptiveNetworkManager::getOutputGate(int srcAddr, int dstAddr)
{

       bool goodPath = checkForGoodPath(srcAddr, dstAddr);
       if(goodPath) {
           return rtable[srcAddr].find(dstAddr)->second;

       }


       cTopology::Node *n = calculatePath(srcAddr, dstAddr);
       updateTables(dstAddr);
        return n->getPath(0)->getLocalGate()->getIndex();



}






cTopology::Node *AdaptiveNetworkManager::calculatePath(int srcAddr, int destAddr) {



    setAllWeight();
    int indexSrc = indexTable[srcAddr];
    int indexDest = indexTable[destAddr];
    //  if(destNode != NULL) {
    topo->calculateWeightedSingleShortestPathsTo(topo->getNode(indexDest));

    return topo->getNode(indexSrc);



}


/*

void AdaptiveNetworkManager::setWeight(cTopology *topo) {
    //  EV<<"SETTING WEIGHT"<<endl;
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
                        time = MAX_DELAY;   // da settare in AMOD ned file
                    else {
                        time = length/speed;
                    }
                  //  if(riskLevel == 0)
                        l->setWeight(time);
                  //  else
                     //   l->setWeight(time*riskLevel);
                    l->getLocalGate()->getTransmissionChannel()->par("delay").setDoubleValue(time);
                    //  EV<<"RiskLevel "<<riskLevel<<" delay"<<time<<endl;

                    // else
                    //   l->setWeight(delay);
                }
            }
            // }
        }

    }
    rDMatch->clear();
}

*/



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






void AdaptiveNetworkManager::updateTables(int destAddress) {

   cTopology::Node* thisNode = NULL;
   int thisAddress;
   int i = indexTable[destAddress];
  // topo->calculateWeightedSingleShortestPathsTo(topo->getNode(i));



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

      //    EV<< "FROM "<<"SRC"<<" to "<<"thisaddr"<<thisAddress<<" gate: "<<gateIndex<<endl;
         std::map<int,int> mapR = rtable.find(thisAddress)->second;



        rtable[thisAddress][destAddress]=gateIndex;
       (*dtable)[thisAddress][destAddress]=timeDistanceToTarget(thisNode);
       (*sdtable)[thisAddress][destAddress]=spaceDistanceToTarget(thisNode);
       cltable[thisAddress][destAddress]=parentModuleGate->getChannel()->par("length").doubleValue();




      }



}





/**
 * Return the vehicles started from nodeAddr.
 *
 * @param nodeAddr
 * @return
 */


std::list<int> AdaptiveNetworkManager::getVehiclesPerNode(int nodeAddr)
{
    /*
    int nVehicles = 0;
    std::map<int,int>::iterator it;

    it = vehiclesPerNode.find(nodeAddr);
    if (it != vehiclesPerNode.end())
        nVehicles = it->second;

    return nVehicles;
    */
    return std::list<int>(vehiclesPerNode[nodeAddr]);
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
    rDMatch->clear();
    delete rDMatch;
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
    riskLevels->clear();
    delete riskLevels;
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
    rDMatch->clear();
    delete rDMatch;
}


/*
void AdaptiveNetworkManager::setChannelDelay(cTopology::Node *srcNode, cTopology::Node *destNode) {


}
 */

/**
 * Setta i pesi relativi a tutti i link della rete
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
    rDMatch->clear();
    delete rDMatch;
}




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



int AdaptiveNetworkManager::getMinRisk() {
    std::map<int, std::string> *rLevels = new std::map<int, std::string>();
    readAllRiskLevels(rLevels, par("riskLevelsFile").stringValue());
    int maxRisk = getMaxRisk();
    int i = 0;
    for(std::map<int, std::string>::iterator it = rLevels->begin(); it!= rLevels->end(); it++) {
        int val = (*it).first;
        if (i == 0) {
            maxRisk = val;
        }
        else if( maxRisk >  val) {
            maxRisk = val;
        }

    }
    return maxRisk;

}








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


    delete nRMatch;
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
    int dist = par("dropOffNodeDist").doubleValue();
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
        int rowT = c.first;
        int colT = c.second;
  /*      EV<<"RowT"<<rowT<<" ColT"<<colT<<endl;
        EV<<"Dist"<<dist<<endl;
        EV<<"rowT mod dist "<<rowT % dist<<endl;
        EV<<"colT mod dist "<<colT % dist<<endl;*/

        cModule *node = getNodeFromCoords(c.first, c.second);
        // EV<<"Destination Node x"<<node->par("x").longValue()<<" y "<<node->par("y").longValue()<<endl;
        if (i ==  0 || i == coords.size()-1) {
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
                if(node->getIndex() % dist == 0 ) {
                if((*it).second.compare("FirstAid")==0) {
                    cPar &typeId =  node->par("typeId");
                    cPar &type =  node->par("type");
                    typeId.setLongValue((*it).first);
                    type.setStringValue((*it).second);
                }
                }
                    cPar &reqGen = node->par("isRequestGenerator");

                    reqGen.setBoolValue(false);

                    //    EV<<"Destination Node x"<<node->par("x").longValue()<<" y "<<node->par("y").longValue()<<": Type"<<type.stdstringValue()<<endl;
                }

        }
        i++;
    }


   nodeTypes->clear();
   delete nodeTypes;
}



void  AdaptiveNetworkManager::setRedZone() {
    int riskZoneExp = this->getParentModule()->par("riskZoneExp").doubleValue();
    std::vector<std::pair<int,int> > *coords = getCenteredSquare(riskZoneExp);

    std::vector<cTopology::Node *> nodes;
    for(const auto &x : (*coords)) {
        cModule *n = getNodeFromCoords(x.first, x.second);
        nodes.push_back(getNodeByID(n->getId()));
    }

     for(int i = 0; i<nodes.size(); i++) {

        cTopology::Node *n = nodes[i];



        for(int j = 0; j<n->getNumOutLinks(); j++) {
               cTopology::LinkOut* srcLinkOut = n->getLinkOut(j);

               for(int k = 0; k<nodes.size(); k++) {
                   cTopology::Node *nT = nodes[k];

                   if(srcLinkOut->getRemoteNode()->getModuleId() ==  nT->getModuleId() && nT->getModuleId() != n->getModuleId() ) {
                  cDisplayString &s  = srcLinkOut->getLocalGate()->getTransmissionChannel()->getDisplayString();
                  s.parse("ls=red");

                   cPar &riskLevel = srcLinkOut->getLocalGate()->getTransmissionChannel()->par("riskLevel");
                   riskLevel.setDoubleValue(1.0);

                   EV<<"Risk Level increased to "<<riskLevel.doubleValue()<< "for connection"<< n->getModule()->getFullName()<<"-->"<<nT->getModule()->getFullName()<<endl;

               }

           }

        }
     }
     coords->clear();
     delete coords;

}





/**
 * Ritorna i nodi di drop-off di una certa tipologia.
 */

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


/**
 * Ritorna i nodi di drop-off
 */

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

    delete nodeTypes;
    return destNodes;
}




/*
 * Emula la rottura di un canale previa verifica di fattibilità valutando se risulta ancora
 * possibile, dopo la rottura, raggungere l'esterno della zona rossa verificando se da un nodo esterno è ancora possibile
 * raggiungere il nodo destinazione e che dal nodo sorgente è ancora possibile raggiungere lo stesso nodo esterno.
 *
 */

bool AdaptiveNetworkManager::checkAndSetMaxRiskAndWeight(cTopology::Node *srcNode, cTopology::Node *destNode) {


    //std::vector<cModule *> destNodes =  getAllDestinationNodes();
    bool isMaxRiskN = false;
    int numLinks = 0;
    int maxRisk = getMaxRisk();
    double weight;
    double riskLevel;
    bool isGoodP = true;

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


    std::vector<cModule *> destNodes = getAllDestinationNodes();

/*
    for(int i = 0; i<destNodes.size(); i++) {


        isGoodP = checkForGoodPath(destNodes[i]->par("address"), destNode->getModule()->par("address"));
        isGoodP = checkForGoodPath(srcNode->getModule()->par("address"), destNodes[i]->par("address"));
        if(isGoodP)
            break;
    }

    if(isGoodP)
        return isGoodP;
*/
    cModule *dNode = destNodes[0];


       cTopology::LinkOut *linkOut;
   // for(int i = 0; i<destNodes.size(); i++) {
        int idNode = indexTable[dNode->par("address")];
        calculatePath(dNode->par("address"),destNode->getModule()->par("address"));
        linkOut = topo->getNode(idNode)->getPath(0);
        while(linkOut!= NULL) {
            if(linkOut->getWeight()  == maxRisk*MAX_DELAY) {
                isGoodP = false;
                break;
            }
            linkOut = linkOut->getRemoteNode()->getPath(0);
        }
   // }


        // verifica se il nodo sorgente non deve necesariamente passare da quel link per raggiungere l'esterno della zona rossa

              calculatePath(srcNode->getModule()->par("address"),dNode->par("address"));
              linkOut = srcNode->getPath(0);
              while(linkOut!= NULL) {
                  if(linkOut->getWeight()  == maxRisk*MAX_DELAY) {
                      isGoodP = false;
                      break;
                  }
                  linkOut = linkOut->getRemoteNode()->getPath(0);
              }




    if(!isGoodP) {
        setRiskLevel(srcNode, destNode, riskLevel);
        updateChannelDelay(srcNode, destNode);
        updateWeight(srcNode, destNode);
        //  setRiskLevel(srcNode, destNode, riskLevel);

        //setWeight(destNode, srcNode, weight);
    }
return isGoodP;

}








bool AdaptiveNetworkManager::checkChannelDropFeasability(cTopology::Node *srcNode, cTopology::Node *destNode) {


    //std::vector<cModule *> destNodes =  getAllDestinationNodes();
    bool isMaxRiskN = false;
    int numLinks = 0;
    int maxRisk = getMaxRisk();
    double weight;
    double riskLevel;
    bool isGoodP = true;

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


    std::vector<cModule *> destNodes = getAllDestinationNodes();

/*
    for(int i = 0; i<destNodes.size(); i++) {


        isGoodP = checkForGoodPath(destNodes[i]->par("address"), destNode->getModule()->par("address"));
        isGoodP = checkForGoodPath(srcNode->getModule()->par("address"), destNodes[i]->par("address"));
        if(isGoodP)
            break;
    }

    if(isGoodP)
        return isGoodP;
*/
    cModule *dNode = destNodes[0];


       cTopology::LinkOut *linkOut;
   // for(int i = 0; i<destNodes.size(); i++) {
        int idNode = indexTable[dNode->par("address")];
        calculatePath(dNode->par("address"),destNode->getModule()->par("address"));
        linkOut = topo->getNode(idNode)->getPath(0);
        while(linkOut!= NULL) {
            if(linkOut->getWeight()  == maxRisk*MAX_DELAY) {
                isGoodP = false;
                break;
            }
            linkOut = linkOut->getRemoteNode()->getPath(0);
        }
   // }


        // verifica se il nodo sorgente non deve necesariamente passare da quel link per raggiungere l'esterno della zona rossa

              calculatePath(srcNode->getModule()->par("address"),dNode->par("address"));
              linkOut = srcNode->getPath(0);
              while(linkOut!= NULL) {
                  if(linkOut->getWeight()  == maxRisk*MAX_DELAY) {
                      isGoodP = false;
                      break;
                  }
                  linkOut = linkOut->getRemoteNode()->getPath(0);
              }





        setRiskLevel(srcNode, destNode, riskLevel);
        updateChannelDelay(srcNode, destNode);
        updateWeight(srcNode, destNode);
        //  setRiskLevel(srcNode, destNode, riskLevel);

        //setWeight(destNode, srcNode, weight);

return isGoodP;

}













void AdaptiveNetworkManager::handleMessage(cMessage *msg)
{
    int seed = 3;
    double maxRiskChann = getParentModule()->par("maxNumDroppedChannels");
    double dropChRate = getParentModule()->par("dropChRate");

    EV << "Scheduling new message  : "<< endl;
    if (!msg->isSelfMessage())
        error("This module does not process messages.");
    if(msg == updateWeightMessage) {
        EV << "UpdateConnectionsMessage arrived  : "<< endl;
    //    int sed = intuniform(0, 3, 2);    //perchè messo qui da errore???
       if(MAX_TRY < maxRiskChann) {
            std::pair<int, int> nodes = getCenteredSquareRndLinkedNodes(3);
            if(nodes.second != -1)  {
            cTopology::Node * sNode= topo->getNode(indexTable[nodes.first]);
            cTopology::Node * dNode= topo->getNode(indexTable[nodes.second]);
           // bool cm = checkAndSetMaxRiskAndWeight(sNode, dNode);
        //    bool cm2 =checkAndSetMaxRiskAndWeight(dNode, sNode);
            bool cm = checkChannelDropFeasability(sNode, dNode);
            bool cm2 = checkChannelDropFeasability( dNode, sNode);
            if(cm && cm2) {
                setRiskLevel(sNode, dNode, getMaxRisk());
                updateChannelDelay(sNode, dNode);
                updateWeight(sNode, dNode);
                setRiskLevel(dNode, sNode, getMaxRisk());
                updateChannelDelay(dNode, sNode);
                updateWeight(dNode, sNode);
                MAX_TRY++;

            }


           // emit(updateSchedulingS,new std::string("hk"));
          //  tcoord->updateAllScheduling();


            }
            scheduleAt(simTime()+dropChRate, updateWeightMessage);
        }



    }




}




cTopology::Node* AdaptiveNetworkManager::getNodeByID(int idx) {

       for(int i=0; i<topo->getNumNodes(); i++) {
           cTopology::Node* n = topo->getNode(i);
         //  EV << "Node idx" << i <<endl;
           if(n->getModuleId() == idx)
               return n;
       }
       return NULL;

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

/**
 * Identifica i nodi di pick-up
 */

void AdaptiveNetworkManager::setPickUpNodes() {
    int riskZoneExp = this->getParentModule()->par("riskZoneExp").doubleValue();
    std::vector<std::pair<int,int> > *coords = getCenteredSquare(riskZoneExp);
    for(const auto &x : (*coords)) {
           //   EV<<"COORDS "<<x.first<<" "<<x.second;
           cModule *node = getNodeFromCoords(x.first, x.second);
           node->par("typeId").setDoubleValue(0);
           node->par("type").setStringValue("PickUp");
           node->par("isRequestGenerator").setBoolValue(true);
    }
    coords->clear();
    delete coords;


}




/**
 * Ritorna gli indirizzi di due nodi direttamente connessi all'interno della zona rossa
 * presi in modo casuale
 */

    std::pair<int, int> AdaptiveNetworkManager::getCenteredSquareRndLinkedNodes(int seed) {
          int riskZoneExp = this->getParentModule()->par("riskZoneExp").doubleValue();
        //  int maxRisk = getMaxRisk();
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
          int indexNode = intuniform(0, nodes.size()-1,seed);
          cTopology::Node *sNode = nodes[indexNode];
          int indexLink;
          cTopology::LinkOut* sLinkOut;
          cTopology::Node *dNode;
          int linkNum=0;

         // indexLink = intuniform(0, sNode->getNumOutLinks()-1,seed);
         // sLinkOut = sNode->getLinkOut(indexLink);
          std::vector<int> sNodeOutLinks(0);
          int numLinks = sNode->getNumOutLinks();

          for(int i = 0; i<numLinks; i++) {
              sNodeOutLinks.push_back(i);
          }

          std::random_shuffle(sNodeOutLinks.begin(), sNodeOutLinks.end());
          for(int i = 0; i<numLinks; i++) {
             // sLinkOut = sNode->getLinkOut(i);
              sLinkOut = sNode->getLinkOut(sNodeOutLinks[i]);

          int delay = sLinkOut->getLocalGate()->getTransmissionChannel()->par("delay").doubleValue();
                    if(delay < MAX_DELAY) {
                    for(int k = 0; k<nodes.size(); k++) {
                        cTopology::Node *nT = nodes[k];
                        if(sLinkOut->getRemoteNode()->getModuleId() == nT->getModuleId()) {


                            dNode = nT;
                            return std::make_pair(sNode->getModule()->par("address").doubleValue(), dNode->getModule()->par("address").doubleValue());
                           // break;
                            }
                          //  dNode = nT;
                        }

                }
          }





                  return std::make_pair(-1,-1);

            //  }

         // }

        // return std::make_pair(-1,-1);
       // return std::make_pair(sNode->getModule()->par("address").doubleValue(), dNode->getModule()->par("address").doubleValue());
      }


/**
 * Ritorna la lista degli indirizzi dei nodi presenti nella zona rossa.
 */

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


/**
 * Ritorna l'indirizzo di destinazione più vicino in base a l'indirizzo sorgente e al tipo
 * della richiesta
 */

int AdaptiveNetworkManager::getCloserValidDestinationAddress(int srcAddress, int requestTypeId) {

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

    nRMatch->clear();
    delete nRMatch;
    return nodeCloser->par("address").doubleValue();



}







