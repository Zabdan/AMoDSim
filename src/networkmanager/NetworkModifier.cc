//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include <NetworkModifier.h>

Define_Module(NetworkModifier);



NetworkModifier::NetworkModifier() {
    // TODO Auto-generated constructor stub


}



NetworkModifier::~NetworkModifier() {
    // TODO Auto-generated destructor stub

}



void NetworkModifier::initialize()
{
    topo = new cTopology("topo");
    delay = 0.01;
    ber = 0;
    datarate = 1e6;
    netmanager = check_and_cast<AbstractNetworkManager *>(getParentModule()->getSubmodule("netmanager"));
    timeLimit = &par("timeLimit");
    updateConnectionsMessage = new cMessage();
    std::vector<std::string> nedTypes;
    nedTypes.push_back("src.node.Node");
    topo->extractByNedTypeName(nedTypes);
    setRiskZone();
   // scheduleAt(0, updateConnectionsMessage);
}

/*

void NetworkModifier::handleMessage(cMessage *msg)
{
    EV << "Scheduling new message"<< endl;
    if (!msg->isSelfMessage())
        error("This module does not process messages.");

    if(msg !=  updateConnectionsMessage) {
      error("Wrong message!");
    }
    else {
        EV << "UpdateConnectionsMessage arrived! "<< endl;

        rndChangeConnectionsRiskLevel();

    }


     scheduleAt(simTime()+10, updateConnectionsMessage);

}
*/

/*

void NetworkModifier::connect(cGate *src, cGate *dest)
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

*/
/*
int NetworkModifier::changeNodesConnectionRiskLevel ( int srcNodeIdx, int destNodeIdx, int count) {
    if(srcNodeIdx == destNodeIdx)
        return -1;
    cTopology::Node* srcNode = getNodeById(srcNodeIdx);
    cTopology::Node* destNode = getNodeById(destNodeIdx);

    for(int j = 0; j<srcNode->getNumOutLinks(); j++) {
        cTopology::LinkOut* srcLinkOut = srcNode->getLinkOut(j);
        if(srcLinkOut->getRemoteGate()->getOwnerModule() == destNode->getModule()) {
           cDisplayString &s  = srcLinkOut->getLocalGate()->getTransmissionChannel()->getDisplayString();
           s.parse("ls=red");

            cPar &riskLevel = srcLinkOut->getLocalGate()->getTransmissionChannel()->par("riskLevel");
            riskLevel.setDoubleValue(1.0);

            EV<<"Risk Level increased to "<<riskLevel.doubleValue()<< "for connection"<< srcNode->getModule()->getFullName()<<"-->"<<destNode->getModule()->getFullName()<<endl;
            count++;
        }

    }
    if(count == 1) {
        changeNodesConnectionRiskLevel(destNodeIdx, srcNodeIdx, count);
        return 0;
    }

    if(count == 2) {
        return 0;
    }

    return -1;
}


*/





void  NetworkModifier::setRiskZone() {
    int riskZoneExp = this->getParentModule()->par("riskZoneExp").doubleValue();
    std::vector<std::pair<int,int> > *coords = netmanager->getCenteredSquare(riskZoneExp);

    std::vector<cTopology::Node *> nodes;
    for(const auto &x : (*coords)) {
        EV<<"COORDS "<<x.first<<" "<<x.second;
        cModule *n = netmanager->getNodeFromCoords(x.first, x.second);
        nodes.push_back(getNodeById(n->getId()));
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





}




/*
int NetworkModifier::disconnectNodes( int srcNodeIdx, int destNodeIdx, int count) {
    if(srcNodeIdx == destNodeIdx)
        return -1;
    cTopology::Node* srcNode = getNodeById(srcNodeIdx);
    cTopology::Node* destNode = getNodeById(destNodeIdx);

               for(int j = 0; j<srcNode->getNumOutLinks(); j++) {
                         cTopology::LinkOut* srcLinkOut = srcNode->getLinkOut(j);
                                 if(srcLinkOut->getRemoteGate()->getOwnerModule() == destNode->getModule() &&  srcLinkOut->getLocalGate()->isConnected() ) {
                                     srcLinkOut->getLocalGate()->disconnect();
                                     EV<<"New disconnection "<< srcNode->getModule()->getFullName()<<"-\\->"<<destNode->getModule()->getFullName()<<endl;
                                     count++;
                                 }

                                 }
                                if(count == 1) {
                                   disconnectNodes(destNodeIdx, srcNodeIdx, count);
                                   return 0;
                                 }

                                 if(count == 2) {
                                     return 0;
                                 }

               return -1;
}



int NetworkModifier::connectNodes( int srcNodeIdx, int destNodeIdx, int count) {
    if(srcNodeIdx == destNodeIdx)
        return -1;
    cTopology::Node* srcNode = getNodeById(srcNodeIdx);
    cTopology::Node* destNode = getNodeById(destNodeIdx);

               for(int j = 0; j<srcNode->getNumOutLinks(); j++) {
                         cTopology::LinkOut* srcLinkOut = srcNode->getLinkOut(j);
                                 if(srcLinkOut->getRemoteGate()->getOwnerModule() == destNode->getModule()) {
                                     if(!srcLinkOut->getLocalGate()->isConnected())
                                         connect(srcLinkOut->getLocalGate(), srcLinkOut->getRemoteGate());
                                         EV<<"New connection "<< srcNode->getModule()->getFullName()<<"-->"<<destNode->getModule()->getFullName()<<endl;
                                     count++;
                                 }

                                 }
                                if(count == 1) {
                                   connectNodes(destNodeIdx, srcNodeIdx, count);
                                   return 0;
                                 }

                                 if(count == 2) {
                                     return 0;
                                 }

               return -1;
}


*/

/*


std::vector<int> *NetworkModifier::getNodesId() {
    std::vector<int> *l = new std::vector<int>(0);
    for(int i=0; i<topo->getNumNodes(); i++) {
              cTopology::Node* n = topo->getNode(i);
               l->push_back(n->getModuleId());
    }
    return l;

}

*/

/*

void NetworkModifier::rndChangeConnections() {

    std::vector<int> *nodesId = getNodesId();
    if(nodesId) {
    int srcNodeId;
    int destNodeId;
    int first = intuniform(0, nodesId->size()-1);
    int second = intuniform(0, nodesId->size()-1);
    if((simTime().dbl() / directionTime->doubleValue()) <1)
       disconnectNodes((*nodesId)[first],(*nodesId)[second]);
    else {
       connectNodes((*nodesId)[first],(*nodesId)[second]);
       if(simTime().dbl() >= directionTime->doubleValue()*2000)
           directionTime->setDoubleValue(simTime().dbl()+directionTime->doubleValue());
    }
    delete nodesId;
    }


}
*/
/*
void NetworkModifier::rndChangeConnectionsRiskLevel() {

    std::vector<int> *nodesId = getNodesId();
    if(nodesId) {
    int srcNodeId = intuniform(0, nodesId->size()-1);
    int destNodeId= intuniform(0, nodesId->size()-1);
    cTopology::Node *sN = getNodeById((*nodesId)[srcNodeId]);
    cTopology::Node *dN = getNodeById((*nodesId)[destNodeId]);
    cModule *sourceNode = sN->getModule();
    cModule *destNode = dN->getModule();
    bool isSValid = netmanager->isValidDestinationAddress(sourceNode->par("address").longValue());
    bool isDValid = netmanager->isValidDestinationAddress(destNode->par("address").longValue());

    if((simTime().dbl() / timeLimit->doubleValue())<1) {
        while(isSValid || isDValid) {
             srcNodeId = intuniform(0, nodesId->size()-1);
             destNodeId= intuniform(0, nodesId->size()-1);
             sN = getNodeById((*nodesId)[srcNodeId]);
             dN = getNodeById((*nodesId)[destNodeId]);
             sourceNode = sN->getModule();
             destNode = dN->getModule();
             isSValid = netmanager->isValidDestinationAddress(sourceNode->par("address").longValue());
             isDValid = netmanager->isValidDestinationAddress(destNode->par("address").longValue());


        }
        changeNodesConnectionRiskLevel((*nodesId)[srcNodeId],(*nodesId)[destNodeId]);


    }
    }

    delete nodesId;
}

*/






cTopology::Node* NetworkModifier::getNodeById(int idx) {

       for(int i=0; i<topo->getNumNodes(); i++) {
           cTopology::Node* n = topo->getNode(i);
         //  EV << "Node idx" << i <<endl;
           if(n->getModuleId() == idx)
               return n;
       }
       return NULL;

   }




