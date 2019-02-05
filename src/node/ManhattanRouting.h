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

#ifndef __AMOD_SIMULATOR_MANHATTANROUTING_H_
#define __AMOD_SIMULATOR_MANHATTANROUTING_H_

#include <omnetpp.h>
#include "ManhattanNetworkManager.h"

class ManhattanRouting : public cSimpleModule
{
private:
    AbstractNetworkManager* netmanager;
    int myAddress;
    int myX;
    int myY;
    int rows;
    int columns;
    double xChannelLength;
    double yChannelLength;

  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
 //   virtual bool isConnectedGate(int outGateIndex, int indexNode);
 //   virtual bool isMinRiskLevelRoute(int outGateIndex);
//    virtual bool isAllEqualRiskLevelRoute();
};

#endif
