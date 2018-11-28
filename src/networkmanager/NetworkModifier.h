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

#ifndef NETWORKMODIFIER_H_
#define NETWORKMODIFIER_H_


#include <string.h>
#include <omnetpp.h>
#include <vector>


class NetworkModifier : public cSimpleModule{

private:
    cMessage *updateConnectionsMessage;
    cTopology* topo = new cTopology("topo");
    double delay = 0.01;
    double ber = 0;
    double datarate = 1e6;


protected:

   virtual void initialize();
   virtual void handleMessage(cMessage *msg);
   virtual void connect(cGate *src, cGate *dest);
   virtual cTopology::Node* getNodeById(int idx);
 //  cModule* getNodeByName(const char *node);
   virtual int disconnectNodes( int srcNodeId, int destNodeId, int count=0);
   virtual int connectNodes( int srcNodeId, int destNodeId, int count=0);
   virtual void rndChangeConnections();
   virtual std::vector<int> *getNodesId();
   virtual int disconnectFromAllNodes(int srcNodeId);



public:

     NetworkModifier();
    virtual ~NetworkModifier();
};

#endif /* NETWORKMODIFIER_H_ */
