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

class BaseCoord : public cSimpleModule, cListener{

private:
    virtual void updateVehicleStopPoints(int vehicleID, std::list<StopPoint*> spList, StopPoint *pickupSP);

    protected:
        double totrequests;
        double totalAssignedRequests;
        double totalPickedupRequests;
        double totalDroppedoffRequest;
        double freeVehicles;
        double minTripLength;
        int requestAssignmentStrategy;

        int boardingTime;
        int alightingTime;
        AbstractNetworkManager* netmanager;

        //Trip related signals
        simsignal_t tripRequest;
        simsignal_t newTripAssigned;

        //Statistical signals
        simsignal_t traveledDistance;
        simsignal_t waitingTime;
        std::vector<double> waitingTimeVector;
        simsignal_t actualTripTime;
        std::vector<double> actualTripTimeVector;
        simsignal_t stretch;
        std::vector<double> stretchVector;
        simsignal_t tripDistance;
        std::vector<double> tripDistanceVector;
        simsignal_t passengersOnBoard;
        simsignal_t toDropoffRequests;
        simsignal_t toPickupRequests;
        simsignal_t requestsAssignedPerVehicle;

        simsignal_t totalRequestsPerTime;
        simsignal_t assignedRequestsPerTime;
        simsignal_t pickedupRequestsPerTime;
        simsignal_t droppedoffRequestsPerTime;
        simsignal_t freeVehiclesPerTime;

        std::map<Vehicle*, int> vehicles; //Vehicle -> node address
        std::map<int, StopPoint*> servedPickup;   //Details related to served pickup: needed to extract per-trip metrics
        std::map<int, double> rAssignedPerVehicle; //Number of requests assigned per vehicle
        std::map<int, std::map<int, VehicleState*>> statePerVehicle;

        typedef std::map<int,std::list<StopPoint*>> RequestsPerVehicle; //vehicleID/list of requests
        RequestsPerVehicle rPerVehicle;

        typedef std::map<int,TripRequest*> UnservedRequests; //requestID/request
        UnservedRequests uRequests;

        typedef std::map<int,TripRequest*> PendingRequests; //requestID/request
        PendingRequests pendingRequests;


        void initialize();
        void finish();

        virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj) = 0;
        virtual void handleTripRequest(TripRequest *tr) = 0;
        bool eval_feasibility(int vehicleID, StopPoint *sp); //Evaluate if the new stop-point is feasible by a vehicle
        virtual StopPointOrderingProposal* eval_requestAssignment(int vehicleID, TripRequest* newTR) = 0; //Sort the stop-points related to the specified vehicle including the new request's pickup and dropoff point, if feasible.
        int minWaitingTimeAssignment (std::map<int,StopPointOrderingProposal*> vehicleProposal, TripRequest* newTR); //Assign the new trip request to the vehicle which minimize the pickup waiting time
        int minCostAssignment(std::map<int,StopPointOrderingProposal*> vehicleProposal, TripRequest* newTR); //Assign the new trip request to the vehicle which minimize the cost

        StopPoint* getRequestPickup(std::list<StopPoint*> spList, int requestID);
        StopPoint* getRequestDropOff(std::list<StopPoint*> spList, int requestID);
        void cleanStopPointList(std::list<StopPoint*> spList);

        virtual void updateStateElapsedTime(int vehicleID, int stateID);
        virtual int getMaxVehiclesSeats();
        virtual void collectPercentileStats(std::string sigName, std::vector<double> values);


        virtual TripRequest *updateRequestPriority(std::list<StopPoint *> spList);
        virtual TripRequest *getOldestPendingRequest(int priority, double oldestTime);
        virtual int getMinPriority(std::list<StopPoint *> spList);
        virtual double getOldestTime(std::list<StopPoint *> spList, int priority);
        virtual int deleteSPFromVehicleList(int vehicleID, int requestID);
        virtual int getMaxRequestPriority();

        virtual std::map<int,int> readAllRequestVeichleTypesMatching();
        virtual bool checkRequestVeichleTypesMatching(int requestTypeId, int veichleTypeId);
        virtual std::map<int, std::string> readAllRequestTypes();

    public:
        void printSPListInfo(int vehicleID);
        StopPoint* getNextStopPoint(int vehicleID);
        StopPoint* getCurrentStopPoint(int vehicleID);
        void registerVehicle (Vehicle *v, int address);
        int getLastVehicleLocation(int vehicleID);
        Vehicle* getVehicleByID(int vehicleID);
        bool isRequestValid(const TripRequest tr);
        int countOnBoardRequests(int vehicleID);
        StopPoint* getNewAssignedStopPoint(int vehicleID);
        inline double getMinTripLength(){return minTripLength;}
};

#endif /* BASECOORD_H_ */
