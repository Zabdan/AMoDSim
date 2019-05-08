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

#include <BaseCoord.h>
#include <sstream>

void BaseCoord::initialize()
{
    /* ---- REGISTER SIGNALS ---- */
    tripRequest = registerSignal("tripRequest");
    newTripAssigned = registerSignal("newTripAssigned");

    traveledDistance = registerSignal("traveledDistance");
    waitingTime = registerSignal("waitingTime");

    waitingTimeForYellowCodes = registerSignal("waitingTimeForYellowCodes");
    waitingTimeForRedCodes = registerSignal("waitingTimeForRedCodes");

    actualTripTime = registerSignal("actualTripTime");

    actualTripTimeForYellowCodes = registerSignal("actualTripTimeForYellowCodes");
    actualTripTimeForRedCodes = registerSignal("actualTripTimeForRedCodes");

    outOfTimeForYellowCodes = registerSignal("outOfTimeForYellowCodes");
    outOfTimeForRedCodes = registerSignal("outOfTimeForRedCodes");
    outOfTimeForGreenCodes = registerSignal("outOfTimeForGreenCodes");



    stretch = registerSignal("stretch");
    tripDistance = registerSignal("tripDistance");
    passengersOnBoard = registerSignal("passengersOnBoard");
    toDropoffRequests = registerSignal("toDropoffRequests");
    toPickupRequests = registerSignal("toPickupRequests");
    requestsAssignedPerVehicle = registerSignal("requestsAssignedPerVehicle");

    totalRequestsPerTime = registerSignal("totalRequestsPerTime");
    assignedRequestsPerTime = registerSignal("assignedRequestsPerTime");
    pickedupRequestsPerTime = registerSignal("pickedupRequestsPerTime");
    droppedoffRequestsPerTime = registerSignal("droppedoffRequestsPerTime");
    freeVehiclesPerTime = registerSignal("freeVehiclesPerTime");

    totrequests = 0.0;
    totalAssignedRequests = 0.0;
    totalPickedupRequests = 0.0;
    totalDroppedoffRequest = 0.0;

    alightingTime = getParentModule()->par("alightingTime").doubleValue();
    boardingTime = getParentModule()->par("boardingTime").doubleValue();
    requestAssignmentStrategy = par("requestAssignmentStrategy");
    netmanager = check_and_cast<AbstractNetworkManager *>(getParentModule()->getSubmodule("netmanager"));
    freeVehicles = netmanager->getNumberOfVehicles();
    emit(freeVehiclesPerTime, freeVehicles);

    //netXsize = (getParentModule()->par("width").doubleValue() - 1) * (getParentModule()->par("nodeDistance").doubleValue());
    //netYsize = (getParentModule()->par("height").doubleValue() - 1) * (getParentModule()->par("nodeDistance").doubleValue());

    simulation.getSystemModule()->subscribe("tripRequest",this);
}



int BaseCoord::minWaitingTimeAssignment (std::map<int,StopPointOrderingProposal*> vehicleProposal, TripRequest *tr)
{
    double pickupDeadline = tr->getPickupSP()->getTime() + tr->getPickupSP()->getMaxDelay();
    double dropoffDeadline = tr->getDropoffSP()->getTime() + tr->getDropoffSP()->getMaxDelay();
    double pickupActualTime = -1.0;
    double dropoffActualTime = -1.0;
    int vehicleID = -1;



    for(auto const &x : vehicleProposal)
    {
        double actualPickupTime = x.second->getActualPickupTime();
      //  if(actualPickupTime <= pickupDeadline)
     //   {
             if(pickupActualTime == -1.0 ||  actualPickupTime < pickupActualTime)
             {
                 if(vehicleID != -1) //The current proposal is better than the previous one
                     delete(vehicleProposal[vehicleID]);

                 vehicleID = x.first;
                 pickupActualTime = actualPickupTime;
                 //dropoffActualTime = actualDropoffTime;
             }
             else
                 delete x.second; //Reject the current proposal (A better one has been accepted)
      //   }
    //     else
       //      delete x.second; //Reject the current proposal: it does not respect the time constraints
    }


      if(pickupActualTime > -1)
      {

              //  EV << "Accepted request of vehicle " << vehicleID << " for request: "
             //              << tr->getID() << " .The time cost is: " << additionalCost << endl;
                 EV<<"Vehicle "<<vehicleID<<"  SP List: ";
                 std::list<StopPoint *> spList = vehicleProposal[vehicleID]->getSpList();
                 for(std::list<StopPoint*>::const_iterator it = spList.begin(); it != spList.end(); it++) {
                     int idTR = pendingRequests[(*it)->getRequestID()]->getTypeID();
                     EV<<idTR<<" ";

                 }






          EV << "Accepted request of vehicle "<< vehicleID << " for request: " << tr->getID() << " .Actual PICKUP time: " << pickupActualTime<<endl;
        //     << "/Requested Pickup Deadline: " << pickupDeadline << endl;
             //" .Actual DROPOFF time: " << dropoffActualTime << "/Requested DropOFF Deadline: " << dropoffDeadline << endl;



          updateVehicleStopPoints(vehicleID, vehicleProposal[vehicleID]->getSpList(), getRequestPickup(vehicleProposal[vehicleID]->getSpList(),tr->getID()));



          for(auto const &el:rPerVehicle) {
              printOverDelayTimeSum(tr->getTypeID(), el.first);
          }

      }
      else
      {
          EV << "No vehicle in the system can serve the request " << tr->getID() << endl;
          uRequests[tr->getID()] = new TripRequest(*tr);
          delete tr;
          return -1;
      }
      delete tr;

      return vehicleID;
}





/**
 * Assign the new trip request to the vehicle which minimize the additional time cost.
 *
 * @param vehicleProposal The vehicles proposals
 * @param tr The new TripRequest
 *
 * @return The ID of the vehicle which will serve the request or -1 otherwise.
 */
int BaseCoord::minCostAssignment(std::map<int, StopPointOrderingProposal*> vehicleProposal, TripRequest *tr) {



    double pickupDeadline = tr->getPickupSP()->getTime()+ tr->getPickupSP()->getMaxDelay();
    double additionalCost = -1.0;
    int vehicleID = -1;



    for (auto const &x : vehicleProposal) {
           double curAdditionalCost = x.second->getAdditionalCost();



           EV<<"Add cost of vehicle "<<x.first<<" is "<<curAdditionalCost<<endl;
       //    if (x.second->getActualPickupTime() <= pickupDeadline) {
               if (additionalCost == -1.0 || curAdditionalCost < additionalCost) {
              //     if (vehicleID != -1) //The current proposal is better than the previous one
               //       delete (vehicleProposal[vehicleID]);

                   vehicleID = x.first;
                   additionalCost = curAdditionalCost;
               }
              else {

               //   if(tr->getPickupSP()->getLocation() == 15)
                  delete x.second; //Reject the current proposal (A better one has been accepted)

              }
          // } else
          //     delete x.second; //Reject the current proposal: it does not respect the time constraints
    //   }


    }



   // if(simTime().dbl()> 7300 && tr->getPickupSP()->getLocation() == 15)
  //            return -1;


    if (additionalCost > -1) {
        EV << "Accepted request of vehicle " << vehicleID << " for request: "
                  << tr->getID() << " .The time cost is: " << additionalCost << endl;
        EV<<"Vehicle "<<vehicleID<<"  SP List: ";
        std::list<StopPoint *> spList = vehicleProposal[vehicleID]->getSpList();
        for(std::list<StopPoint*>::const_iterator it = spList.begin(); it != spList.end(); it++) {
            int idTR = pendingRequests[(*it)->getRequestID()]->getTypeID();
            EV<<idTR<<" ";
        }


        updateVehicleStopPoints(vehicleID, vehicleProposal[vehicleID]->getSpList(),getRequestPickup(vehicleProposal[vehicleID]->getSpList(),tr->getID()));

    } else {
        EV << "No vehicle in the system can serve the request " << tr->getID()<< endl;
        uRequests[tr->getID()] = new TripRequest(*tr);
        delete tr;
        return -1;
    }
    delete tr;

    return vehicleID;


}



/**
 * Update the list of stop points assigned to a vehicle.
 *
 * @param vehicleID The vehicle ID
 * @param spList The list of stop points.
 *
 */
void BaseCoord::updateVehicleStopPoints(int vehicleID, std::list<StopPoint*> spList, StopPoint *pickupSP)
{
      rAssignedPerVehicle[vehicleID]++;
      totalAssignedRequests++;
      emit(assignedRequestsPerTime, totalAssignedRequests);

      bool toEmit = false;
      if(rPerVehicle[vehicleID].empty())
      {
          //The node which handle the selected vehicle should be notified
          toEmit = true;
          freeVehicles--;
          emit(freeVehiclesPerTime, freeVehicles);

          updateStateElapsedTime(vehicleID, -1); //update IDLE elapsed time
      }
      else
      {
          //clean the old stop point list assigned to the vehicle
          cleanStopPointList(rPerVehicle[vehicleID]);
      }

      rPerVehicle[vehicleID] = spList;
      if(toEmit)
      {
          (statePerVehicle[vehicleID][0])->setStartingTime(simTime().dbl());
          emit(newTripAssigned, (double)vehicleID);
      }
}


/*
void BaseCoord::getVehicleProposalByRequest(std::map<int, StopPointOrderingProposal*> vehicleProposal, TripRequest *tr) {
    if(tr->getTypeID() == 0) {

    if(tr->getTypeID() == 0 && vehicles[x])
}*/


/**
 * Emit statistical signal before end the simulation
 */
void BaseCoord::finish()
{
    /*--- Total Requests Statistic ---*/
    char totalRequestSignal[32];
    sprintf(totalRequestSignal, "Total Requests");
    recordScalar(totalRequestSignal, totrequests);

    /*--- Unserved Requests Statistic ---*/
    char unservedRequestSignal[32];
    sprintf(unservedRequestSignal, "Unserved Requests");
    recordScalar(unservedRequestSignal, uRequests.size());

    /*--- Pending Requests Statistic ---*/
    char pendingRequestSignal[32];
    sprintf(pendingRequestSignal, "Pending Requests");
    recordScalar(pendingRequestSignal, pendingRequests.size());

    /*Define vectors for additional statistics (Percentiles) */
    std::vector<double> traveledDistanceVector;
    std::vector<double> requestsAssignedPerVehicleVector;
    std::vector<double> passengersOnBoardVector;
    std::vector<double> toDropoffRequestsVector;
    std::vector<double> toPickupRequestsVector;
    std::map<int, std::vector<double>> statsPerVehiclesVectors;
    double totalToPickup = 0.0;

    /* Register the Travel-Time related signals */
    int maxSeats = getMaxVehiclesSeats();
    std::map<int, simsignal_t> travelStats;
    for(int i=-1; i<=maxSeats; i++)
    {
        char sigName[32];
        sprintf(sigName, "travel%d-time", i);
        simsignal_t travsignal = registerSignal(sigName);
        travelStats[i] = travsignal;

        char statisticName[32];
        sprintf(statisticName, "travel%d-time", i);
        cProperty *statisticTemplate =
            getProperties()->get("statisticTemplate", "travelTime");

        ev.addResultRecorders(this, travsignal, statisticName, statisticTemplate);
    }


    /*--- Per Vehicle related Statistics ---*/
    for(std::map<Vehicle*, int>::iterator itr = vehicles.begin(); itr != vehicles.end(); itr++)
    {
        double tmp = (itr->first->getTraveledDistance())/1000;
        emit(traveledDistance, tmp);
            traveledDistanceVector.push_back(tmp);

        tmp=rAssignedPerVehicle[itr->first->getID()];
        emit(requestsAssignedPerVehicle, tmp);
            requestsAssignedPerVehicleVector.push_back(tmp);

        std::map<int, std::list<StopPoint*>>::const_iterator it = rPerVehicle.find(itr->first->getID());
        if(it == rPerVehicle.end() || it->second.empty())
        {
            emit(passengersOnBoard, 0.0);
                passengersOnBoardVector.push_back(0.0);
            emit(toDropoffRequests, 0.0);
                toDropoffRequestsVector.push_back(0.0);
            emit(toPickupRequests, 0.0);
                toPickupRequestsVector.push_back(0.0);
        }
        else
        {
            StopPoint* nextSP = it->second.front();
            tmp=(double)nextSP->getActualNumberOfPassengers() - nextSP->getNumberOfPassengers();
            emit(passengersOnBoard, tmp);
                passengersOnBoardVector.push_back(tmp);

            int pickedupRequests = countOnBoardRequests(itr->first->getID());
            emit(toDropoffRequests, (double)pickedupRequests);
                toDropoffRequestsVector.push_back((double)pickedupRequests);
            double tpr = (double)((it->second.size() - pickedupRequests)/2);
            emit(toPickupRequests, (double)((it->second.size() - pickedupRequests)/2));
                toPickupRequestsVector.push_back(tpr);
                totalToPickup += tpr;
        }


        for(auto const& x : statePerVehicle[itr->first->getID()])
        {
            tmp= x.second->getElapsedTime() / 60;
            emit(travelStats[x.first], tmp);
            statsPerVehiclesVectors[x.first].push_back(tmp);
        }

        vehicles.erase(itr);
    }
    /*--- Total To Pickup ---*/
    char totalRequestsToPickup[32];
    sprintf(totalRequestsToPickup, "Total Requests To Pickup");
    recordScalar(totalRequestsToPickup, totalToPickup);

    /* -- Collect Percentile Statistic -- */
    collectPercentileStats("traveledDistance", traveledDistanceVector);
    collectPercentileStats("requestsPerVehicle", requestsAssignedPerVehicleVector);
    collectPercentileStats("passengersOnBoard", passengersOnBoardVector);
    collectPercentileStats("toDropoffRequests", toDropoffRequestsVector);
    collectPercentileStats("toPickupRequests", toPickupRequestsVector);
    collectPercentileStats("waitingTime", waitingTimeVector);
    collectPercentileStats("actualTripTime", actualTripTimeVector);
    collectPercentileStats("stretch",stretchVector);
    collectPercentileStats("tripDistance", tripDistanceVector);

    for(auto const& x : statsPerVehiclesVectors)
    {
        char statisticName[32];
        sprintf(statisticName, "travel%d-time", x.first);
        if (!x.second.empty())
            collectPercentileStats(statisticName, x.second);
    }


    /*------------------------------- CLEAN ENVIRONMENT -------------------------------*/

    for(std::map<int, TripRequest*>::iterator itr = pendingRequests.begin(); itr != pendingRequests.end(); itr++)
        delete itr->second;

    for(std::map<int, TripRequest*>::iterator itr = uRequests.begin(); itr != uRequests.end(); itr++)
        delete itr->second;

    for(std::map<int, std::list<StopPoint*>>::iterator itr = rPerVehicle.begin(); itr != rPerVehicle.end(); itr++)
        cleanStopPointList(itr->second);

    for(std::map<int, StopPoint*>::iterator itr = servedPickup.begin(); itr != servedPickup.end(); itr++)
        delete itr->second;

    for(std::map<int, std::map<int, VehicleState*>>::iterator itr = statePerVehicle.begin(); itr != statePerVehicle.end(); itr++)
        for(std::map<int, VehicleState*>::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++)
            delete itr2->second;


    /*------------------------------- END CLEAN ENVIRONMENT ----------------------------*/
}

/**
 * Get the number of picked-up requests not yet dropped-off.
 *
 * @param vehicleID
 *
 * @return number of picked-up requests.
 */
int BaseCoord::countOnBoardRequests(int vehicleID)
{
    std::list<StopPoint*> requests = rPerVehicle[vehicleID];
    int onBoard = 0;
    std::set<int> pickupSP;

    for (std::list<StopPoint*>::const_iterator it=requests.begin(); it != requests.end(); ++it)
    {
        if((*it)->getIsPickup())
            pickupSP.insert((*it)->getRequestID());
        else
            if(pickupSP.find((*it)->getRequestID()) == pickupSP.end())
                onBoard++;
    }
    return onBoard;
}


void BaseCoord::printOverDelayTimeSum(int typeID, int vehicleID) {
    std::list<StopPoint*> requests = rPerVehicle[vehicleID];
   // int num;
    std::vector<double> time(0);

    for(std::list<StopPoint *>::const_iterator it=requests.begin(); it!=requests.end(); it++) {
        StopPoint *sp = (*it);
        int tmpTypeID = pendingRequests[sp->getRequestID()]->getTypeID();
        if(tmpTypeID == typeID && sp->getIsPickup()) {
           double t = sp->getActualTime()-sp->getTime()-sp->getMaxDelay();
           //num++;
           time.push_back(t);
        }

    }

    EV<<"Vehicle ID "<<vehicleID<<" has "<<time.size()<< " pending requests of type"<<typeID<<endl;
    EV<<"Vehicle ID "<<vehicleID<<" has "<<std::accumulate(time.begin(), time.end(),0.0)<< "  of over delay time sum  for pending requests of type"<<typeID<<endl;


}






void BaseCoord::printSPListInfo(int vehicleID) {
    std::list<StopPoint *> spList = rPerVehicle[vehicleID];
    EV<<" SPListInfo - INSPECT vehicle "<<vehicleID<<" SP LIST"<<endl;
    EV<<"NUM SP"<<spList.size()<<endl;
  /*  for(std::list<StopPoint*>::const_iterator it = spList.begin(); it != spList.end(); it++) {
        int idTR = pendingRequests[(*it)->getRequestID()]->getTypeID();
        EV<<idTR<<" ";
    }*/


}



/**
 * Get the pointer to the pickup SP related to the requestID.
 *
 * @param spList List of stop-point where look for the pickup.
 * @param requestID The ID of the TripRequest
 *
 * @return The StopPoint or NULL
 */
StopPoint* BaseCoord::getRequestPickup(std::list<StopPoint*> spList, int requestID)
{
    for (std::list<StopPoint*>::iterator it=spList.begin(); it != spList.end(); ++it)
    {
        if(((*it)->getRequestID() == requestID) && (*it)->getIsPickup())
            return (*it);
    }

    return nullptr;
}


/**
 * Get the pointer to the dropoff SP related to the requestID.
 *
 * @param spList List of stop-point where look for the dropoff.
 * @param requestID The ID of the TripRequest
 *
 * @return The StopPoint or NULL
 */
StopPoint* BaseCoord::getRequestDropOff(std::list<StopPoint*> spList, int requestID)
{
    for (std::list<StopPoint*>::iterator it=spList.begin(); it != spList.end(); ++it)
    {
        if(((*it)->getRequestID() == requestID) && !(*it)->getIsPickup())
            return (*it);
    }

    return nullptr;
}


/**
 * Get the next stop point for the specified vehicle.
 *
 * @param vehicleID
 * @return
 */
StopPoint* BaseCoord::getNextStopPoint(int vehicleID)
{
    if ((rPerVehicle.find(vehicleID) != rPerVehicle.end()) && !(rPerVehicle[vehicleID].empty()))
    {
        StopPoint *front = rPerVehicle[vehicleID].front();
        int currentPassengers = front->getActualNumberOfPassengers();
        rPerVehicle[vehicleID].pop_front();

/*
        if(simTime().dbl() > 7300  && vehicleID == 4) {
                     EV<<" GET NEXT STOP POINT - INSPECT vehicle 4 SP LIST"<<endl;
                     std::list<StopPoint *> spList = rPerVehicle[4];
                     EV<<"NUM SP in vehicle 4  list "<<spList.size()<<endl;
                            for(std::list<StopPoint*>::const_iterator it = spList.begin(); it != spList.end(); it++) {
                                int idTR = pendingRequests[(*it)->getRequestID()]->getTypeID();
                                EV<<idTR<<" ";
                            }

                 }
*/





        if (!rPerVehicle[vehicleID].empty())
        {
            VehicleState *currState = statePerVehicle[vehicleID][currentPassengers];
            currState->setStartingTime(simTime().dbl());

            StopPoint *r = rPerVehicle[vehicleID].front();
            delete front;
            return r;
        }
        delete front;
    }

    VehicleState *idleState = statePerVehicle[vehicleID][-1];
    idleState->setStartingTime(simTime().dbl());
    freeVehicles++;
    emit(freeVehiclesPerTime, freeVehicles);

    return NULL;
}


/**
 * Get the current stop point for the specified vehicle.
 * Call the function when the vehicle reach a stop-point location.
 *
 * @param vehicleID
 * @return The pointer to the current stop point
 */
StopPoint* BaseCoord::getCurrentStopPoint(int vehicleID)
{
    if ((rPerVehicle.find(vehicleID) != rPerVehicle.end()) && !(rPerVehicle[vehicleID].empty()))
    {


        StopPoint *r = rPerVehicle[vehicleID].front();

   /*     if(simTime().dbl() > 7900 && vehicleID == 4){
                                                                   EV<<"Pk stopped!"<<endl;
                                                                   EV<<"Request ID:  "<<r->getRequestID()<<endl;
                                                                   return  NULL;

                                                  }*/

        updateStateElapsedTime(vehicleID, r->getActualNumberOfPassengers() - r->getNumberOfPassengers());





        if(r->getIsPickup())
        {

            StopPoint *sPickup = new StopPoint(*r);

            servedPickup[r->getRequestID()] = sPickup;

            double tmp = (simTime().dbl()-r->getTime())/60;



            totalPickedupRequests++;
            emit(pickedupRequestsPerTime, totalPickedupRequests);
            emit(waitingTime, tmp);
                waitingTimeVector.push_back(tmp);

            if(pendingRequests[r->getRequestID()]->getTypeID()==1)    {
                emit(waitingTimeForYellowCodes, tmp);
                               waitingTimeForYellowCodesVector.push_back(tmp);
            }
            else if(pendingRequests[r->getRequestID()]->getTypeID()==2)    {
                emit(waitingTimeForRedCodes, tmp);
                               waitingTimeForRedCodesVector.push_back(tmp);
            }


        }
        else
        {
            TripRequest *preq = pendingRequests[r->getRequestID()];
            double att = (simTime().dbl() - servedPickup[r->getRequestID()]->getActualTime()); //ActualTripTime
            double str = (netmanager->getTimeDistance(servedPickup[r->getRequestID()]->getLocation(), r->getLocation())) / att; //Trip Efficiency Ratio
            totalDroppedoffRequest++;
            emit(droppedoffRequestsPerTime, totalDroppedoffRequest);

            double trip_distance = netmanager->getSpaceDistance(servedPickup[r->getRequestID()]->getLocation(), r->getLocation()) / 1000;
            emit(actualTripTime, (att/60));
                actualTripTimeVector.push_back((att/60));

            double ot = r->getActualTime() -r->getTime()-r->getMaxDelay();

            if(preq->getTypeID()==0)    {
            //    if(ot<0) {
                    emit(outOfTimeForGreenCodes, (ot/60));
                    outOfTimeForGreenCodesVector.push_back((ot/60));
            //    }
          /*      else {
                    emit(outOfTimeForGreenCodes, 0);
                    outOfTimeForGreenCodesVector.push_back(0);
                }*/


            }

            if(preq->getTypeID()==1)    {
                emit(actualTripTimeForYellowCodes, (att/60));
                actualTripTimeForYellowCodesVector.push_back((att/60));
                if(ot<0) {
                            emit(outOfTimeForYellowCodes, (ot/60));
                            outOfTimeForYellowCodesVector.push_back((ot/60));
                        }
                        else {
                            emit(outOfTimeForYellowCodes, 0);
                            outOfTimeForYellowCodesVector.push_back(0);
                        }
                }
            else if(preq->getTypeID()==2)    {
                emit(actualTripTimeForRedCodes, (att/60));
                actualTripTimeForRedCodesVector.push_back((att/60));

                if(ot<0) {
                    emit(outOfTimeForRedCodes, (ot/60));
                    outOfTimeForRedCodesVector.push_back((ot/60));
                }
                else {
                    emit(outOfTimeForRedCodes, 0);
                    outOfTimeForRedCodesVector.push_back(0);
                }


                }


            emit(stretch, str);
                stretchVector.push_back(str);
            emit(tripDistance, trip_distance);
                tripDistanceVector.push_back(trip_distance);



                // delete associated pendingRequest


          pendingRequests.erase(r->getRequestID());
          delete preq;

        }
        return r;
    }
    return NULL;
}

/**
 * Get the pointer to the first stop point assigned to the vehicle.
 * Call this function when receive a "newTripAssigned" signal.
 *
 * @param vehicleID
 * @return The pointer to the first stop point
 */
StopPoint* BaseCoord::getNewAssignedStopPoint(int vehicleID)
{
    return rPerVehicle[vehicleID].front();
}

/**
 * Update the elapsed time of the specified TravelState.
 *
 * @param vehicleID
 * @param stateID
 */
void BaseCoord::updateStateElapsedTime(int vehicleID, int stateID)
{
    VehicleState *prevState = statePerVehicle[vehicleID][stateID];
    prevState->setElapsedTime(prevState->getElapsedTime() + (simTime().dbl() - prevState->getStartingTime()));

    EV << "Elapsed Time for state "<< stateID << " is: " << prevState->getElapsedTime() << endl;
}

/**
 * Register the vehicle v in a node.
 *
 * @param v
 * @param address The node address
 */
void BaseCoord::registerVehicle(Vehicle *v, int address)
{
    if(statePerVehicle.find(v->getID()) == statePerVehicle.end())
    {
        double currTime = simTime().dbl();

        for(int i=-1; i<=v->getSeats(); i++)
            statePerVehicle[v->getID()].insert(std::make_pair(i, new VehicleState(i,currTime)));
    }
    vehicles[v] = address;
    EV << "Registered vehicle " << v->getID() << " in node: " << address << endl;

}


/**
 * Get the last location where the vehicle was registered.
 *
 * @param vehicleID
 * @return the location address
 */
int BaseCoord::getLastVehicleLocation(int vehicleID)
{
    for(auto const& x : vehicles)
    {
        if(x.first->getID() == vehicleID)
            return x.second;
    }
    return -1;
}


/**
 * Get vehicle from its ID.
 *
 * @param vehicleID
 * @return pointer to the vehicle
 */
Vehicle* BaseCoord::getVehicleByID(int vehicleID)
{
    for(auto const& x : vehicles)
    {
        if(x.first->getID() == vehicleID)
            return x.first;
    }
    return nullptr;
}


/**
 * Delete unused dynamically allocated memory.
 *
 * @param spList The list of stop point
 */
void BaseCoord::cleanStopPointList(std::list<StopPoint*> spList)
{
    for(auto &it:spList) delete it;
    spList.clear();
}


/**
 * Check if a Trip Request is valid.
 *
 * @param tr The trip Request.
 * @return true if the request is valid.
 */
bool BaseCoord::isRequestValid(const TripRequest tr)
{
    bool valid = false;

            if(tr.getPickupSP() && tr.getDropoffSP() &&
                       netmanager->isValidAddress(tr.getPickupSP()->getLocation()) && !netmanager->isValidDestinationAddress(tr.getTypeID(),tr.getPickupSP()->getLocation())
                       && netmanager->isValidDestinationAddress(tr.getTypeID(),tr.getDropoffSP()->getLocation())) {
                           valid = true;

            }
  //  EV<<"VA "<<netmanager->isValidAddress(tr.getPickupSP()->getLocation())<<endl;
   // EV<<"!VDA "<<!netmanager->isValidDestinationAddress(tr.getTypeID(),tr.getPickupSP()->getLocation())<<endl;
 //   EV<<"VDA"<<netmanager->isValidDestinationAddress(tr.getTypeID(),tr.getDropoffSP()->getLocation())<<endl;


    return valid;


}

/**
 * Get from all available vehicles, the max number of seats.
 *
 * @return The max seats
 */
int BaseCoord::getMaxVehiclesSeats()
{
    int vSeats = 0;

    for(auto &it:vehicles)
    {
        if(it.first->getSeats() > vSeats)
            vSeats = it.first->getSeats();
    }
    return vSeats;
}

/**
 * Collect Median and 95th percentile related to the provided vector.
 *
 * @param sigName The signal name
 * @param values
 */
void BaseCoord::collectPercentileStats(std::string sigName, std::vector<double> values)
{
    int size=values.size();
    if(size==0)
    {
        recordScalar((sigName+": Median").c_str(), 0.0);
        recordScalar((sigName+": 95Percentile").c_str(), 0.0);
        return;
    }

    if(size == 1)
    {
        recordScalar((sigName+": Median").c_str(), values[0]);
        recordScalar((sigName+": 95Percentile").c_str(), values[0]);
        return;
    }

    std::sort(values.begin(), values.end());
    double median;
    double percentile95;

    if (size % 2 == 0)
          median= (values[size / 2 - 1] + values[size / 2]) / 2;
    else
        median = values[size / 2];

    recordScalar((sigName+": Median").c_str(), median);


    /*95th percentile Evaluation*/
    double index = 0.95*size;
    double intpart;
    double decpart;

    decpart = modf(index, &intpart);
    if(decpart == 0.0)
        percentile95 = (values[intpart-1]+values[intpart]) / 2;

    else{
        if( decpart > 0.4)
            index=intpart;
        else
            index=intpart-1;
        percentile95 = values[index];
    }

    recordScalar((sigName+": 95Percentile").c_str(), percentile95);

}

/**
 * Evaluate if a stop point is "feasible" by a vehicle.
 *
 * @param vehicleID The vehicleID
 * @param sp The stop-point to evaluate
 *
 * @return true if feasible
 */
bool BaseCoord::eval_feasibility (int vehicleID, StopPoint* sp)
{
    //TODO return the beforeR and afterR

    std::list<StopPoint*> lsp = rPerVehicle[vehicleID];
    std::list<StopPoint*> afterR;
    std::list<StopPoint*> beforeR;
    double currentTime = simTime().dbl();
    bool isFeasible = true;

    for (std::list<StopPoint*>::const_iterator it = lsp.begin(), end = lsp.end(); it != end; ++it) {
        EV <<"Distance from " << sp->getLocation() << " to " << (*it)->getLocation() << " is > " <<  ((*it)->getTime() + (*it)->getMaxDelay() - currentTime) << endl;
        if (netmanager->getTimeDistance(sp->getLocation(), (*it)->getLocation()) > ((*it)->getTime() + (*it)->getMaxDelay() - currentTime))
            beforeR.push_back((*it));
    }

    for (std::list<StopPoint*>::const_iterator it = lsp.begin(), end = lsp.end(); it != end; ++it) {
        EV <<"Distance from " << (*it)->getLocation() << " to " << sp->getLocation() << " is > " <<  (sp->getTime() + sp->getMaxDelay() - currentTime) << endl;
        if (netmanager->getTimeDistance((*it)->getLocation(), sp->getLocation()) > (sp->getTime() + sp->getMaxDelay() - currentTime))
            afterR.push_back((*it));
    }

    if(beforeR.empty() || afterR.empty())
        isFeasible = true;

    else
    {
        for (std::list<StopPoint*>::const_iterator it = beforeR.begin(), end = beforeR.end(); it != end; ++it)
        {
            for(std::list<StopPoint*>::const_iterator it2 = afterR.begin(), end = afterR.end(); it2 != end; ++it2)
            {
                if((*it)->getLocation() == (*it2)->getLocation())
                {
                    EV << "The same node is in before and after list! Node is: " << (*it)->getLocation() << endl;
                    isFeasible = false;
                    break;
                }

                if (netmanager->getTimeDistance((*it)->getLocation(), (*it2)->getLocation()) > ((*it2)->getTime() + (*it2)->getMaxDelay() - currentTime))
                {
                    EV << "The request is not feasible for the vehicle " << vehicleID << endl;
                    isFeasible = false;
                    break;
                }
            }
            if(!isFeasible)
                break;
            EV << "The request could be feasible for the vehicle " << vehicleID << endl;
        }
    }

    return isFeasible;
}



bool BaseCoord::checkRequestVeichleTypesMatching(int requestTypeId, int veichleTypeId) {

    std::map<int,int> rVMatch = readAllRequestVeichleTypesMatching();

    for(auto const &rv : rVMatch) {

        if( rv.first == requestTypeId && rv.second == veichleTypeId) {
            return true;
        }
    }
    return false;
}



std::map<int, std::string>   BaseCoord::readAllRequestTypes() {

    std::map<int, std::string> requestTypes;
    std::string line;
     std::fstream requestTypesFile(par("requestTypesFile").stringValue(), std::ios::in);
    // EV<<"File opened"<<nodeTypesFile.is_open()<<endl;
        while(getline(requestTypesFile, line, '\n'))
        {
           // EV<<"Line "<<endl;
            if (line.empty() || line[0] == '#')
                continue;
          //  EV<<"Line "<< line<<endl;
            std::vector<std::string> tokens = cStringTokenizer(line.c_str()).asVector();
            if (tokens.size() != 2)
                throw cRuntimeError("wrong line in module file: 2 items required, line: \"%s\"", line.c_str());

            // get fields from tokens
            int requestTypeId = atoi(tokens[0].c_str());
            const char *requestTypeName = tokens[1].c_str();
           // EV<<"Node type id"<<nodeTypeid<<" name"<<nodeTypeName<<endl;
            requestTypes.insert(std::pair<int, std::string>(requestTypeId, requestTypeName));


 }

return requestTypes;


}






std::map<int,int> BaseCoord::readAllRequestVeichleTypesMatching() {
    std::map<int,int> rVMatch;
    std::string line;
          std::fstream vRMFile(par("requestVehicleMatchingFile").stringValue(), std::ios::in);
     //     EV<<"File opened"<<vRMFile.is_open()<<endl;
             while(getline(vRMFile, line, '\n'))
             {

                 if (line.empty() || line[0] == '#')
                     continue;

                 std::vector<std::string> tokens = cStringTokenizer(line.c_str()).asVector();
                 if (tokens.size() != 2)
                     throw cRuntimeError("wrong line in module file: 2 items required, line: \"%s\"", line.c_str());

                 int requestTypeId = atoi(tokens[0].c_str());
                 int veichleTypeId = atoi(tokens[1].c_str());

                 rVMatch.insert(std::pair<int, int>(requestTypeId, veichleTypeId));
              }
        return rVMatch;

}



