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

#ifndef HEURISTICCOORD_H_
#define HEURISTICCOORD_H_

#include <BaseCoord.h>
#include "StopPointOrderingProposal.h"

class HeuristicCoord: public BaseCoord {

private:

    int redCodesReallocCount = 0;

    std::map<int,int> numReallocPerRedCode;


    void updateScheduling(int vehicleID);
    std::list<StopPointOrderingProposal*> addStopPointToTrip(int vehicleID, std::list<StopPoint*> oldTrip, StopPoint* newSP, int dropOffLocation);

    StopPointOrderingProposal* addRequestToFirstPos(int vehicleID, std::list<StopPoint*> spl, TripRequest* tr);
    std::list<double> getResidualTime(std::list<StopPoint*> spl, int requestID);
protected:
    void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj) override;
 //   void handleTripRequest(TripRequest *tr) override;
  //  StopPointOrderingProposal* eval_requestAssignment(int vehicleID, TripRequest* newTR) override;

    int handleTripRequest(TripRequest *tr, bool isReAllocation) override;
    StopPointOrderingProposal* eval_requestAssignment(int vehicleID, TripRequest* newTR, bool isR)  override;
    std::list<TripRequest*> checkForRelocation(int vehicleID);
    bool checkForSaturation(int vehicleID);
   // int getMinTInSPList( std::list<StopPoint*> spList);
  //  int getMaxTInSPList( std::list<StopPoint*> spList);



public:
    void updateScheduling(int vehicleID, int actVehicleLoc) override;


};

#endif /* HEURISTICCOORD_H_ */
