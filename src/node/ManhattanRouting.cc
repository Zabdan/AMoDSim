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

    EV << "I am node " << myAddress << ". My X/Y are: " << myX << "/" << myY << endl;
}

void ManhattanRouting::handleMessage(cMessage *msg)
{
    Vehicle *pk = check_and_cast<Vehicle *>(msg);
    int destAddr = pk->getDestAddr();

    //If this node is the destination, forward the vehicle to the application level
    if (destAddr == myAddress)
    {
        EV << "Vehicle arrived in the stop point " << myAddress << ". Traveled distance: " << pk->getTraveledDistance() << endl;
        send(pk, "localOut");
        return;
    }

    bool noAvailableRoute = false;
    int distance;
    int outGateIndex;
    int destX = pk->getDestAddr() % rows;
    int destY = pk->getDestAddr() / rows;


    bool gateZeroConnected = isConnectedGate(0);
    bool gateOneConnected = isConnectedGate(1);
    bool gateTwoConnected = isConnectedGate(2);
    bool gateThreeConnected = isConnectedGate(3);



    if(gateZeroConnected || gateOneConnected || gateTwoConnected || gateThreeConnected  ) {
    if(myX < destX ||  myX > destX) {

        if(myX < destX &&  gateTwoConnected)
           {


               outGateIndex = 2; //right

               distance = xChannelLength;
           }
           else
               if(myX > destX &&  gateThreeConnected)
               {
                   outGateIndex = 3; //left
                   distance = xChannelLength;
               }
           else
               if(myY < destY && gateZeroConnected)
               {
                   outGateIndex = 0; //sud
                   distance = yChannelLength;
               }
               else
               {
                   if(gateOneConnected) {
                   outGateIndex = 1; //north
                   distance = yChannelLength;
                   }
               }


        }

    else  if(myY < destY ||  myY > destY) {


        if(myY < destY && gateZeroConnected)
        {
            outGateIndex = 0; //sud
            distance = yChannelLength;
        }
        else  if(myY > destY && gateOneConnected)
        {
            outGateIndex = 1; //north
            distance = yChannelLength;
        }
        else if(myX < destX && gateTwoConnected)
        {

            outGateIndex = 2; //right

            distance = xChannelLength;
        }
        else {
            if(gateThreeConnected) {
            outGateIndex = 3; //left
            distance = xChannelLength;
            }
        }

    }
    }
    else  {
        noAvailableRoute = true;
        EV <<"NO AVAILABLE ROUTE FOR DESTINATION: "<<pk->getDestAddr()<<endl;
        if (ev.isGUI()) getParentModule()->bubble("NO AVAILABLE ROUTE!");

    }
     if(!noAvailableRoute) {
    pk->setHopCount(pk->getHopCount()+1);
    pk->setTraveledDistance(pk->getTraveledDistance() + distance);

    //send the vehicle to the next node
    /**
    if(outGateIndex > 0)
    */


        send(pk, "out", outGateIndex);


     }



}




bool ManhattanRouting::isConnectedGate(int outGateIndex) {

    cModule *node = getParentModule();
    for(cModule::GateIterator i(node); !i.end(); i++) {
           cGate *gate = i();
           if(gate->getType()==cGate::OUTPUT && gate->isConnected() && gate->getIndex() == outGateIndex) {
               EV <<"Gate founded!"<<endl;
               return true;

           }
    }

     return false;

}
