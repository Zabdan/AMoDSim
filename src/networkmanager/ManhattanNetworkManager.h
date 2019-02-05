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

#ifndef __AMOD_SIMULATOR_MANHATTANNETWORKMANAGER_H_
#define __AMOD_SIMULATOR_MANHATTANNETWORKMANAGER_H_

#include <omnetpp.h>
#include <AbstractNetworkManager.h>
#include<iostream>
#include<fstream>

class ManhattanNetworkManager : public AbstractNetworkManager
{
private:
    int rows;
    int columns;

    double xChannelLength;
    double yChannelLength;
    double xTravelTime;
    double yTravelTime;



protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void setDropOffNodes() override;
    virtual cModule * getNodeFromCoords(int x, int y);
  //  virtual void readAlldestNodesRequestsMatching(std::map<int, int> *nRMatch) override;
  //  virtual void readAllNodeTypes(std::map<int, std::string> *nodeTypes) override;
    virtual std::vector<cModule *> * getAllDestinationNodes(int nodeTypeId) override;
    virtual void setWeight(cTopology *topo);
    virtual cModule *getNodeFromAddress(int address);
    cTopology::Node *calculatePath(int srcAddr, int destAddr);


  public:
    virtual double getTimeDistance(int srcAddr, int dstAddr) override;
    virtual double getSpaceDistance(int srcAddr, int dstAddr) override;
    virtual double getChannelLength(int nodeAddr, int gateIndex) override;
    virtual int getOutputGate(int srcAddr, int destAddr) override;
    virtual int getVehiclesPerNode(int nodeAddr) override;
    virtual bool isValidAddress(int nodeAddr) override;
    virtual bool isValidDestinationAddress(int requestTypeId,int destAddr) override;
    virtual bool isValidDestinationAddress(int destAddr) override;
    virtual int getValidDestinationAddress(int requestTypeId) override;
    virtual std::vector<std::pair<int,int>> *getCenteredSquare(int mult) override;


};

#endif
