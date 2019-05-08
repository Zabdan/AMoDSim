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

#ifndef __AMOD_SIMULATOR_NETWORKMANAGER_H_
#define __AMOD_SIMULATOR_NETWORKMANAGER_H_

#include <omnetpp.h>
#include <AbstractNetworkManager.h>
#include<iostream>
#include<fstream>
#include<algorithm>

class AdaptiveNetworkManager : public AbstractNetworkManager
{
private:
    int rows;
    int columns;
    double MAX_TRY;
    double MAX_DELAY;
    cTopology* topo;
    cMessage* updateWeightMessage;
    std::map<int,int> indexTable;
    std::map<int, std::map<int,int>> rtable;     //RoutingTable
    //std::map<int, std::map<int,int>> *rtable = new std::map<int, std::map<int,int>>();     //RoutingTable
    std::map<int, std::map<int,double>> *dtable = new std::map<int, std::map<int,double>>();  //Time-Distance table
    std::map<int, std::map<int,double>> *sdtable = new std::map<int, std::map<int,double>>(); //Space-Distance table
    std::map<int, std::map<int,double>> cltable; //Channel length table

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual double timeDistanceToTarget(cTopology::Node *thisNode);
    virtual double spaceDistanceToTarget(cTopology::Node *thisNode);
    virtual void updateTables( int destAddress);
  //  virtual void updateAllTables();


    virtual bool checkForGoodPath(int srcAddr, int dstAddr);
    virtual cTopology::Node *calculatePath(int srcAddr, int destAddr);
    virtual void setWeight(cTopology *topo);

    virtual void setRiskLevel(cTopology::Node *srcNode, cTopology::Node *destNode, double riskLevel);
    virtual void setAllWeight();
   // virtual void setMaxRiskAndWeight(cTopology::Node*srcNode, cTopology::Node*destNode);
  //  virtual void setWeight(cTopology::Node *srcNode, cTopology::Node *destNode, double weight);
    virtual void updateWeight(cTopology::Node *srcNode, cTopology::Node *destNode);
    virtual void updateChannelDelay(cTopology::Node *srcNode, cTopology::Node *destNode);
    virtual int getMaxRisk();
    virtual  bool checkAndSetMaxRiskAndWeight(cTopology::Node *srcNode, cTopology::Node *destNode);
  //  virtual void readAllNodeTypes(std::map<int, std::string> *nodeTypes) override;
  //  virtual void readAlldestNodesRequestsMatching(std::map<int, int> *nRMatch) override;
    virtual std::vector<cModule *> getAllDestinationNodes(int nodeTypeId) override;
    std::vector<cModule *> getAllDestinationNodes();
//    virtual double calculateEccentricity(int srcAddr);
  //  virtual double calculateEccMax(std::map<cModule *, double> *nodesEcc);
    virtual void setDropOffNodes() override;
    virtual void setPickUpNodes();
    //virtual std::pair< cTopology::Node*, cTopology::Node*> getCenteredSquareRndLinkedNodes(int seed);
    virtual std::pair< int, int> getCenteredSquareRndLinkedNodes(int seed);
    void setRedZone();
    cTopology::Node* getNodeByID(int idx);




    // Net manipulation methods
  /*  virtual  void destroyConnections();
    virtual  cTopology::Node* getNodeByIdx(int id);
    virtual  bool existDirectConnection(int srcNodeId, int destNodeId);
    virtual  cChannel* checkForChannel();
    virtual  void updateTopology();
    virtual  void connectNodes(cGate *src, cGate *dest, double delay, double ber, double datarate);
    virtual  int disconnectNodes( int srcNodeId, int destNodeId, int count);
    virtual  void rndDisconnection();*/


  public:
    virtual ~AdaptiveNetworkManager();

    virtual double getTimeDistance(int srcAddr, int dstAddr) override;
    virtual double getSpaceDistance(int srcAddr, int dstAddr) override;
    virtual double getChannelLength(int nodeAddr, int gateIndex) override;
    virtual int getOutputGate(int srcAddr, int destAddr) override;
    virtual int getVehiclesPerNode(int nodeAddr) override;
    virtual bool isValidAddress(int nodeAddr) override;
    virtual bool isValidDestinationAddress(int requestTypeId,int destAddr) override;
    virtual bool isValidDestinationAddress(int destAddr) override;
 //   virtual int getValidDestinationAddress(int requestTypeId) override;
    virtual int getCloserValidDestinationAddress(int srcAddress, int requestTypeId)override;
    virtual cModule*getNodeFromCoords(int x, int y) override;
    virtual std::vector<std::pair<int,int>> *getCenteredSquare(int mult) override;
};

#endif
