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



}



NetworkModifier::~NetworkModifier() {


}



void NetworkModifier::initialize()
{

    updateConnectionsMessage = new cMessage();
    std::vector<std::string> nedTypes;
    nedTypes.push_back("src.node.Node");
    topo->extractByNedTypeName(nedTypes);
    scheduleAt(0, updateConnectionsMessage);
}



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

        rndChangeConnections();

    }


     scheduleAt(simTime()+1.5, updateConnectionsMessage);

}




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



/**
 * Disconnect nearby nodes
 */


int NetworkModifier::disconnectNodes( int srcNodeId, int destNodeId, int count) {
    if(srcNodeId == destNodeId)
        return -1;
    cTopology::Node* srcNode = getNodeById(srcNodeId);
    cTopology::Node* destNode = getNodeById(destNodeId);
    if(srcNode == NULL || destNode == NULL)
        return -1;
    for(int j = 0; j<srcNode->getNumOutLinks(); j++) {
        cTopology::LinkOut* srcLinkOut = srcNode->getLinkOut(j);
        if(srcLinkOut->getRemoteGate()->getOwnerModule() == destNode->getModule() &&  srcLinkOut->getLocalGate()->isConnected() ) {
            srcLinkOut->getLocalGate()->disconnect();
            EV<<"New disconnection "<< srcNode->getModule()->getFullName()<<"-\\->"<<destNode->getModule()->getFullName()<<endl;
            count++;
        }

    }
    if(count == 1)
        disconnectNodes(destNodeId, srcNodeId, count);



    return 0;
}



/**
 * Disconnect all nearby nodes from srcNode
 */

int NetworkModifier::disconnectFromAllNodes(int srcNodeId) {
    cTopology::Node* srcNode = getNodeById(srcNodeId);
    if(srcNode == NULL)
        return -1;
    for(int j = 0; j<srcNode->getNumOutLinks(); j++) {
        cTopology::LinkOut* srcLinkOut = srcNode->getLinkOut(j);
        if(srcLinkOut->getLocalGate()->isConnected()) {
            srcLinkOut->getLocalGate()->disconnect();
            EV<<"New disconnection "<< srcNode->getModule()->getFullName()<<"-\\->"<<srcLinkOut->getRemoteGate()->getOwnerModule()->getFullName()<<endl;
        }

    }




    return 0;
}



/**
 * Connect two nearby Nodes
 */

int NetworkModifier::connectNodes( int srcNodeId, int destNodeId, int count) {
    if(srcNodeId == destNodeId)
        return -1;
    cTopology::Node* srcNode = getNodeById(srcNodeId);
    cTopology::Node* destNode = getNodeById(destNodeId);
    if(srcNode == NULL || destNode == NULL)
        return -1;
    for(int j = 0; j<srcNode->getNumOutLinks(); j++) {
        cTopology::LinkOut* srcLinkOut = srcNode->getLinkOut(j);
        if(srcLinkOut->getRemoteGate()->getOwnerModule() == destNode->getModule()) {
            if(!srcLinkOut->getLocalGate()->isConnected()){
                connect(srcLinkOut->getLocalGate(), srcLinkOut->getRemoteGate());
            EV<<"New connection "<< srcNode->getModule()->getFullName()<<"-->"<<destNode->getModule()->getFullName()<<endl;
            }
            count++;
        }

    }
    if(count == 1) {
        connectNodes(destNodeId, srcNodeId, count);
    }



    return 0;
}





std::vector<int> *NetworkModifier::getNodesId() {
    std::vector<int> *l = new std::vector<int>(0);
    for(int i=0; i<topo->getNumNodes(); i++) {
              cTopology::Node* n = topo->getNode(i);
               l->push_back(n->getModuleId());
    }
    return l;

}



/**
 * Execute disconnections and  connections random test
 */

void NetworkModifier::rndChangeConnections() {

    std::vector<int> *nodesId = getNodesId();
    if(nodesId) {
    int srcNodeId;
    int destNodeId;
    int first = intuniform(0, nodesId->size()-1);
    int second = intuniform(0, nodesId->size()-1);
    if(intuniform(0, 2) % 2 ==0)
       disconnectNodes((*nodesId)[first],(*nodesId)[second]);
    else
       connectNodes((*nodesId)[first],(*nodesId)[second]);
    delete nodesId;
    }


}






cTopology::Node* NetworkModifier::getNodeById(int id) {

       for(int i=0; i<topo->getNumNodes(); i++) {
           cTopology::Node* n = topo->getNode(i);
         //  EV << "Node idx" << i <<endl;
           if(n->getModuleId() == id)
               return n;
       }
       return NULL;

   }




