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

#ifndef BASECOORD_H_
#define BASECOORD_H_

#include <omnetpp.h>
#include "TripRequest.h"
#include "Vehicle.h"
#include "VehicleState.h"
#include "AbstractNetworkManager.h"
#include "StopPointOrderingProposal.h"
#include <list>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <limits>

class BaseCoord : public cSimpleModule, cListener{

private:
    virtual void updateVehicleStopPoints(int vehicleID, std::list<StopPoint*> spList, StopPoint *pickupSP);
    virtual void updateVehicleStopPointsRealloc(int vehicleID, std::list<StopPoint*> spList, StopPoint *pickupSP);
    virtual void deleteOutOfTimeSP(int vehicleID);

    protected:
        double totrequests;

        double totRedCodes;
        double totYellowCodes;

        double totInTimeAssignedRedCodes;
        double totInTimeAssignedYellowCodes;

        double totalAssignedRequests;
        double totalPickedupRequests;
        double totalDroppedoffRequest;

        double totalInTimePickedupRedCodes;
        double totalInTimePickedupYellowCodes;
        double totalInTimeDroppedoffRedCodes;
        double totalInTimeDroppedoffYellowCodes;

        double freeVehicles;
        double minTripLength;
        int requestAssignmentStrategy;

        int boardingTime;
        int alightingTime;
        bool requestReallocation;



        AbstractNetworkManager* netmanager;

        //Trip related signals
        simsignal_t tripRequest;
        simsignal_t newTripAssigned;

        //Statistical signals
        simsignal_t traveledDistance;
        simsignal_t waitingTime;

        simsignal_t waitingTimeForYellowCodes;
        simsignal_t waitingTimeForRedCodes;

        simsignal_t updateSchedulingS;

        std::vector<double> waitingTimeVector;


        std::vector<double> waitingTimeForYellowCodesVector;
        std::vector<double> waitingTimeForRedCodesVector;

        simsignal_t actualTripTime;

        simsignal_t actualTripTimeForYellowCodes;
        simsignal_t actualTripTimeForRedCodes;


        simsignal_t normalizedTripTimeForRedCodes;
        simsignal_t normalizedTripTimeForYellowCodes;


        std::vector<double> actualTripTimeVector;

        std::vector<double> actualTripTimeForYellowCodesVector;
        std::vector<double> actualTripTimeForRedCodesVector;

        std::vector<double> normalizedTripTimeForYellowCodesVector;
        std::vector<double> normalizedTripTimeForRedCodesVector;

      //  simsignal_t totInTimeAssignedYellowCodes;
       // simsignal_t totInTimeRedCodes;
        simsignal_t outOfTimeForGreenCodes;


        std::vector<double> inTimeAssignedYellowCodesVector;
        std::vector<double> inTimeAssignedRedCodesVector;
        std::vector<double> outOfTimeForGreenCodesVector;

        simsignal_t stretch;
        std::vector<double> stretchVector;
        simsignal_t tripDistance;
        std::vector<double> tripDistanceVector;
        simsignal_t passengersOnBoard;
        simsignal_t toDropoffRequests;
        simsignal_t toPickupRequests;
        simsignal_t requestsAssignedPerVehicle;

        simsignal_t totalRequestsPerTime;

        simsignal_t totalRedCodesPerTime;
        simsignal_t totalYellowCodesPerTime;

        simsignal_t assignedRequestsPerTime;
        simsignal_t pickedupRequestsPerTime;
        simsignal_t droppedoffRequestsPerTime;

        simsignal_t pickedupRedCodesPerTime;
        simsignal_t droppedoffRedCodesPerTime;

        simsignal_t pickedupYellowCodesPerTime;
        simsignal_t droppedoffYellowCodesPerTime;

        simsignal_t freeVehiclesPerTime;

        std::map<Vehicle*, int> vehicles; //Vehicle -> node address
        std::map<int, StopPoint*> servedPickup;   //Details related to served pickup: needed to extract per-trip metrics
        std::map<int, double> rAssignedPerVehicle; //Number of requests assigned per vehicle
        std::map<int, std::map<int, VehicleState*>> statePerVehicle;
        std::map<int, StopPoint*>  lastSPPerVehicle;


        typedef std::map<int,std::list<StopPoint*>> RequestsPerVehicle; //vehicleID/list of requests
        RequestsPerVehicle rPerVehicle;

        typedef std::map<int,TripRequest*> UnservedRequests; //requestID/request
        UnservedRequests uRequests;

        typedef std::map<int,TripRequest*> PendingRequests; //requestID/request
        PendingRequests pendingRequests;

        typedef std::map<int, TripRequest*> InTimeYellowCodesRequests;
        InTimeYellowCodesRequests inTimeYellowCodesRequests;

        typedef std::map<int, TripRequest*> InTimeRedCodesRequests;
        InTimeRedCodesRequests inTimeRedCodesRequests;


        void initialize();
        void finish();

        virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj) = 0;
      //  virtual void handleTripRequest(TripRequest *tr) = 0;
      bool eval_feasibility(int vehicleID, StopPoint *sp); //Evaluate if the new stop-point is feasible by a vehicle

        virtual int handleTripRequest(TripRequest *tr, bool isReAllocation) = 0;
        virtual StopPointOrderingProposal* eval_requestAssignment(int vehicleID, TripRequest* newTR, bool isR) = 0; //Sort the stop-points related to the specified vehicle including the new request's pickup and dropoff point, if feasible.
        int minWaitingTimeAssignment (std::map<int,StopPointOrderingProposal*> vehicleProposal, TripRequest* newTR); //Assign the new trip request to the vehicle which minimize the pickup waiting time
        int minCostAssignment(std::map<int,StopPointOrderingProposal*> vehicleProposal, TripRequest* newTR); //Assign the new trip request to the vehicle which minimize the cost
        int minCostAssignment(std::map<int, StopPointOrderingProposal*> vehicleProposal, TripRequest *tr, bool isRealloc);
        int minWaitingTimeAssignmentWithReloc (std::map<int,StopPointOrderingProposal*> vehicleProposal, TripRequest* newTR, bool isReloc); //Assign the new trip request to the vehicle which minimize the pickup waiting time

        void updateOverTimeRequestsStatistic(std::list<StopPoint*> spList,int pReq);

       StopPoint* getRequestPickup(std::list<StopPoint*> spList, int requestID);
        StopPoint* getRequestDropOff(std::list<StopPoint*> spList, int requestID);
        void cleanStopPointList(std::list<StopPoint*> spList);

        virtual void updateStateElapsedTime(int vehicleID, int stateID);
        virtual int getMaxVehiclesSeats();
        virtual void collectPercentileStats(std::string sigName, std::vector<double> values);

      // My functions-----
     //   virtual TripRequest *updateRequestPriority(std::list<StopPoint *> spList);
     //   virtual TripRequest *getOldestPendingRequest(int priority, double oldestTime);
     /*   virtual int getMinPriority(std::list<StopPoint *> spList);
        virtual double getOldestTime(std::list<StopPoint *> spList, int priority);
        virtual int deleteSPFromVehicleList(int vehicleID, int requestID);
        virtual int getMaxRequestPriority();*/




        int getMaxTypeRequestPriority();
        virtual int getMaxTInSPList( std::list<StopPoint*> spList);
        virtual int getMinTInSPList( std::list<StopPoint*> spList);
        virtual std::list<StopPoint *>::iterator getPosOfLastPBlock(std::list<StopPoint *>::iterator begin, std::list<StopPoint *>::iterator end, int priority);
        virtual std::map<int,int> readAllRequestVeichleTypesMatching();
        virtual bool checkRequestVeichleTypesMatching(int requestTypeId, int veichleTypeId);
        virtual std::map<int, std::string> readAllRequestTypes();

        virtual void printOverDelayTimeSum(int requestTypeID, int vehicleID);



        //--------

    public:
        void printSPListInfo(int vehicleID);
        StopPoint* getNextStopPoint(int vehicleID);
        StopPoint* getCurrentStopPoint(int vehicleID);
        void registerVehicle (Vehicle *v, int address);
        int getLastVehicleLocation(int vehicleID);
        Vehicle* getVehicleByID(int vehicleID);
        //--modified--
        bool isRequestValid(const TripRequest tr);

        void updateAllScheduling();
        virtual void updateScheduling(int vehicleID, int vehicleLocation)=0;
        void updateScheduling(int vehicleID);

        int countOnBoardRequests(int vehicleID);
        StopPoint* getNewAssignedStopPoint(int vehicleID);
        inline double getMinTripLength(){return minTripLength;}


};

#endif /* BASECOORD_H_ */
