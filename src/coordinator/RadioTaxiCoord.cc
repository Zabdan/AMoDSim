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

#include <RadioTaxiCoord.h>

Define_Module(RadioTaxiCoord);

double currentTime = 0.0;

void RadioTaxiCoord::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj)
{
    currentTime = simTime().dbl();

    if(signalID == tripRequest)
    {
        TripRequest *tr = check_and_cast<TripRequest *>(obj);
        EV << "New TRIP request from: " << source->getFullPath() << endl;

        if(isRequestValid(*tr))
        {
            pendingRequests.insert(std::make_pair(tr->getID(),  new TripRequest(*tr)));
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


void RadioTaxiCoord::handleTripRequest(TripRequest *tr)
{
    std::map<int,StopPointOrderingProposal*> vehicleProposals;



    for (auto const &x : vehicles)
    {

       if(checkRequestVeichleTypesMatching(tr->getTypeID(), x.first->getTypeId()) ) {
        //Check if the vehicle has enough seats to serve the request
        if(x.first->getSeats() >= tr->getPickupSP()->getNumberOfPassengers())
        {
            StopPointOrderingProposal *tmp = eval_requestAssignment(x.first->getID(), tr);
            if(tmp)
                vehicleProposals[x.first->getID()] = tmp;
        }
   // }
    }

    }

    //Assign the request to the vehicle which minimize the waiting time
    if(requestAssignmentStrategy == 0)
        minCostAssignment(vehicleProposals, tr);
    else
        minWaitingTimeAssignment(vehicleProposals, tr);
}


/**
 * Sort the stop-points related to the specified vehicle including the new request's pickup and dropoff point, if feasible.
 * This sorting add the new Trip Request in last position.
 *
 * @param vehicleID The vehicleID
 * @param tr The new TripRequest
 *
 * @return The sorted list of stop-point related to the vehicle.
 */
StopPointOrderingProposal* RadioTaxiCoord::eval_requestAssignment(int vehicleID, TripRequest* tr)
{
    StopPoint *pickupSP = new StopPoint(*tr->getPickupSP());
    StopPoint *dropoffSP = new StopPoint(*tr->getDropoffSP());
    dropoffSP->setNumberOfPassengers(-pickupSP->getNumberOfPassengers());

    double dst_to_pickup = -1;
    double dst_to_dropoff = -1;
    double additionalCost = -1;
    std::list<StopPoint*> old = rPerVehicle[vehicleID];
    std::list<StopPoint*> newList;
    StopPointOrderingProposal *proposal = NULL;



    int newTypeID = tr->getTypeID();
       int oldFirstTypeID = -1;

       if(!old.empty())
        oldFirstTypeID = pendingRequests[old.front()->getRequestID()]->getTypeID();


    //-----The Vehicle is empty-----
    if(rPerVehicle.find(vehicleID) == rPerVehicle.end() || old.empty())
    {
        EV << "The vehicle " << vehicleID << " has not other stop points!" << endl;
        dst_to_pickup = netmanager->getTimeDistance(getLastVehicleLocation(vehicleID), pickupSP->getLocation());
        dst_to_dropoff = netmanager->getTimeDistance(pickupSP->getLocation(), dropoffSP->getLocation()) + (boardingTime*abs(pickupSP->getNumberOfPassengers()));
        additionalCost = dst_to_pickup + dst_to_dropoff;

        if (dst_to_pickup >= 0 && dst_to_dropoff >= 0)
        {
            pickupSP->setActualTime(dst_to_pickup + currentTime);
            dropoffSP->setActualTime(pickupSP->getActualTime() + dst_to_dropoff);

            EV << "Time needed to vehicle: " << vehicleID << " to reach pickup: " << pickupSP->getLocation() << " is: " << (pickupSP->getActualTime()-currentTime)/60 << " minutes." << endl;
            EV << "Time needed to vehicle: " << vehicleID << " to reach dropoff: " << dropoffSP->getLocation() << " is: " << (dropoffSP->getActualTime()-currentTime)/60 << " minutes." << endl;
        }

        for (auto const &x : old)
                   newList.push_back(new StopPoint(*x));

               pickupSP->setActualNumberOfPassengers(pickupSP->getNumberOfPassengers());
               dropoffSP->setActualNumberOfPassengers(0);
               newList.push_back(pickupSP);
               newList.push_back(dropoffSP);

               proposal = new StopPointOrderingProposal(vehicleID,vehicleID, additionalCost, pickupSP->getActualTime(), newList);





    }

    else if(rPerVehicle[vehicleID].size() == 1) {

        EV << "The vehicle " << vehicleID << " has other stop points!" << endl;
        //Get last stop point for the vehicle
        StopPoint *sp = old.back();
        additionalCost = netmanager->getTimeDistance(sp->getLocation(), pickupSP->getLocation());
        dst_to_pickup = additionalCost + (sp->getActualTime() - currentTime) + (alightingTime*abs(sp->getNumberOfPassengers())); //The last stop point is a dropOff point.
        dst_to_dropoff = netmanager->getTimeDistance(pickupSP->getLocation(), dropoffSP->getLocation()) + (boardingTime*pickupSP->getNumberOfPassengers());
        additionalCost += dst_to_dropoff;

        if (dst_to_pickup >= 0 && dst_to_dropoff >= 0)
        {
            pickupSP->setActualTime(dst_to_pickup + currentTime);
            dropoffSP->setActualTime(pickupSP->getActualTime() + dst_to_dropoff);

            EV << "Time needed to vehicle: " << vehicleID << " to reach pickup: " << pickupSP->getLocation() << " from current time, is: " << (pickupSP->getActualTime()-currentTime)/60  << " minutes." << endl;
            EV << "Time needed to vehicle: " << vehicleID << " to reach dropoff: " << dropoffSP->getLocation() << " from current time, is: " << (dropoffSP->getActualTime()-currentTime)/60 << " minutes." << endl;
        }


    //The vehicle can satisfy the request within its deadline
  //  if(dst_to_pickup != -1 && pickupSP->getActualTime() <= (pickupSP->getTime() + pickupSP->getMaxDelay()))// && dropoffSP->getActualTime() <= (dropoffSP->getTime() + dropoffSP->getMaxWaitingTime()))
  //  {
        for (auto const &x : old)
            newList.push_back(new StopPoint(*x));

        pickupSP->setActualNumberOfPassengers(pickupSP->getNumberOfPassengers());
        dropoffSP->setActualNumberOfPassengers(0);
        newList.push_back(pickupSP);
        newList.push_back(dropoffSP);

        proposal = new StopPointOrderingProposal(vehicleID,vehicleID, additionalCost, pickupSP->getActualTime(), newList);

    // }
 /*    else{
        delete pickupSP;
        delete dropoffSP;
    }*/
    }



    // The  vehicle has more stop points with more low priority
        else if( old.size() > 1  && newTypeID > oldFirstTypeID ) {

            EV<<"The vehicle "<<vehicleID<<"has new stop point with more high priority "<<endl;
            proposal = addStopPointToFirstPos(vehicleID, old, tr);
            EV<<"The vehicle "<<vehicleID<<"has new proposal! "<<endl;


        }


    else {
        int precProb = getPrecProb( tr->getTypeID(), old);
        int nextProb = getNextProb(tr->getTypeID(), old);


        if(simTime().dbl()> 11200 && tr->getTypeID() == 2) {
          EV<<"Prec Prob"<<precProb<<endl;
          EV<<"NextProb"<<nextProb<<endl;
          EV<<"VEHICLE ID "<<vehicleID<<endl;
          for(std::list<StopPoint*>::const_iterator it = old.begin(); it != old.end(); it++) {
                                          int idTR = pendingRequests[(*it)->getRequestID()]->getTypeID();
                                          EV<<idTR<<" ";
                                      }
                                     EV<<endl;

        }

        if((precProb == -1 && nextProb == -1) || (precProb != -1 && nextProb == -1)) {
            EV << "The vehicle " << vehicleID << " has other stop points!" << endl;




                   //Get last stop point for the vehicle
                   StopPoint *sp = old.back();
                   additionalCost = netmanager->getTimeDistance(sp->getLocation(), pickupSP->getLocation());
                   dst_to_pickup = additionalCost + (sp->getActualTime() - currentTime) + (alightingTime*abs(sp->getNumberOfPassengers())); //The last stop point is a dropOff point.
                   dst_to_dropoff = netmanager->getTimeDistance(pickupSP->getLocation(), dropoffSP->getLocation()) + (boardingTime*pickupSP->getNumberOfPassengers());
                   additionalCost += dst_to_dropoff;

                   if (dst_to_pickup >= 0 && dst_to_dropoff >= 0)
                   {
                       pickupSP->setActualTime(dst_to_pickup + currentTime);
                       dropoffSP->setActualTime(pickupSP->getActualTime() + dst_to_dropoff);

                       EV << "Time needed to vehicle: " << vehicleID << " to reach pickup: " << pickupSP->getLocation() << " from current time, is: " << (pickupSP->getActualTime()-currentTime)/60  << " minutes." << endl;
                       EV << "Time needed to vehicle: " << vehicleID << " to reach dropoff: " << dropoffSP->getLocation() << " from current time, is: " << (dropoffSP->getActualTime()-currentTime)/60 << " minutes." << endl;
                   }


               //The vehicle can satisfy the request within its deadline
             //  if(dst_to_pickup != -1 && pickupSP->getActualTime() <= (pickupSP->getTime() + pickupSP->getMaxDelay()))// && dropoffSP->getActualTime() <= (dropoffSP->getTime() + dropoffSP->getMaxWaitingTime()))
             //  {
                   for (auto const &x : old)
                       newList.push_back(new StopPoint(*x));

                   pickupSP->setActualNumberOfPassengers(pickupSP->getNumberOfPassengers());
                   dropoffSP->setActualNumberOfPassengers(0);
                   newList.push_back(pickupSP);
                   newList.push_back(dropoffSP);

                   proposal = new StopPointOrderingProposal(vehicleID,vehicleID, additionalCost, pickupSP->getActualTime(), newList);



        }


        else  {
            int pos = -1;
            if(precProb != -1 && nextProb != -1) {
                 pos = getLastPos( precProb, old);
            }
            else if(precProb == -1 && nextProb != -1) {
                 pos = getLastPos( tr->getTypeID(), old);
            }
            if(pos != -1) {
            std::list<StopPoint*>::const_iterator it = old.begin();
            StopPoint *lastAdded;
            for(int i = 0; i < pos+1; i++) {
                    newList.push_back(new StopPoint(**it));

                if(i == pos) {
                    lastAdded = *it;
                }
                it++;
            }


            additionalCost = netmanager->getTimeDistance(lastAdded->getLocation(), pickupSP->getLocation());
            dst_to_pickup = additionalCost + (lastAdded->getActualTime() - currentTime) + (alightingTime*abs(lastAdded->getNumberOfPassengers())); //The last stop point is a dropOff point.
            dst_to_dropoff = netmanager->getTimeDistance(pickupSP->getLocation(), dropoffSP->getLocation()) + (boardingTime*pickupSP->getNumberOfPassengers());
            additionalCost += dst_to_dropoff;

            if (dst_to_pickup >= 0 && dst_to_dropoff >= 0)
            {
                pickupSP->setActualTime(dst_to_pickup + currentTime);
                dropoffSP->setActualTime(pickupSP->getActualTime() + dst_to_dropoff);

                EV << "Time needed to vehicle: " << vehicleID << " to reach pickup: " << pickupSP->getLocation() << " from current time, is: " << (pickupSP->getActualTime()-currentTime)/60  << " minutes." << endl;
                EV << "Time needed to vehicle: " << vehicleID << " to reach dropoff: " << dropoffSP->getLocation() << " from current time, is: " << (dropoffSP->getActualTime()-currentTime)/60 << " minutes." << endl;
            }

            pickupSP->setActualNumberOfPassengers(pickupSP->getNumberOfPassengers());
            dropoffSP->setActualNumberOfPassengers(0);
            newList.push_back(pickupSP);
            newList.push_back(dropoffSP);


            StopPoint *precSP;
            double cost;
            double dst_to_sp;

            for(int i = pos+1; i < old.size(); i++) {
                StopPoint *sp = new StopPoint(**it);


                if(i == pos+1) {

                     cost = netmanager->getTimeDistance(dropoffSP->getLocation(), sp->getLocation());
                    dst_to_sp = additionalCost + (dropoffSP->getActualTime() - currentTime) + (alightingTime*abs(dropoffSP->getNumberOfPassengers()));
                    sp->setActualTime(dst_to_sp +currentTime);

                }
                else {
                    if(precSP->getIsPickup()) {
                        cost = netmanager->getTimeDistance(precSP->getLocation(), sp->getLocation());
                        dst_to_sp = additionalCost + (precSP->getActualTime() - currentTime) + (alightingTime*abs(precSP->getNumberOfPassengers()));
                        sp->setActualTime(dst_to_sp +currentTime);

                    }
                    else {
                        cost = netmanager->getTimeDistance(precSP->getLocation(), sp->getLocation());
                        dst_to_sp = additionalCost + (precSP->getActualTime() - currentTime) + (boardingTime*abs(precSP->getNumberOfPassengers()));
                        sp->setActualTime(dst_to_sp +currentTime);
                    }


                }

                newList.push_back(sp);
                it++;

                precSP = sp;




            }

            proposal = new StopPointOrderingProposal(vehicleID,vehicleID, additionalCost, pickupSP->getActualTime(), newList);


            }

        }

    }



    return proposal;
}




int RadioTaxiCoord::getLastPos(int firstProb, std::list<StopPoint*> spl) {
    int lastPos = 0;
    int i = 0;

    for (std::list<StopPoint*>::const_iterator it = spl.begin(), end = spl.end(); it != end; ++it) {
        StopPoint* tmpSP = *it;
        int prob = pendingRequests[tmpSP->getRequestID()]->getTypeID();
        if(firstProb == prob) {
            lastPos = i;
        }
        i++;

    }
    return lastPos;


}


int RadioTaxiCoord::getPrecProb(int prob, std::list<StopPoint*> spl) {
    int firstProb = -1;


    for (std::list<StopPoint*>::const_iterator it = spl.begin(), end = spl.end(); it != end; ++it) {
        StopPoint* tmpSP = *it;
        int tmpProb = pendingRequests[tmpSP->getRequestID()]->getTypeID();
        if(tmpProb - prob >=1  ) {
            firstProb = tmpProb;
        }


    }
    return firstProb;


}



int RadioTaxiCoord::getNextProb(int prob, std::list<StopPoint*> spl) {
    int nextProb = -1;


    for (std::list<StopPoint*>::const_iterator it = spl.begin(), end = spl.end(); it != end; ++it) {
        StopPoint* tmpSP = *it;
        int tmpProb = pendingRequests[tmpSP->getRequestID()]->getTypeID();
        if( prob - tmpProb >= 1  ) {
            nextProb = tmpProb;
            break;
          //  it = std::prec(spl.end());
        }


    }
    return nextProb;


}



StopPointOrderingProposal* RadioTaxiCoord::addStopPointToFirstPos(int vehicleID, std::list<StopPoint*> spl, TripRequest* tr) {

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
   /*     if(tmpSP->getIsPickup()) {
            tmpSP->setActualNumberOfPassengers(tmpSP->getActualNumberOfPassengers());
        }
        else {
            tmpSP->setActualNumberOfPassengers(0);
        }*/

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








