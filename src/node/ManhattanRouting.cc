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

       // cGate * g = netmanager->getGateToDestination(myAddress, pk->getDestAddr());




   // else {


        int gIndex = netmanager->getOutputGate(myAddress, pk->getDestAddr());
        distance = netmanager->getChannelLength(myAddress,gIndex);
/*
    int startIndexToCheck = getParentModule()->getIndex();
    EV<<"Staring Node x"<< myX<<" y "<<myY<<endl;

    bool noAssignedGate = true;



        if(myX < destX )
           {
              if(isMinRiskLevelRoute(2) || isAllEqualRiskLevelRoute()  || myY == destY) {

               outGateIndex = 2; //right

               distance = xChannelLength;
               noAssignedGate = false;
               }
           }

        else if(myX > destX )
               {
           if(isMinRiskLevelRoute(3) || isAllEqualRiskLevelRoute() || myY == destY) {
                   outGateIndex = 3; //left
                   distance = xChannelLength;
                   noAssignedGate = false;
            }
               }

             if(myY < destY && noAssignedGate)
               {

                   outGateIndex = 0; //sud
                   distance = yChannelLength;

               }
               else if(myY > destY && noAssignedGate)
               {
                   outGateIndex = 1; //north
                   distance = yChannelLength;

               }

*/



    pk->setHopCount(pk->getHopCount()+1);
    pk->setTraveledDistance(pk->getTraveledDistance() + distance);



    //send the vehicle to the next node

        send(pk, "out", gIndex);

   // }




}



/*

bool ManhattanRouting::isMinRiskLevelRoute(int outGateIndex) {
   // double minRisk = -1;
    std::vector<int> allMin;
    int indexGate = -1;
    int j = 0;
    bool allEqual = true;
    double minRisk = -1;
    cModule *node = getParentModule();
       for(cModule::GateIterator i(node); !i.end(); i++) {
              cGate *gate = i();
              if(gate->getType()==cGate::OUTPUT && gate->isConnected()) {
                  EV<<"GATE NAME END "<<gate->getFullName()<< "INDEX "<<gate->getIndex()<<endl;
                  cChannel *c = gate->getTransmissionChannel();

                  double riskLevel = c->par("riskLevel").doubleValue();

                  if(j == 0 || minRisk > riskLevel) {
                      minRisk = riskLevel;
                      indexGate =  gate->getIndex();
                  }

                  j++;
              }
       }
       if(node->gate("port$o", outGateIndex)->getTransmissionChannel()->par("riskLevel").doubleValue() == minRisk)
           return true;
            return false;

   }



bool ManhattanRouting::isAllEqualRiskLevelRoute() {

    int j = 0;
    double risk = -1;
    bool allEqual = true;
    cModule *node = getParentModule();
       for(cModule::GateIterator i(node); !i.end(); i++) {
              cGate *gate = i();
              if(gate->getType()==cGate::OUTPUT && gate->isConnected()) {

                  cChannel *c = gate->getTransmissionChannel();
                  double riskLevel = c->par("riskLevel").doubleValue();
                  EV<< "RISK level "<< riskLevel<<endl;
                  if(j == 0) {
                      risk = riskLevel;
                  }
                  if(allEqual) {
                      if(!(risk == riskLevel)) {
                          allEqual = false;
                          EV<< "NOT ALL Equal "<<endl;
                      }
                  }
                  j++;
              }
       }

    return allEqual;

   }

*/

/*
bool ManhattanRouting::isConnectedGate(int outGateIndex, int indexNode) {

    cModule *node = getParentModule()->getParentModule()->getSubmodule("n", indexNode);
    for(cModule::GateIterator i(node); !i.end(); i++) {
           cGate *gate = i();
           if(gate->getType()==cGate::OUTPUT && gate->isConnected() && gate->getIndex() == outGateIndex) {
               EV <<"Gate founded!"<<endl;
               return true;

           }
    }

     return false;

}
*/
