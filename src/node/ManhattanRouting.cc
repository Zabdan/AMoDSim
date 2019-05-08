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

#include "ManhattanRouting.h"
#include "Vehicle.h"


Define_Module(ManhattanRouting);

void ManhattanRouting::initialize()
{

    myAddress = getParentModule()->par("address");
    myX = getParentModule()->par("x");
    myY = getParentModule()->par("y");
    rows = getParentModule()->getParentModule()->par("width");
    columns = getParentModule()->getParentModule()->par("height");

    xChannelLength = getParentModule()->getParentModule()->par("xNodeDistance");
    yChannelLength = getParentModule()->getParentModule()->par("yNodeDistance");
    netmanager = check_and_cast<AbstractNetworkManager *>(getParentModule()->getParentModule()->getSubmodule("netmanager"));

    tcoord = check_and_cast<BaseCoord *>(getParentModule()->getParentModule()->getSubmodule("tcoord"));
    EV << "I am node " << myAddress << ". My X/Y are: " << myX << "/" << myY << endl;
}

void ManhattanRouting::handleMessage(cMessage *msg)
{
    Vehicle *pk = check_and_cast<Vehicle *>(msg);
    int destAddr = pk->getDestAddr();



    //DEBUG TEST
    /*    if(simTime().dbl() > 17900 && myAddress == 21){
                EV<<"Pk stopped!"<<endl;
                tcoord->printSPListInfo(3);
                return;
            }*/



    //If this node is the destination, forward the vehicle to the application level
    if (destAddr == myAddress)
    {

        EV << "Vehicle arrived in the stop point " << myAddress << ". Traveled distance: " << pk->getTraveledDistance() << endl;

      //    else {

        send(pk, "localOut");
        return;
       //   }
    }
    if(msg->isSelfMessage())   {
      //  if (ev.isGUI()) getParentModule()->bubble("RETRY TO SEND pk!");
    }


    bool noAvailableRoute = false;
    int distance;
    int outGateIndex;
    int destX = pk->getDestAddr() % rows;
    int destY = pk->getDestAddr() / rows;

    EV <<"Pk dest address "<<pk->getDestAddr()<<"  destX"<<destX<< " destY"<<destY<<endl;



        int gIndex = netmanager->getOutputGate(myAddress, pk->getDestAddr());
        distance = netmanager->getChannelLength(myAddress,gIndex);



    pk->setHopCount(pk->getHopCount()+1);
    pk->setTraveledDistance(pk->getTraveledDistance() + distance);



    //send the vehicle to the next node

        send(pk, "out", gIndex);

   // }




}




