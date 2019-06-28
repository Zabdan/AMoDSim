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

#ifndef RADIOTAXICOORD_H_
#define RADIOTAXICOORD_H_

#include <BaseCoord.h>

class RadioTaxiCoord: public BaseCoord{

protected:
        void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj) override;
     //   void handleTripRequest(TripRequest *tr) override;
     //   StopPointOrderingProposal* eval_requestAssignment(int vehicleID, TripRequest* newTR) override;
        StopPointOrderingProposal* addStopPointToFirstPos(int vehicleID, std::list<StopPoint*> spl, TripRequest* tr);
        int getLastPos(int firstProb, std::list<StopPoint*> spList);
        int getPrecProb(int prob, std::list<StopPoint*> spList);
        int getNextProb(int prob, std::list<StopPoint*> spList);

        int handleTripRequest(TripRequest *tr, bool isReAllocation) override;
        StopPointOrderingProposal* eval_requestAssignment(int vehicleID, TripRequest* newTR, bool isR)  override;


public:
    void updateScheduling(int vehicleID, int actVehicleLoc) override;
};

#endif /* RADIOTAXICOORD_H_ */
