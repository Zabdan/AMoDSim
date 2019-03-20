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

#include <HeuristicCoord.h>
#include <algorithm>

Define_Module(HeuristicCoord);

void HeuristicCoord::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj)
{
    if(signalID == tripRequest)
    {
      TripRequest *tr = check_and_cast<TripRequest *>(obj);
      EV << "New TRIP request from: " << source->getFullPath() << endl;

      if(isRequestValid(*tr))
      {
          pendingRequests.insert(std::make_pair(tr->getID(), new TripRequest(*tr)));
          totrequests++;
          emit(totalRequestsPerTime, totrequests);

          handleTripRequest(tr);
      }
      else
      {
          EV << "The request " << tr->getID() << " is not valid!" << endl;
      }
    }
}

void HeuristicCoord::handleTripRequest(TripRequest *tr)
{
    std::map<int,StopPointOrderingProposal*> vehicleProposals;
    //bool stop = false;

    /*if ( simTime().dbl() > 28700)
                  return;*/



    for (auto const &x : vehicles)
    {
        TripRequest *oldTR = updateRequestPriority(rPerVehicle[x.first->getID()]);


        if(checkRequestVeichleTypesMatching(tr->getTypeID(), x.first->getTypeId()) ) {

            if(oldTR != NULL) {
                      int num = deleteSPFromVehicleList(x.first->getID(), oldTR->getID());
                      if(num == 2) {
                       StopPointOrderingProposal *tmp = eval_requestAssignment(x.first->getID(), oldTR);
                       rPerVehicle[x.first->getID()] = tmp->getSpList();
                       }
                     //  stop = true;

                       // return;

                   }

          //  if(simTime().dbl() > 24000)
            //        return;




        //Check if the vehicle has enough seats to serve the request
        if(x.first->getSeats() >= tr->getPickupSP()->getNumberOfPassengers())
        {
            StopPointOrderingProposal *tmp = eval_requestAssignment(x.first->getID(), tr);
            if(tmp && !tmp->getSpList().empty())
                vehicleProposals[x.first->getID()] = tmp;
        }
        }
    }

    //Assign the request to the vehicle which minimize the waiting time
    if(requestAssignmentStrategy == 0)
        minCostAssignment(vehicleProposals, tr);
    else
        minWaitingTimeAssignment(vehicleProposals, tr);
}





/**
 * Sort the stop-points related to the specified vehicle
 * including the new request's pickup and dropoff point, if feasible.
 * Returns the "optimal" stop point sorting.
 *
 * @param vehicleID The vehicleID
 * @param tr The new trip request
 *
 * @return The list of stop-point related to the vehicle.
 */
StopPointOrderingProposal* HeuristicCoord::eval_requestAssignment(int vehicleID, TripRequest* tr)
{
    std::list<StopPoint*> old = rPerVehicle[vehicleID];
    std::list<StopPoint*> newList;
    StopPoint* newTRpickup = new StopPoint(*tr->getPickupSP());
    StopPoint* newTRdropoff = new StopPoint(*tr->getDropoffSP());
    StopPointOrderingProposal* toReturn = NULL;
    newTRdropoff->setNumberOfPassengers(-newTRpickup->getNumberOfPassengers());
    double additionalCost = -1;


    int newTypeID = tr->getTypeID();
    int oldFirstTypeID = -1;

    if(!old.empty())
     oldFirstTypeID = pendingRequests[old.front()->getRequestID()]->getTypeID();


    //The vehicle is empty
    if(rPerVehicle.find(vehicleID) == rPerVehicle.end() || old.empty())
    {

/*
        if(simTime().dbl() > 7300) {
                   EV<<"INSPECT vehicle 4 SP LIST"<<endl;
                   std::list<StopPoint *> spList = rPerVehicle[4];
                   EV<<"NUM SP in vehicle 4  list "<<spList.size()<<endl;
                          for(std::list<StopPoint*>::const_iterator it = spList.begin(); it != spList.end(); it++) {
                              int idTR = pendingRequests[(*it)->getRequestID()]->getTypeID();
                              EV<<idTR<<" ";
                          }
                          EV<<endl;

               }

*/



        EV << "The vehicle " << vehicleID << " has not other stop points!" << endl;
        additionalCost = netmanager->getTimeDistance(getLastVehicleLocation(vehicleID), newTRpickup->getLocation());
        double timeToPickup = additionalCost + simTime().dbl();
        additionalCost+=netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation()) + (boardingTime*newTRpickup->getNumberOfPassengers());

      //  if(timeToPickup <= (newTRpickup->getTime() + newTRpickup->getMaxDelay()))
      //  {
            newTRpickup->setActualTime(timeToPickup);
            newTRpickup->setActualNumberOfPassengers(newTRpickup->getNumberOfPassengers());
            newTRdropoff->setActualTime(newTRpickup->getActualTime() + netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation()) + (boardingTime*newTRpickup->getActualNumberOfPassengers()));
            newTRdropoff->setActualNumberOfPassengers(0);
            newList.push_back(newTRpickup);
            newList.push_back(newTRdropoff);

            toReturn=new StopPointOrderingProposal(vehicleID,vehicleID, additionalCost, timeToPickup, newList);
            EV << "New Pickup can be reached at " << newTRpickup->getActualTime() << " by the vehicle " << vehicleID << ". Max allowed time is: " << (newTRpickup->getTime() + newTRpickup->getMaxDelay()) << endl;
            EV << "New Dropoff can be reached at " << newTRdropoff->getActualTime() << " by the vehicle " << vehicleID << ". Max allowed time is: " << (newTRdropoff->getTime() + newTRdropoff->getMaxDelay()) << endl;
      //  }
     /*   else
        {
            EV << " The vehicle " << vehicleID << " can not serve the Request " << tr->getID() << endl;
            delete newTRpickup;
            delete newTRdropoff;
        }*/
    }

    //The vehicle has 1 stop point
    else if(old.size() == 1)
    {
        EV << " The vehicle " << vehicleID << " has only 1 SP. Trying to push new request back..." << endl;

        StopPoint *last = new StopPoint(*old.back());
        additionalCost = netmanager->getTimeDistance(last->getLocation(), newTRpickup->getLocation());
        double timeToPickup = additionalCost + last->getActualTime() + (alightingTime*abs(last->getNumberOfPassengers())); //The last SP is a dropOff point.
        additionalCost+= netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation()) + (boardingTime*newTRpickup->getNumberOfPassengers());

     //   if(timeToPickup <= (newTRpickup->getTime() + newTRpickup->getMaxDelay()))
     //   {
            newTRpickup->setActualTime(timeToPickup);
            newTRdropoff->setActualTime(newTRpickup->getActualTime() + netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation()) + (boardingTime*newTRpickup->getNumberOfPassengers()));
            newTRpickup->setActualNumberOfPassengers(newTRpickup->getNumberOfPassengers());
            newTRdropoff->setActualNumberOfPassengers(0);
            newList.push_back(last);
            newList.push_back(newTRpickup);
            newList.push_back(newTRdropoff);

            toReturn=new StopPointOrderingProposal(vehicleID,vehicleID, additionalCost, timeToPickup, newList);
            EV << "New Pickup can be reached at " << newTRpickup->getActualTime() << " by the vehicle " << vehicleID << ". Max allowed time is: " << (newTRpickup->getTime() + newTRpickup->getMaxDelay()) << endl;
            EV << "New Dropoff can be reached at " << newTRdropoff->getActualTime() << " by the vehicle " << vehicleID << ". Max allowed time is: " << (newTRdropoff->getTime() + newTRdropoff->getMaxDelay()) << endl;
      //  }
      /*  else
        {
            EV << " The vehicle " << vehicleID << " can not serve the Request " << tr->getID() << endl;
            delete newTRpickup;
            delete newTRdropoff;
            delete last;
        }*/
    }




    else if( old.size() > 1  && newTypeID > oldFirstTypeID ) {

        EV<<"The vehicle "<<vehicleID<<"has new stop point with more high priority "<<endl;
        toReturn = addStopPointToFirstPos(vehicleID, old, tr);
        EV<<"The vehicle "<<vehicleID<<"has new proposal! "<<endl;
/*
        if(simTime().dbl() > 7300) {
               EV<<"INSPECT vehicle 4 SP LIST"<<endl;
               std::list<StopPoint *> spList = rPerVehicle[4];
               EV<<"NUM SP in vehicle 4  list "<<spList.size()<<endl;
                      for(std::list<StopPoint*>::const_iterator it = spList.begin(); it != spList.end(); it++) {
                          int idTR = pendingRequests[(*it)->getRequestID()]->getTypeID();
                          EV<<idTR<<" ";
                      }

           }*/


    }


    //The vehicle has more stop points
    else
    {
        EV << " The vehicle " << vehicleID << " has more stop points..." << endl;

        //Get all the possible sorting within the Pickup
        std::list<StopPointOrderingProposal*> proposalsWithPickup = addStopPointToTrip(vehicleID, old, newTRpickup);
        delete newTRpickup;

        std::list<StopPointOrderingProposal*> proposalsWithDropoff;

        if(!proposalsWithPickup.empty())
        {
            int cost = -1;
            EV << "STARTING DROPOFF EVALUATION..." << endl;
            for(std::list<StopPointOrderingProposal*>::const_iterator it2 = proposalsWithPickup.begin(), end2 = proposalsWithPickup.end(); it2 != end2; ++it2)
            {
                //Get all the possible sorting within the Dropoff
                proposalsWithDropoff = addStopPointToTrip(vehicleID, (*it2)->getSpList(), newTRdropoff);

                for(std::list<StopPointOrderingProposal*>::const_iterator it3 = proposalsWithDropoff.begin(), end3 = proposalsWithDropoff.end(); it3 != end3; ++it3)
                {
                    //Choose the proposal within min additional cost for the vehicle
                    (*it3)->setAdditionalCost((*it3)->getAdditionalCost() + (*it2)->getAdditionalCost());
                    if(cost == -1 || (*it3)->getAdditionalCost() < cost)
                    {
                        cost = (*it3)->getAdditionalCost();
                        delete toReturn;

                        toReturn = (*it3);
                        toReturn->setAdditionalCost(cost);
                        toReturn->setActualPickupTime((*it2)->getActualPickupTime());
                    }
                    else
                        delete *it3;
                }
                delete *it2;
            }
            delete newTRdropoff;
        }

        else
        {
            EV << " The vehicle " << vehicleID << " can not serve the Request " << tr->getID() << endl;
            delete newTRdropoff;
            delete toReturn;
        }
    }
    return toReturn;
}

















/*


StopPointOrderingProposal* HeuristicCoord::eval_requestAssignment(int vehicleID, TripRequest* tr)
{
    std::list<StopPoint*> old = rPerVehicle[vehicleID];
    std::list<StopPoint*> newList;
    StopPoint* newTRpickup = new StopPoint(*tr->getPickupSP());
    StopPoint* newTRdropoff = new StopPoint(*tr->getDropoffSP());
    StopPointOrderingProposal* toReturn = NULL;
    newTRdropoff->setNumberOfPassengers(-newTRpickup->getNumberOfPassengers());
    double additionalCost = -1;

    //The vehicle is empty
    if(rPerVehicle.find(vehicleID) == rPerVehicle.end() || old.empty() )
    {
        EV << "The vehicle " << vehicleID << " has not other stop points!" << endl;
        additionalCost = netmanager->getTimeDistance(getLastVehicleLocation(vehicleID), newTRpickup->getLocation());
        double timeToPickup = additionalCost + simTime().dbl();
        additionalCost+=netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation()) + (boardingTime*newTRpickup->getNumberOfPassengers());

        if(timeToPickup <= (newTRpickup->getTime() + newTRpickup->getMaxDelay()))
        {
            newTRpickup->setActualTime(timeToPickup);
            newTRpickup->setActualNumberOfPassengers(newTRpickup->getNumberOfPassengers());
            newTRdropoff->setActualTime(newTRpickup->getActualTime() + netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation()) + (boardingTime*newTRpickup->getActualNumberOfPassengers()));
            newTRdropoff->setActualNumberOfPassengers(0);
            newList.push_back(newTRpickup);
            newList.push_back(newTRdropoff);

            toReturn=new StopPointOrderingProposal(vehicleID,vehicleID, additionalCost, timeToPickup, newList);
            EV << "New Pickup can be reached at " << newTRpickup->getActualTime() << " by the vehicle " << vehicleID << ". Max allowed time is: " << (newTRpickup->getTime() + newTRpickup->getMaxDelay()) << endl;
            EV << "New Dropoff can be reached at " << newTRdropoff->getActualTime() << " by the vehicle " << vehicleID << ". Max allowed time is: " << (newTRdropoff->getTime() + newTRdropoff->getMaxDelay()) << endl;
        }
        else
        {
            EV << " The vehicle " << vehicleID << " can not serve the Request " << tr->getID() << endl;
            delete newTRpickup;
            delete newTRdropoff;
        }
    }

    //The vehicle has 1 stop point
    else if(old.size() == 1)
    {
        EV << " The vehicle " << vehicleID << " has only 1 SP. Trying to push new request back..." << endl;
        reSortingByPriority(old, newList, tr);
        StopPoint *last = new StopPoint(*old.back());
        TripRequest *preq = pendingRequests[last->getRequestID()];
        int reqP = getMaxRequestPriority();

        if(preq->getTypeID() == reqP || ) {
        additionalCost = netmanager->getTimeDistance(last->getLocation(), newTRpickup->getLocation());
        double timeToPickup = additionalCost + last->getActualTime() + (alightingTime*abs(last->getNumberOfPassengers())); //The last SP is a dropOff point.
        additionalCost+= netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation()) + (boardingTime*newTRpickup->getNumberOfPassengers());

        if(timeToPickup <= (newTRpickup->getTime() + newTRpickup->getMaxDelay()))
        {
            newTRpickup->setActualTime(timeToPickup);
            newTRdropoff->setActualTime(newTRpickup->getActualTime() + netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation()) + (boardingTime*newTRpickup->getNumberOfPassengers()));
            newTRpickup->setActualNumberOfPassengers(newTRpickup->getNumberOfPassengers());
            newTRdropoff->setActualNumberOfPassengers(0);
            newList.push_back(last);
            newList.push_back(newTRpickup);
            newList.push_back(newTRdropoff);

            toReturn=new StopPointOrderingProposal(vehicleID,vehicleID, additionalCost, timeToPickup, newList);
            EV << "New Pickup can be reached at " << newTRpickup->getActualTime() << " by the vehicle " << vehicleID << ". Max allowed time is: " << (newTRpickup->getTime() + newTRpickup->getMaxDelay()) << endl;
            EV << "New Dropoff can be reached at " << newTRdropoff->getActualTime() << " by the vehicle " << vehicleID << ". Max allowed time is: " << (newTRdropoff->getTime() + newTRdropoff->getMaxDelay()) << endl;
        }
        else
        {
            EV << " The vehicle " << vehicleID << " can not serve the Request " << tr->getID() << endl;
            delete newTRpickup;
            delete newTRdropoff;
            delete last;
        }
        }
        else if(preq->getTypeID() != reqP && tr->getTypeID() == reqP) {
            additionalCost = netmanager->getTimeDistance(getLastVehicleLocation(vehicleID), newTRpickup->getLocation());
            double timeToPickup = additionalCost + simTime().dbl();
            additionalCost+=netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation()) + (boardingTime*newTRpickup->getNumberOfPassengers());



        }

    }

    //The vehicle has more stop points
    else
    {
        EV << " The vehicle " << vehicleID << " has more stop points..." << endl;

       int maxP = getMaxPriority(old);
       if(tr->getTypeID() == maxP) {
       std::list<StopPoint*> selectedSp =  selectAllSpByPriority(maxP, old);


       // std::map<int, std::string>  reqTypes = readAllRequestTypes();
     //   for(std::map<int, std::string>::iterator it = reqTypes.begin(); it != reqTypes.end(); it++) {



       // }


        //Get all the possible sorting within the Pickup
        std::list<StopPointOrderingProposal*> proposalsWithPickup = addStopPointToTrip(vehicleID, selectedSp, newTRpickup);
        delete newTRpickup;

        std::list<StopPointOrderingProposal*> proposalsWithDropoff;

        if(!proposalsWithPickup.empty())
        {
            int cost = -1;
            EV << "STARTING DROPOFF EVALUATION..." << endl;
            for(std::list<StopPointOrderingProposal*>::const_iterator it2 = proposalsWithPickup.begin(), end2 = proposalsWithPickup.end(); it2 != end2; ++it2)
            {
                //Get all the possible sorting within the Dropoff
                proposalsWithDropoff = addStopPointToTrip(vehicleID, (*it2)->getSpList(), newTRdropoff);

                for(std::list<StopPointOrderingProposal*>::const_iterator it3 = proposalsWithDropoff.begin(), end3 = proposalsWithDropoff.end(); it3 != end3; ++it3)
                {
                    //Choose the proposal within min additional cost for the vehicle
                    (*it3)->setAdditionalCost((*it3)->getAdditionalCost() + (*it2)->getAdditionalCost());
                    if(cost == -1 || (*it3)->getAdditionalCost() < cost)
                    {
                        cost = (*it3)->getAdditionalCost();
                        delete toReturn;

                        toReturn = (*it3);
                        toReturn->setAdditionalCost(cost);
                        toReturn->setActualPickupTime((*it2)->getActualPickupTime());
                    }
                    else
                        delete *it3;
                }
                delete *it2;
            }
            delete newTRdropoff;
        }

        else
        {
            EV << " The vehicle " << vehicleID << " can not serve the Request " << tr->getID() << endl;
            delete newTRdropoff;
            delete toReturn;
        }
       }


       else if(tr->getTypeID() > maxP) {






       }



    }
    return toReturn;
}



*/







/*



std::list<StopPointOrderingProposal*> HeuristicCoord::addStopPointToTrip(int vehicleID, std::list<StopPoint*> spl, StopPoint* newSP)
{
    int passengers = 0;
    int delay_BA = 0; // boarding/alighting delay.
    int vehicleSeats = getVehicleByID(vehicleID)->getSeats();
    double cost = -1.0;
    double actualTime = 0.0;
    double minResidual = 0.0;
    bool constrainedPosition = false;
    double distanceTo, distanceFrom, dt, df = 0.0;
    std::list<StopPointOrderingProposal*> mylist;

    std::list<double> residualTimes;
    std::list<StopPoint*>::const_iterator it = spl.begin();
    std::list<StopPoint*>::const_iterator it2, it3 = spl.end();

    if(newSP->getIsPickup())
    {
        delay_BA = (boardingTime*newSP->getNumberOfPassengers());
        residualTimes = getResidualTime(spl,-1);
    }
    else
    {
        delay_BA = (alightingTime*abs(newSP->getNumberOfPassengers()));
        residualTimes = getResidualTime(spl,newSP->getRequestID());
        //Move until the pickup stop point
        for (it; it != spl.end(); ++it) {
            if((*it)->getRequestID() == newSP->getRequestID())
                break;
        }
    }

    for (it; it!=spl.end(); ++it) {
        it2 = it;

        if((*it)->getActualNumberOfPassengers() + newSP->getNumberOfPassengers() <= vehicleSeats)
        {
            passengers = (*it)->getActualNumberOfPassengers();

            //Get the min residual-time from "it" to last SP
            if(residualTimes.size() > 1)
            {
                residualTimes.pop_front();
                minResidual = *(std::min_element(std::begin(residualTimes), std::end(residualTimes)));
                EV << " Min residual from " << (*it)->getLocation() << " is " <<  minResidual << endl;
            }
            //Arrived in last position
            else
            {
                residualTimes.clear();
                minResidual = newSP->getTime()+newSP->getMaxDelay() - (*it)->getActualTime();
                EV << " Min residual in last position is:  " << minResidual << endl;
                if(minResidual < 0)
                    break;
            }

            //Distance from prev SP to new SP
            if((*it)->getIsPickup())
                dt = netmanager->getTimeDistance((*it)->getLocation(), newSP->getLocation()) + (boardingTime*(*it)->getNumberOfPassengers());
            else
                dt = netmanager->getTimeDistance((*it)->getLocation(), newSP->getLocation()) + alightingTime*abs((*it)->getNumberOfPassengers());

            EV << " Distance from " << (*it)->getLocation() << " to " <<  newSP->getLocation() << " is " << dt << endl;
            if((*it)->getActualTime() + dt > (newSP->getTime()+newSP->getMaxDelay()))
            {
                EV << "Stop search: from here will be unreachable within its deadline!" << endl;
                break;
            }

            //Distance from new SP to next SP
            if(it2 != (std::prev(spl.end())))
            {
                it2++;
                if((*it)->getLocation() == (*it2)->getLocation() && (!(*it2)->getIsPickup())) //Two stop point with the same location.
                {
                    EV << "Two Stop point with location " << (*it)->getLocation() << endl;
                    continue;
                }

                if(!newSP->getIsPickup() && (*it2)->getActualNumberOfPassengers() > vehicleSeats)
                {
                    EV << "The new DropOFF MUST be put before location " << (*it2)->getLocation() << " because it has " << (*it2)->getActualNumberOfPassengers() << " passengers!" << endl;
                    constrainedPosition = true;
                }

                df = netmanager->getTimeDistance(newSP->getLocation(), (*it2)->getLocation()) + delay_BA;
                EV << " Distance from " << newSP->getLocation()  << " to " << (*it2)->getLocation() << " is " << df << endl;
                actualTime = (*it2)->getActualTime() - (*it)->getActualTime();
            }
            else
                df = actualTime = 0.0;

            double c = abs(df + dt - actualTime);
            EV << " The cost is " << c << ". The minResidual is: " << minResidual << endl;

            //The additional cost does not violate any deadline
            if(c < minResidual)
            {
                if((*it)->getIsPickup() || (cost == -1 || c<cost))
                {
                    distanceTo = dt;
                    distanceFrom = df;
                    cost = c;
                    it3 = it;
                    std::list<StopPoint*> orderedList;
                    StopPointOrderingProposal* proposal = new StopPointOrderingProposal();
                    StopPoint* newSPcopy = new StopPoint(*newSP);

                    //Before new Stop point
                    for (std::list<StopPoint*>::const_iterator it = spl.begin(), end = std::next(it3); it != end; ++it) {
                        EV << " Before new SP pushing SP " << (*it)->getLocation() << ". Actual Time: " << (*it)->getActualTime() << ". PASSENGERS: " << (*it)->getActualNumberOfPassengers() << endl;
                        orderedList.push_back(new StopPoint(**it));
                    }

                    StopPoint *newListBack = orderedList.back();
                    EV << " Adding new SP after " << newListBack->getLocation() << endl;
                    newSPcopy->setActualTime(newListBack->getActualTime()+distanceTo);
                    newSPcopy->setActualNumberOfPassengers(passengers + newSP->getNumberOfPassengers());
                    EV << " New SP Max time: " << newSPcopy->getTime() + newSPcopy->getMaxDelay() << ". Actual Time: " << newSPcopy->getActualTime() << " PASSENGERS: " << newSPcopy->getActualNumberOfPassengers() << endl;
                    orderedList.push_back(newSPcopy);

                    //After new SP
                    for (std::list<StopPoint*>::const_iterator it = std::next(it3), end = spl.end(); it != end; ++it) {
                        StopPoint* tmp = new StopPoint(**it);
                        double prevActualTime = tmp->getActualTime();
                        tmp->setActualTime(tmp->getActualTime() + cost);
                        tmp->setActualNumberOfPassengers(tmp->getActualNumberOfPassengers()+newSP->getNumberOfPassengers());
                        EV << " After new SP pushing " << tmp->getLocation() << ". Previous actualTime: " << prevActualTime << ". Current actualTime: " <<tmp->getActualTime() << " max time: " << tmp->getTime() + tmp->getMaxDelay() << ". PASSENGERS: " << tmp->getActualNumberOfPassengers()<< endl;
                        orderedList.push_back(tmp);
                    }

                    proposal->setVehicleID(vehicleID);
                    proposal->setAdditionalCost(cost);
                    proposal->setActualPickupTime(newSPcopy->getActualTime());
                    proposal->setSpList(orderedList);


                    if(!(*it)->getIsPickup() && !mylist.empty())
                    {
                        StopPointOrderingProposal* toDelete = mylist.front();
                        mylist.pop_front();
                        delete toDelete;
                    }


                    mylist.push_back(proposal);
                }
            }
            else
                EV << "Additional Time not allowed!" << endl;

        }
        else
            EV << "Too many passengers after SP in location " << (*it)->getLocation() << " and ReqiestID " << (*it)->getRequestID() << endl;

        if(constrainedPosition)
            break;
    }
    return mylist;
}

*/


StopPointOrderingProposal* HeuristicCoord::addStopPointToFirstPos(int vehicleID, std::list<StopPoint*> spl, TripRequest* tr) {

    StopPointOrderingProposal *  spo = new StopPointOrderingProposal();
    StopPoint* newTRpickup = new StopPoint(*tr->getPickupSP());
    StopPoint* newTRdropoff = new StopPoint(*tr->getDropoffSP());
    std::list<StopPoint *> reOrderedList;
    StopPoint *last;
    last = newTRdropoff;
    double additionalCost = 0;
    double timeIncr = 0;



   additionalCost = netmanager->getTimeDistance(getLastVehicleLocation(vehicleID), newTRpickup->getLocation());
   double timeToPickup = additionalCost + simTime().dbl();
   additionalCost+=netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation()) + (boardingTime*newTRpickup->getNumberOfPassengers());
   newTRpickup->setActualTime(timeToPickup);
   newTRpickup->setActualNumberOfPassengers(newTRpickup->getNumberOfPassengers());
   newTRdropoff->setActualTime(newTRpickup->getActualTime() + netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation()) + (boardingTime*newTRpickup->getActualNumberOfPassengers()));
   newTRdropoff->setActualNumberOfPassengers(0);



    reOrderedList.push_back(newTRpickup);
    reOrderedList.push_back(newTRdropoff);



    additionalCost = newTRdropoff->getActualTime() + netmanager->getTimeDistance(newTRdropoff->getLocation(), spl.front()->getLocation()) + alightingTime*abs(newTRdropoff->getNumberOfPassengers());
    last = newTRdropoff;

    for (std::list<StopPoint*>::const_iterator it = spl.begin(), end = spl.end(); it != end; ++it) {

        reOrderedList.push_back(new StopPoint(**it));
    }


    std::list<StopPoint*>::const_iterator it = reOrderedList.begin();
    it++;
    it++;

    for (it; it != reOrderedList.end(); it++) {

        StopPoint* tmpSP = *it;
      //  if(it == reOrderedList.begin()) {
        if(!last->getIsPickup()) {
           timeIncr = netmanager->getTimeDistance(last->getLocation(), tmpSP->getLocation()) + last->getActualTime() + (alightingTime*abs(last->getNumberOfPassengers()));
           tmpSP->setActualTime(timeIncr);

        }
        else {
            timeIncr = netmanager->getTimeDistance(last->getLocation(), tmpSP->getLocation()) + last->getActualTime() + (boardingTime*abs(last->getNumberOfPassengers()));
            tmpSP->setActualTime(timeIncr);
        }
        if(tmpSP->getIsPickup()) {
            tmpSP->setActualNumberOfPassengers(tmpSP->getActualNumberOfPassengers());
        }
        else {
            tmpSP->setActualNumberOfPassengers(0);
        }

         last = tmpSP;
       // }

    }

    EV<<"New Proposal  with new request at first pos"<<endl;
                              for(std::list<StopPoint*>::const_iterator it = reOrderedList.begin(); it != reOrderedList.end(); it++) {
                                  int idTR = pendingRequests[(*it)->getRequestID()]->getTypeID();
                                  EV<<idTR<<" ";
                              }
    EV<<"End New Proposal"<<endl;



    spo->setVehicleID(vehicleID);
    spo->setAdditionalCost(additionalCost);
    spo->setSpList(reOrderedList);
    spo->setActualPickupTime(newTRpickup->getActualTime());
    return spo;




}




std::list<StopPointOrderingProposal*> HeuristicCoord::addStopPointToTrip(int vehicleID, std::list<StopPoint*> spl, StopPoint* newSP)
{
    int passengers = 0;
    int delay_BA = 0; // boarding/alighting delay.
    int vehicleSeats = getVehicleByID(vehicleID)->getSeats();
    double cost = -1.0;
    double actualTime = 0.0;
    double minResidual = 0.0;
    bool constrainedPosition = false;

    bool skippedProposal = false;
    double distanceTo, distanceFrom, dt, df = 0.0;

    int i = 0;
    std::list<StopPointOrderingProposal*> mylist;

    std::list<double> residualTimes;
    std::list<StopPoint*>::const_iterator it = spl.begin();
    std::list<StopPoint*>::const_iterator it2, it3 = spl.end();

    if(newSP->getIsPickup())
    {
        delay_BA = (boardingTime*newSP->getNumberOfPassengers());
        residualTimes = getResidualTime(spl,-1);
    }
    else
    {
        delay_BA = (alightingTime*abs(newSP->getNumberOfPassengers()));
        residualTimes = getResidualTime(spl,newSP->getRequestID());
        //Move until the pickup stop point
        for (it; it != spl.end(); ++it) {
            if((*it)->getRequestID() == newSP->getRequestID())
                break;
        }
    }









    for (it; it!=spl.end(); ++it) {
        it2 = it;
        i++;
        if((*it)->getActualNumberOfPassengers() + newSP->getNumberOfPassengers() <= vehicleSeats)
        {
            passengers = (*it)->getActualNumberOfPassengers();
/*
            //Get the min residual-time from "it" to last SP
            if(residualTimes.size() > 1)
            {
                residualTimes.pop_front();
                minResidual = *(std::min_element(std::begin(residualTimes), std::end(residualTimes)));
                EV << " Min residual from " << (*it)->getLocation() << " is " <<  minResidual << endl;
            }
            //Arrived in last position
            else
            {
                residualTimes.clear();
                minResidual = newSP->getTime()+newSP->getMaxDelay() - (*it)->getActualTime();
                EV << " Min residual in last position is:  " << minResidual << endl;
                if(minResidual < 0)
                    break;
            }*/

            //Distance from prev SP to new SP
            if((*it)->getIsPickup())
                dt = netmanager->getTimeDistance((*it)->getLocation(), newSP->getLocation()) + (boardingTime*(*it)->getNumberOfPassengers());
            else
                dt = netmanager->getTimeDistance((*it)->getLocation(), newSP->getLocation()) + alightingTime*abs((*it)->getNumberOfPassengers());

            EV << " Distance from " << (*it)->getLocation() << " to " <<  newSP->getLocation() << " is " << dt << endl;
 /*           if((*it)->getActualTime() + dt > (newSP->getTime()+newSP->getMaxDelay()))
            {
                EV << "Stop search: from here will be unreachable within its deadline!" << endl;
                break;
            }*/

            //Distance from new SP to next SP
            if(it2 != (std::prev(spl.end())))
            {
                it2++;
                if((*it)->getLocation() == (*it2)->getLocation() && (!(*it2)->getIsPickup())) //Two stop point with the same location.
                {
                    EV << "Two Stop point with location " << (*it)->getLocation() << endl;
                    continue;
                }

                if(!newSP->getIsPickup() && (*it2)->getActualNumberOfPassengers() > vehicleSeats)
                {
                    EV << "The new DropOFF MUST be put before location " << (*it2)->getLocation() << " because it has " << (*it2)->getActualNumberOfPassengers() << " passengers!" << endl;
                    constrainedPosition = true;
                }

                df = netmanager->getTimeDistance(newSP->getLocation(), (*it2)->getLocation()) + delay_BA;
                EV << " Distance from " << newSP->getLocation()  << " to " << (*it2)->getLocation() << " is " << df << endl;
                actualTime = (*it2)->getActualTime() - (*it)->getActualTime();
            }
            else
                df = actualTime = 0.0;

            double c = abs(df + dt - actualTime);
            EV << " The cost is " << c << ". The minResidual is: " << minResidual << endl;

            //The additional cost does not violate any deadline
          //  if(c < minResidual)
          //  {
                if((*it)->getIsPickup() || (cost == -1 || c<cost))
                {
                    distanceTo = dt;
                    distanceFrom = df;
                    cost = c;
                    it3 = it;
                    std::list<StopPoint*> orderedList;
                    StopPointOrderingProposal* proposal = new StopPointOrderingProposal();
                    StopPoint* newSPcopy = new StopPoint(*newSP);

                    /*Before new Stop point*/
                    for (std::list<StopPoint*>::const_iterator it = spl.begin(), end = std::next(it3); it != end; ++it) {
                        EV << " Before new SP pushing SP " << (*it)->getLocation() << ". Actual Time: " << (*it)->getActualTime() << ". PASSENGERS: " << (*it)->getActualNumberOfPassengers() << endl;
                           orderedList.push_back(new StopPoint(**it));
                        }



                    bool okIns = false;
                 //   std::list<StopPoint*>::const_iterator it4 = spl.begin();
                  //  StopPoint *firstEl = *it4;
                  //  it4++;
                  //  StopPoint *secondEl = *it4;
                    StopPoint *newListBack = orderedList.back();
                    StopPoint *newListSucc;
                    int typeIDnewListSucc;
                    int typeIDnewSPcopy = pendingRequests[newSPcopy->getRequestID()]->getTypeID();
                    int typeIDnewListBack = pendingRequests[newListBack->getRequestID()]->getTypeID();
                    if(orderedList.size() < spl.size())  {
                       newListSucc = *std::next(it3);
                      typeIDnewListSucc = pendingRequests[newListSucc->getRequestID()]->getTypeID();

                    }
                    if(orderedList.size() == 1 && typeIDnewListSucc > typeIDnewListBack ) {



                   // int typeIDFirstEl = pendingRequests[firstEl->getRequestID()]->getTypeID();

                    if( typeIDnewSPcopy >= typeIDnewListSucc)
                        okIns = true;

                  //  if(newListSucc->getIsPickup() && secondEl->getIsPickup() && secondEl->getRequestID() == newListSucc->getRequestID()) {
                 //      if(typeIDFirstEl < typeIDnewListSucc && typeIDnewSPcopy >=  typeIDnewListSucc)


                 //   }
                    }

                    // check for reqTypeID constraint
                    else {
                    int pDiff = typeIDnewSPcopy - typeIDnewListBack;
                    int pDiffSucc = 0;
                    if(orderedList.size() < spl.size()) {
                        newListSucc = *std::next(it3);
                  //  if(!newListSucc == NULL)
                      pDiffSucc = pendingRequests[newSPcopy->getRequestID()]->getTypeID() - pendingRequests[newListSucc->getRequestID()]->getTypeID();
                  //  }
                    }

                    if((pDiff == 0 || pDiff <= -1) && (pDiffSucc == 0 || pDiffSucc >= 1)) {
                        okIns = true;

                    }
                    }
                    if(okIns) {
                    EV << " Adding new SP after " << newListBack->getLocation() << endl;
                    newSPcopy->setActualTime(newListBack->getActualTime()+distanceTo);
                    newSPcopy->setActualNumberOfPassengers(passengers + newSP->getNumberOfPassengers());
                 //   EV << " New SP Max time: " << newSPcopy->getTime() + newSPcopy->getMaxDelay() << ". Actual Time: " << newSPcopy->getActualTime() << " PASSENGERS: " << newSPcopy->getActualNumberOfPassengers() << endl;
                    orderedList.push_back(newSPcopy);

                    //After new SP
                    for (std::list<StopPoint*>::const_iterator it = std::next(it3), end = spl.end(); it != end; ++it) {
                        StopPoint* tmp = new StopPoint(**it);
                        double prevActualTime = tmp->getActualTime();
                        tmp->setActualTime(tmp->getActualTime() + cost);
                        tmp->setActualNumberOfPassengers(tmp->getActualNumberOfPassengers()+newSP->getNumberOfPassengers());
                        EV << " After new SP pushing " << tmp->getLocation() << ". Previous actualTime: " << prevActualTime << ". Current actualTime: " <<tmp->getActualTime() << " max time: " << tmp->getTime() + tmp->getMaxDelay() << ". PASSENGERS: " << tmp->getActualNumberOfPassengers()<< endl;
                        orderedList.push_back(tmp);
                    }

                    proposal->setVehicleID(vehicleID);
                    proposal->setAdditionalCost(cost);
                    proposal->setActualPickupTime(newSPcopy->getActualTime());
                    proposal->setSpList(orderedList);

                       /*
                          EV<<"New Proposal"<<endl;
                           for(std::list<StopPoint*>::const_iterator it = orderedList.begin(); it != orderedList.end(); it++) {
                               int idTR = pendingRequests[(*it)->getRequestID()]->getTypeID();
                               EV<<idTR<<" ";
                           }
                           EV<<"End New Proposal"<<endl;*/

                    // A che serve??
                    if(!(*it)->getIsPickup() && !mylist.empty())
                    {
                        StopPointOrderingProposal* toDelete = mylist.front();
                        mylist.pop_front();
                        delete toDelete;
                    }
                    mylist.push_back(proposal);

                }

                    else {

                                      while(orderedList.size() > 0) {
                                      StopPoint *toDel = orderedList.front();
                                      orderedList.pop_front();
                                      delete toDel;
                                      }

                                  }
                }

         //   }
         //   else
         //       EV << "Additional Time not allowed!" << endl;

        }
        else
            EV << "Too many passengers after SP in location " << (*it)->getLocation() << " and ReqiestID " << (*it)->getRequestID() << endl;

        if(constrainedPosition)
            break;
    }

    return mylist;
}















//Get the additional time-cost allowed by each stop point
std::list<double> HeuristicCoord::getResidualTime(std::list<StopPoint*> spl, int requestID)
{
    std::list<double> residuals;

    //It is a pickup
    if(requestID == -1)
    {
        for (std::list<StopPoint*>::const_iterator it = spl.begin(), end = spl.end(); it != end; ++it)
            residuals.push_back((*it)->getMaxDelay() + (*it)->getTime() - (*it)->getActualTime());
    }
    else
    {
        bool found = false;
        for (std::list<StopPoint*>::const_iterator it = spl.begin(), end = spl.end(); it != end; ++it)
        {
            if(found)
                residuals.push_back((*it)->getMaxDelay() + (*it)->getTime() - (*it)->getActualTime());
            else if((*it)->getRequestID() == requestID)
            {
                residuals.push_back((*it)->getMaxDelay() + (*it)->getTime() - (*it)->getActualTime());
                found = true;
            }

        }
    }
    return residuals;
}





