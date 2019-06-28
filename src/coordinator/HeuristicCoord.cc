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
 //   bool requestsReallocation = par("requestsReallocation");
    if(signalID == tripRequest)
    {

      TripRequest *tr = check_and_cast<TripRequest *>(obj);
      EV << "New TRIP request from: " << source->getFullPath() << endl;

      if(isRequestValid(*tr))
      {
          pendingRequests.insert(std::make_pair(tr->getID(), new TripRequest(*tr)));
          totrequests++;
          emit(totalRequestsPerTime, totrequests);



         //  updateAllScheduling();


          if(tr->getTypeID() == 1) {
              totYellowCodes++;
              emit(totalYellowCodesPerTime, totYellowCodes);


          }
          else if (tr->getTypeID() == 2)  {
              totRedCodes++;
              emit(totalRedCodesPerTime, totRedCodes);
          }

         int requestTypeID = tr->getTypeID();

         int vehicleID = handleTripRequest(tr, false);





  // occorrerebbe modificare anche le statistiche dei codici gialli quando avviene la riallocazione
          if(requestReallocation && vehicleID > -1 ) {
            //  bool sat = checkForSaturation(vehicleID);


      //   if(!sat) {


          std::list<TripRequest *> l = checkForRelocation(vehicleID);

          if(l.size() > 0)
              updateScheduling(vehicleID);

          if(!l.empty()) {
              for(auto &trx: l) {
                  EV<<"TRIP TO REALLOCATE"<<" REQ ID "<<trx->getID()<<" TYPEID "<<trx->getTypeID()<< "max time to  PickUp:  "<<trx->getPickupSP()->getTime()+trx->getPickupSP()->getMaxDelay()<<" location:"<<trx->getPickupSP()->getLocation()<<endl;
                 // trx->getDropoffSP()->setTime(trx->getDropoffSP()->getTime()-trx->getPickupSP()->getTime()+simtime);
                 // trx->getPickupSP()->setTime(simtime);
                  trx->getPickupSP()->setActualTime(0.0);
                  trx->getPickupSP()->setActualNumberOfPassengers(0);
                  trx->getDropoffSP()->setActualTime(0.0);
                  trx->getDropoffSP()->setActualNumberOfPassengers(0);

                  handleTripRequest(trx, true);

                //  simulation.endRun();


              }
              l.clear();
          }
        //  }
          }


      }
      else
      {
          EV << "The request " << tr->getID() << " is not valid!" << endl;
      }
    }

    else if (signalID == updateSchedulingS) {
       EV<<"NETWORK MODIFIED...UPDATE SCHEDULING"<<endl;
        updateAllScheduling();
    }


}











bool HeuristicCoord::checkForSaturation(int vehicleID) {
 bool sat = false;
 int count = 0;
 int countV = 0;

 for (auto const &v : vehicles) {

          int idV = v.first->getID();
          if(idV == vehicleID)
              continue;
          if(v.first->getTypeId() == 1)
              countV++;

          std::list<StopPoint*> spList = rPerVehicle[idV];
          if(!spList.empty())  {


          int minT = getMinTInSPList(spList);
          int maxT = getMaxTInSPList(spList);
          if(minT < maxT) {
              std::list<StopPoint*> spSubList;

              std::list<StopPoint*>::iterator it= rPerVehicle[idV].begin();
              while(it != rPerVehicle[idV].end() && pendingRequests[(*it)->getRequestID()]->getTypeID() != maxT)
                it++;
             // funziona se sono presenti solamnete codici gialli oltre ai rossi
              while(it != rPerVehicle[idV].end()) {
                  if(pendingRequests[(*it)->getRequestID()]->getTypeID()==1) {
                  spSubList.push_back((*it));
                  it++;
                  }
              }


          std::list<double> resl = getResidualTime(spSubList, -1);
         // std::list<double>::iterator it;
          for(int i = 0; i<resl.size(); i++) {
          double minRes = *(std::min_element(std::begin(resl), std::end(resl)));
         // if(minRes > 0 && minRes > actRes) {
              count++;
              break;
        //  }
          //lres.pop_front();
          }
          spSubList.clear();
          resl.clear();
          }

 }
 spList.clear();
 }
 if(count == countV) {
     sat = true;


 }
 return sat;

}



std::list<TripRequest*> HeuristicCoord::checkForRelocation(int vehicleID) {

   int  maxReallocForRedCodes = par("maxReallocForRedCodes");
    std::list<TripRequest*> l;
          int idV = vehicleID;
          int maxT = getMaxTypeRequestPriority();
          std::list<StopPoint*> spList = rPerVehicle[idV];
          if(!spList.empty())  {
          int minT = getMinTInSPList(spList);
       //   int maxTInL = getMaxTInSPList(spList);
         // if(minT < maxT /*&& maxTInL == maxT*/) {
            //  std::list<StopPoint*>::iterator it = getPosOfLastPBlock(rPerVehicle[idV].begin(), rPerVehicle[idV].end(), 2);
              std::list<StopPoint*>::iterator it= rPerVehicle[idV].begin();

              // Nel  caso in cui sp  di drop-off che fa riferimento ad una richiesta a più bassa priorita non si  prende
              //in  considerazione per un eventuale  ricollocazione
             // if(pendingRequests[(*it)->getRequestID()]->getTypeID() < maxT) {
           //    it++;
               // La ricollocazione riguarda le richieste a più bassa  priorità tra quelle pianificate
        //  while(it != rPerVehicle[idV].end() && pendingRequests[(*it)->getRequestID()]->getTypeID()==maxT)
              it++;

          for( it; it != rPerVehicle[idV].end();) {
              StopPoint *sp = (*it);
              TripRequest *tr = pendingRequests[sp->getRequestID()];

              bool okCheck = false;
              bool deleted = false;
             // if(tr->getTypeID() < maxT) {
                  if(((sp->getTime()+sp->getMaxDelay())-sp->getActualTime())<0) {
                      //  pendingRequests.erase(sp->getRequestID());
                      if(tr->getTypeID() == maxT) {
                        //  numReallocPerRedCode.
                          if(numReallocPerRedCode[tr->getID()]<maxReallocForRedCodes) {

                              okCheck=true;
                              numReallocPerRedCode[tr->getID()] =numReallocPerRedCode[tr->getID()]+1;
                          }

                      }
                      else {
                          okCheck=true;
                      }

                      if(sp->getIsPickup() && okCheck ) {
                          l.push_back(new TripRequest(*pendingRequests[sp->getRequestID()]));

                          it = rPerVehicle[idV].erase(it);
                        //   it++;
                          //DA ERRORE
                       //   delete(sp);

                          EV<<"SP PICKUP OF REQUEST  ID: "<<tr->getID()<<" DELETED FROM VEHICLE: "<<idV<<endl;
                        //  break;
                      deleted = true;
                   //   deleteDropOffSPromSPList(idV, sp->getRequestID());
                      }

                  }
                //  else
                 //     deleted = false;

           //  }
              if(!deleted)
                  it++;

        //  }

    }
          //eliminazione degli sp di drop-off
       //   it= getPosOfLastPBlock(rPerVehicle[idV].begin(), rPerVehicle[idV].end(), 2);
           it= rPerVehicle[idV].begin();
          //deleted = false;
          if(l.size()>0) {
          //while(it != rPerVehicle[idV].end() && pendingRequests[(*it)->getRequestID()]->getTypeID()<maxT)
           //          it++;

                 for(it; it != rPerVehicle[idV].end();) {
                     StopPoint *sp = (*it);
                    // TripRequest *tr = pendingRequests[sp->getRequestID()];
                     bool deleted = false;
                    // if(tr->getTypeID() < maxT) {
                         for(const auto &tmpTr: l) {
                             if(tmpTr->getID() == sp->getRequestID()) {
                      //   if(((sp->getTime()+sp->getMaxDelay())-sp->getActualTime())<0) {
                             //  pendingRequests.erase(sp->getRequestID());

                             if(!sp->getIsPickup()) {


                                 it=rPerVehicle[idV].erase(it);

                                 //DA ERRORE
                              //   delete(sp);
                                 EV<<"SP DROPOFF OF REQUEST  ID: "<<sp->getRequestID()<<" DELETED FROM VEHICLE: "<<idV<<endl;
                                 deleted = true;
                               //  break;
                             }
                             }
                         }


                     if(!deleted)
                         it++;

               //  }

           }
          }






             // }

}

spList.clear();
   // }
    return l;
}




void HeuristicCoord::updateScheduling(int vehicleID, int actVehicleLoc) {


    EV<<"UPDATE SCHEDULING..."<<endl;
    int state = getVehicleByID(vehicleID)->getState();
    double GAP = 5.80;
    double dtFromActPos=0.0;
    double delay_BA = 0.0;
    bool changed = false;
    if(!rPerVehicle[vehicleID].empty()) {

     std::list<StopPoint*>::iterator it= rPerVehicle[vehicleID].begin();

  //  if(state == -1 || state == 0)   {
     if(actVehicleLoc!= (*it)->getLocation()) {
     dtFromActPos=  netmanager->getTimeDistance(actVehicleLoc,(*it)->getLocation());
     double precTime =  (*it)->getActualTime();
     (*it)->setActualTime(simTime().dbl()+dtFromActPos-GAP);
     if( (*it)->getActualTime() > precTime || (*it)->getActualTime() < precTime)
         changed = true;
  //  }
  /*  else {
        StopPoint * sp =lastSPPerVehicle[vehicleID];
        if(sp->getIsPickup())
            delay_BA = boardingTime*(sp->getNumberOfPassengers());
        else {
            delay_BA = alightingTime*abs(sp->getNumberOfPassengers());
        }

        dtFromActPos=  netmanager->getTimeDistance(actVehicleLoc,(*it)->getLocation())+delay_BA;

      //  (*it)->setActualTime(sp->getActualTime()+dtFromActPos);

    (*it)->setActualTime(simTime().dbl()+dtFromActPos);

    }*/


    std::list<StopPoint*>::iterator it2;
    double dt = -1.0;
    if(rPerVehicle[vehicleID].size()>1) {
    for (it; it!=std::prev(rPerVehicle[vehicleID].end()); ++it) {
           it2 = it;

              int passengers = (*it)->getActualNumberOfPassengers();


               if(it2 != (rPerVehicle[vehicleID].end()))
               {
                   it2++;

                   if((*it)->getIsPickup())
                                   dt = netmanager->getTimeDistance((*it)->getLocation(), (*it2)->getLocation()) + (boardingTime*(*it)->getNumberOfPassengers());
                                else
                                   dt = netmanager->getTimeDistance((*it)->getLocation(), (*it2)->getLocation()) + alightingTime*abs((*it)->getNumberOfPassengers());

                                EV << " Distance from " << (*it)->getLocation() << " to " <<  (*it2)->getLocation() << " is " << dt << endl;

                                (*it2)->setActualTime((*it)->getActualTime()+dt);
                                (*it2)->setActualNumberOfPassengers(passengers+(*it2)->getNumberOfPassengers());




               }



    }
    }
     }
    }

    if(changed) {
        updateOverTimeRequestsStatistic(rPerVehicle[vehicleID],1);
        updateOverTimeRequestsStatistic(rPerVehicle[vehicleID],2);
    }


    if(changed) {

        std::list<TripRequest *> l = checkForRelocation(vehicleID);

                 if(l.size() > 0)
                     updateScheduling(vehicleID);

                 if(!l.empty()) {
                     for(auto &trx: l) {
                         EV<<"TRIP TO REALLOCATE IN UPDATE SCHEDULING"<<" REQ ID "<<trx->getID()<<" TYPEID "<<trx->getTypeID()<< "max time to  PickUp:  "<<trx->getPickupSP()->getTime()+trx->getPickupSP()->getMaxDelay()<<" location:"<<trx->getPickupSP()->getLocation()<<endl;
                        // trx->getDropoffSP()->setTime(trx->getDropoffSP()->getTime()-trx->getPickupSP()->getTime()+simtime);
                        // trx->getPickupSP()->setTime(simtime);
                         trx->getPickupSP()->setActualTime(0.0);
                         trx->getPickupSP()->setActualNumberOfPassengers(0);
                         trx->getDropoffSP()->setActualTime(0.0);
                         trx->getDropoffSP()->setActualNumberOfPassengers(0);

                         int vehicleIDR=handleTripRequest(trx, true);

                     }
                     l.clear();
                 }

    }



}








void HeuristicCoord::updateScheduling(int vehicleID) {




    std::list<StopPoint*>::iterator it= rPerVehicle[vehicleID].begin();
    std::list<StopPoint*>::iterator it2;
    double dt = -1.0;

    for (it; it!=std::prev(rPerVehicle[vehicleID].end()); ++it) {
           it2 = it;

              int passengers = (*it)->getActualNumberOfPassengers();


               if(it2 != (rPerVehicle[vehicleID].end()))
               {
                   it2++;

                   if((*it)->getIsPickup())
                                   dt = netmanager->getTimeDistance((*it)->getLocation(), (*it2)->getLocation()) + (boardingTime*(*it)->getNumberOfPassengers());
                                else
                                   dt = netmanager->getTimeDistance((*it)->getLocation(), (*it2)->getLocation()) + alightingTime*abs((*it)->getNumberOfPassengers());

                                EV << " Distance from " << (*it)->getLocation() << " to " <<  (*it2)->getLocation() << " is " << dt << endl;

                                (*it2)->setActualTime((*it)->getActualTime()+dt);
                                (*it2)->setActualNumberOfPassengers(passengers+(*it2)->getNumberOfPassengers());




               }



    }

}






int HeuristicCoord::handleTripRequest(TripRequest *tr, bool isReAllocation)
{
    std::map<int,StopPointOrderingProposal*> vehicleProposals;
    //bool stop = false;

    /*if ( simTime().dbl() > 28700)
                  return;*/



    for (auto const &x : vehicles)
    {

        if(checkRequestVeichleTypesMatching(tr->getTypeID(), x.first->getTypeId()) ) {


          //  if(simTime().dbl() > 24000)
            //        return;


        //Check if the vehicle has enough seats to serve the request
        if(x.first->getSeats() >= tr->getPickupSP()->getNumberOfPassengers())
        {

            StopPointOrderingProposal *tmp = eval_requestAssignment(x.first->getID(), tr, isReAllocation);
            if(tmp && !tmp->getSpList().empty())
                vehicleProposals[x.first->getID()] = tmp;

        }
        }


    }


    //Assign the request to the vehicle which minimize the waiting time

    //else {

        if(isReAllocation) {
            if(requestAssignmentStrategy == 0)
              return   minCostAssignment(vehicleProposals, tr,isReAllocation);
            return  minWaitingTimeAssignmentWithReloc(vehicleProposals, tr, isReAllocation);
        }
        else {
            if(requestAssignmentStrategy == 0)
                  return   minCostAssignment(vehicleProposals, tr);
          return  minWaitingTimeAssignment(vehicleProposals, tr);

       // }
   // }
}

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
StopPointOrderingProposal* HeuristicCoord::eval_requestAssignment(int vehicleID, TripRequest* tr, bool isReallocation)
{
    //if(isReallocation)
       //     return NULL;

    std::list<StopPoint*> old = rPerVehicle[vehicleID];
    std::list<StopPoint*> newList;
    StopPoint* newTRpickup = new StopPoint(*tr->getPickupSP());
    StopPoint* newTRdropoff = new StopPoint(*tr->getDropoffSP());
    StopPointOrderingProposal* toReturn = NULL;
    int maxT = getMaxTypeRequestPriority();
    int maxTInList = getMaxTInSPList(old);
    int minTInList = getMinTInSPList(old);

    newTRdropoff->setNumberOfPassengers(-newTRpickup->getNumberOfPassengers());
    double additionalCost = -1;


    int newTypeID = tr->getTypeID();
    int oldFirstTypeID = -1;

    EV<<"REQUEST "<<tr->getID()<<" EVAL AT TIME "<<tr->getPickupSP()->getTime()<<endl;

    if(!old.empty()) {
     oldFirstTypeID = pendingRequests[old.front()->getRequestID()]->getTypeID();

   //  EV<<"REQUEST IN FRONT IS PICKUP "<<old.front()->getIsPickup()<<endl;
     EV<<"MAX T IN LIST OF "<<vehicleID<<"is "<<maxTInList<<endl;
    }



    //The vehicle is empty
    if(rPerVehicle.find(vehicleID) == rPerVehicle.end() || old.empty())
    {

       // if(isReallocation)
         //    return NULL;

        EV << "The vehicle " << vehicleID << " has not other stop points!" << endl;
        additionalCost = netmanager->getTimeDistance(getLastVehicleLocation(vehicleID), newTRpickup->getLocation());
        double timeToPickup = additionalCost + simTime().dbl();
        double distancePToD = netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation());
        additionalCost+=distancePToD + (boardingTime*newTRpickup->getNumberOfPassengers());

       // if(timeToPickup <= (newTRpickup->getTime() + newTRpickup->getMaxDelay()))
           //    {

        if(timeToPickup > (newTRpickup->getTime() + newTRpickup->getMaxDelay()) && tr->getTypeID() < maxT ) {
            EV << " The vehicle " << vehicleID << " can not serve the Request " << tr->getID() <<" of type "<<tr->getType()<<endl;
                  delete newTRpickup;
                  delete newTRdropoff;


        }
        else {
            newTRpickup->setActualTime(timeToPickup);
            newTRpickup->setActualNumberOfPassengers(newTRpickup->getNumberOfPassengers());
            newTRdropoff->setActualTime(newTRpickup->getActualTime() + distancePToD + (boardingTime*newTRpickup->getActualNumberOfPassengers()));
            newTRdropoff->setActualNumberOfPassengers(0);
            newList.push_back(newTRpickup);
            newList.push_back(newTRdropoff);

            toReturn=new StopPointOrderingProposal(vehicleID,vehicleID, additionalCost, timeToPickup, newList);
            EV << "New Pickup can be reached at " << newTRpickup->getActualTime() << " by the vehicle " << vehicleID << ". Max allowed time is: " << (newTRpickup->getTime() + newTRpickup->getMaxDelay()) << endl;
            EV << "New Dropoff can be reached at " << newTRdropoff->getActualTime() << " by the vehicle " << vehicleID << ". Max allowed time is: " << (newTRdropoff->getTime() + newTRdropoff->getMaxDelay()) << endl;
        }
            //    }
     /*   else {
            EV << " The vehicle " << vehicleID << " can not serve the Request " << tr->getID() << endl;
                    delete newTRpickup;
                    delete newTRdropoff;

        }*/

    }

    //The vehicle has 1 stop point
    else if(old.size() == 1/* &&    newTypeID <= maxTInList*/)
    {
       // if(isReallocation)
            //   return NULL;


        EV << " The vehicle " << vehicleID << " has only 1 SP. Trying to push new request back..." << endl;

        StopPoint *last = new StopPoint(*old.back());

        additionalCost = netmanager->getTimeDistance(last->getLocation(), newTRpickup->getLocation());
        double timeToPickup = additionalCost + last->getActualTime() + (alightingTime*abs(last->getNumberOfPassengers())); //The last SP is a dropOff point.
        double distancePtoD = netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation());

        additionalCost+= distancePtoD + (boardingTime*newTRpickup->getNumberOfPassengers());

        if(timeToPickup > (newTRpickup->getTime() + newTRpickup->getMaxDelay()) && tr->getTypeID() < maxT ) {
            EV << " The vehicle " << vehicleID << " can not serve the Request " << tr->getID() <<" of type "<<tr->getType()<<endl;
                         delete newTRpickup;
                         delete newTRdropoff;
                         delete last;


               }
        else {
            newTRpickup->setActualTime(timeToPickup);
            newTRdropoff->setActualTime(newTRpickup->getActualTime() + distancePtoD + (boardingTime*newTRpickup->getNumberOfPassengers()));
            newTRpickup->setActualNumberOfPassengers(newTRpickup->getNumberOfPassengers());
            newTRdropoff->setActualNumberOfPassengers(0);
            newList.push_back(last);
            newList.push_back(newTRpickup);
            newList.push_back(newTRdropoff);

            toReturn=new StopPointOrderingProposal(vehicleID,vehicleID, additionalCost, timeToPickup, newList);
            EV << "New Pickup can be reached at " << newTRpickup->getActualTime() << " by the vehicle " << vehicleID << ". Max allowed time is: " << (newTRpickup->getTime() + newTRpickup->getMaxDelay()) << endl;
            EV << "New Dropoff can be reached at " << newTRdropoff->getActualTime() << " by the vehicle " << vehicleID << ". Max allowed time is: " << (newTRdropoff->getTime() + newTRdropoff->getMaxDelay()) << endl;
        }

    }



// Alla nuova richiesta viene assegnata la prima posizione se ha una priorità maggiore rispetto a quelle già schedulate
  /*  else if( old.size() > 1  && newTypeID > maxTInList ) {


        EV<<"The vehicle "<<vehicleID<<"has new stop point with more high priority! "<<endl;
       toReturn = addRequestToFirstPos(vehicleID, old, tr);
       if(toReturn == NULL) {
           EV << " The vehicle " << vehicleID << " can not serve the Request " << tr->getID() <<" of type "<<tr->getType()<<endl;
           delete newTRpickup;
           delete newTRdropoff;


       }


    }*/


    //The vehicle has more stop points
    else
    {

     //   updateScheduling(vehicleID);


        std::list<StopPoint *> subL;

                std::list<StopPoint *>::iterator startPos = old.begin();
                  std::list<StopPoint *>::iterator lastValidPos= old.begin();

                  // in questo caso individua il sotto insieme di sp in cui è presente
                  // l'ultimo sp maggiore o uguale in priorita a quelli da inserire
        if(maxTInList > minTInList) {


              for(int refP=maxTInList; refP>minTInList; refP--) {
                  startPos = getPosOfLastPBlock(lastValidPos, old.end(), refP);

                  if(startPos != old.end() && refP>=newTypeID) {
                     lastValidPos = startPos;
                     subL.clear();
                     subL.insert(subL.begin(),lastValidPos, old.end());
                     if(refP == newTypeID)
                        break;

                  }


              }
              }
               // in questo caso tutti gli sp hanno la stessa priorita per cui si considera
               // l'intero insieme e i nuovi  sp  verranno inseriti in coda.
              else
                  subL = old;



        EV << " The vehicle " << vehicleID << " has more stop points..." << endl;



        //Get all the possible sorting within the Pickup
        std::list<StopPointOrderingProposal*> proposalsWithPickup = addStopPointToTrip(vehicleID, subL, newTRpickup, newTRdropoff->getLocation());
        delete newTRpickup;

        std::list<StopPointOrderingProposal*> proposalsWithDropoff;

        if(!proposalsWithPickup.empty())
        {
            int cost = -1;
            EV << "STARTING DROPOFF EVALUATION..." << endl;
            for(std::list<StopPointOrderingProposal*>::const_iterator it2 = proposalsWithPickup.begin(), end2 = proposalsWithPickup.end(); it2 != end2; ++it2)
            {
                //Get all the possible sorting within the Dropoff
                proposalsWithDropoff = addStopPointToTrip(vehicleID, (*it2)->getSpList(), newTRdropoff, -1);

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
            //  if((*old.begin()) !=  (*toReturn->getSpList().begin()))
            if( toReturn!=NULL  && subL.size()<old.size() ) {
             // toReturn->getSpList().insert(toReturn->getSpList().begin(), old.begin(), lastValidPos);
              std::list<StopPoint*> newL;
              std::list<StopPoint*> tmpL;

              for(std::list<StopPoint*>::const_iterator it = old.begin(); it != old.end(); it++) {
                  if(it == lastValidPos)
                      break;
                  tmpL.push_back(new StopPoint(**it));
              }



             newL.insert(newL.begin(),tmpL.begin(), tmpL.end());
                std::list<StopPoint*> returnL = toReturn->getSpList();
                newL.insert(newL.end(),returnL.begin(), returnL.end());
                toReturn->setSpList(newL);


              EV<<"SPLIST EL SIZE"<<toReturn->getSpList().size()<<endl;
              for(std::list<StopPoint*>::iterator it4 = newL.begin(); it4 != newL.end(); it4++) {
                      int idTR = pendingRequests[(*it4)->getRequestID()]->getTypeID();
                      EV<<idTR<<" ";
                  }

              EV<<"OLD EL "<<endl;
              for(std::list<StopPoint*>::const_iterator it = old.begin(); it != old.end(); it++) {
                    if(it == lastValidPos)
                        break;
                   // tmpL.push_back(new StopPoint(**it));
                  //  toReturn->getSpList().push_front((*it));
                  int idTR = pendingRequests[(*it)->getRequestID()]->getTypeID();
                                EV<<idTR<<" ";
                            }


              for(const auto &el:subL) {

                               EV<<"SUBLIST EL "<<pendingRequests[el->getRequestID()]->getTypeID()<<endl;

                              }
            //  return NULL;


            }
           //   toReturn->setSpList(newL)
        }

        else
        {
            EV << " The vehicle " << vehicleID << " can not serve the Request " << tr->getID() <<" of type "<<tr->getType()<<endl;
            delete newTRdropoff;
            delete toReturn;
        }
    }


    return toReturn;
}




















/**
 * Viene posizionata la  richiesta in prima posizione
 */
StopPointOrderingProposal* HeuristicCoord::addRequestToFirstPos(int vehicleID, std::list<StopPoint*> spl, TripRequest* tr) {

    StopPointOrderingProposal *  spo = NULL;
    StopPoint* newTRpickup = new StopPoint(*tr->getPickupSP());
    StopPoint* newTRdropoff = new StopPoint(*tr->getDropoffSP());
    std::list<StopPoint *> reOrderedList;
    StopPoint *last;
    StopPoint *prec;
  //  StopPoint *succ;
    last = newTRdropoff;
    double additionalCost = 0;
    double timeIncr = 0;
    int numPos = 0;
  //  int num = 0;
    int maxT = getMaxTypeRequestPriority();
    int vehicleSeats = getVehicleByID(vehicleID)->getSeats();
    int vehicleState = getVehicleByID(vehicleID)->getState();
    std::list<StopPoint*>::const_iterator it = spl.begin();
    bool listComplete = false;
    double addCost=0.0;



    last = (*it);

  /* if(vehicleState == 0 || vehicleState == -1) {

       additionalCost = netmanager->getTimeDistance(getLastVehicleLocation(vehicleID), newTRpickup->getLocation());
         double timeToPickup = additionalCost + simTime().dbl();
         double distancePtoD = netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation());
         additionalCost+=distancePtoD + (boardingTime*newTRpickup->getNumberOfPassengers());

         double newPathCost = additionalCost + netmanager->getTimeDistance(newTRdropoff->getLocation(), last->getLocation()) + alightingTime*abs(newTRdropoff->getNumberOfPassengers());
         double oldPathCost = netmanager->getTimeDistance(getLastVehicleLocation(vehicleID), last->getLocation());

         addCost = abs(newPathCost-oldPathCost);
         //addCost = additionalCost;


         if(timeToPickup > (newTRpickup->getTime() + newTRpickup->getMaxDelay()) && tr->getTypeID() < maxT ) {
              //  delete last;
              //  delete spo;
                EV << " The vehicle " << vehicleID << " can not serve the Request " << tr->getID() <<" of type "<<tr->getType()<<endl;
                return NULL;
         }

         newTRpickup->setActualTime(timeToPickup);

         newTRpickup->setActualNumberOfPassengers(newTRpickup->getNumberOfPassengers());

         newTRdropoff->setActualTime(newTRpickup->getActualTime() +distancePtoD + (boardingTime*newTRpickup->getActualNumberOfPassengers()));
         newTRdropoff->setActualNumberOfPassengers(0);


          reOrderedList.push_back(newTRpickup);
          reOrderedList.push_back(newTRdropoff);
          EV<<"New request with more high priority is positioned in 1st place!"<<endl;



        //  additionalCost = newTRdropoff->getActualTime() + netmanager->getTimeDistance(newTRdropoff->getLocation(), spl.front()->getLocation()) + alightingTime*abs(newTRdropoff->getNumberOfPassengers());
          last = newTRdropoff;






   }*/

  // else { //  if(!last->getIsPickup() &&  newTRpickup->getNumberOfPassengers()+last->getActualNumberOfPassengers()-last->getActualNumberOfPassengers() >= vehicleSeats  || last->getIsPickup() &&  newTRpickup->getNumberOfPassengers()+last->getActualNumberOfPassengers()-last->getNumberOfPassengers() > vehicleSeats) {

             // reOrderedList.push_back(last);
                    prec = last;
                   // EV<<"In first pos there is a drop-off.Check Number Of Passengers"<<endl;
                   // it++;
                  //  EV<<"In first position there is a drop-off with no available seats.Can't be passed!"<<endl;
                    reOrderedList.push_back(new StopPoint(*prec));
                 //   if(spl.size() > 1) {

                       // return NULL;
                        it++;
                    for ( it; it != spl.end(); ++it) {

                        last = (*it);
                        if(last->getIsPickup() &&  newTRpickup->getNumberOfPassengers()+last->getActualNumberOfPassengers()-last->getActualNumberOfPassengers() > vehicleSeats || !last->getIsPickup() &&  newTRpickup->getNumberOfPassengers()+last->getActualNumberOfPassengers()-last->getActualNumberOfPassengers() >= vehicleSeats) {
                            reOrderedList.push_back(new StopPoint(**it));
                           numPos++;
                           prec = last;

                        }
                        else {
                           --it;
                           break;
                        }

                       }


                 //   }


             // EV<<"In first position there is a drop-off with no available seats.Can't be passed!"<<endl;
             //  additionalCost = netmanager->getTimeDistance(prec->getLocation(), newTRpickup->getLocation());
              // double timeToPickup = additionalCost + prec->getActualTime() + (alightingTime*abs(prec->getNumberOfPassengers())); //The last SP is a dropOff point.
             //  double distancePtoD = netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation());

                double dtFromPrec = 0.0;
               if(prec->getIsPickup())
                dtFromPrec = netmanager->getTimeDistance(prec->getLocation(), newTRpickup->getLocation()) + boardingTime*(prec->getNumberOfPassengers());
               else
                dtFromPrec = netmanager->getTimeDistance(prec->getLocation(), newTRpickup->getLocation()) + alightingTime*abs(prec->getNumberOfPassengers());

               double dtToDrop = netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation()) + boardingTime*(newTRpickup->getNumberOfPassengers());


              // additionalCost+= distancePtoD + (boardingTime*newTRpickup->getNumberOfPassengers());

             //  double cost = additionalCost+ distancePtoD + alightingTime*abs(prec->getNumberOfPassengers());


               if((prec->getActualTime()+dtFromPrec) > (newTRpickup->getTime() + newTRpickup->getMaxDelay()) && tr->getTypeID() < maxT ) {
                   EV << " The vehicle " << vehicleID << " can not serve the Request " << tr->getID() <<" of type "<<tr->getType()<<endl;
                            //    delete newTRpickup;
                         //       delete newTRdropoff;
                              //  delete last;
                             //   delete spo;
                                return NULL;

                      }

                   newTRpickup->setActualTime(prec->getActualTime()+dtFromPrec);
                   newTRdropoff->setActualTime(newTRpickup->getActualTime() + dtToDrop);
                   newTRpickup->setActualNumberOfPassengers(newTRpickup->getNumberOfPassengers()+prec->getActualNumberOfPassengers());
                   newTRdropoff->setActualNumberOfPassengers(newTRpickup->getActualNumberOfPassengers()-newTRpickup->getNumberOfPassengers());
                  // reOrderedList.push_back(new StopPoint(*prec));
                   reOrderedList.push_back(newTRpickup);
                   reOrderedList.push_back(newTRdropoff);

                   // in questo caso last rappresenta il successivo sp rispetto al sp di drop-off della
                    //  nuova richiesta
                   //viceversa o è presente un solo elemento nella vecchia lista o è l'ultimo elemento

                   if(prec != last) {

                     //  double newPathCost = cost + netmanager->getTimeDistance(newTRdropoff->getLocation(), last->getLocation()) + alightingTime*abs(newTRdropoff->getNumberOfPassengers());

                       double newPathCost = dtFromPrec+dtToDrop +netmanager->getTimeDistance(newTRdropoff->getLocation(), last->getLocation()) + alightingTime*abs(newTRdropoff->getNumberOfPassengers());

                     // prec deve essere di drop-off necessariamente
                       double  oldPathCost =  last->getActualTime() - prec->getActualTime();
                        addCost =  abs(newPathCost-oldPathCost);


                    last = newTRdropoff;
                    it++;




                   //  ins = false;

                   // listComplete = false;
                    EV<<"New request with more high priority is positioned in "<< numPos<<" position!"<<endl;
                  }
                   else {
                       EV<<"New request with more high priority is positioned in second or last "<< numPos++<<" position!"<<endl;
                      listComplete = true;
                      addCost = dtFromPrec+dtToDrop;
                      spo=new StopPointOrderingProposal(vehicleID,vehicleID, addCost, newTRpickup->getActualTime(), reOrderedList);
                      EV<<"Vehicle  "<<vehicleID<<" Proposed sp List: ";
                            std::list<StopPoint *> spList = spo->getSpList();
                            for(std::list<StopPoint*>::const_iterator it = spList.begin(); it != spList.end(); it++) {
                                int idTR = pendingRequests[(*it)->getRequestID()]->getTypeID();
                                if((*it)->getIsPickup())
                                                                    EV<<idTR<<"pk-"<<(*it)->getActualNumberOfPassengers()<<" ";
                                                                    else
                                                                        EV<<idTR<<"dr-"<<(*it)->getActualNumberOfPassengers()<< " ";
                            }
                   }


                  // last = newTRdropoff;
                   // Si deve riempire la nuova lista a partire dal secondo elemento della vecchia in quanto
                   // il primo e gia stato inserito

              //  EV<<"New request with more high priority is positioned in 2nd place!"<<endl;


           // return spo;
   // }
 /*   else {



   additionalCost = netmanager->getTimeDistance(getLastVehicleLocation(vehicleID), newTRpickup->getLocation());
   double timeToPickup = additionalCost + simTime().dbl();
   double distancePtoD = netmanager->getTimeDistance(newTRpickup->getLocation(), newTRdropoff->getLocation());
   additionalCost+=distancePtoD + (boardingTime*newTRpickup->getNumberOfPassengers());

   double newPathCost = additionalCost;
   double oldPathCost = netmanager->getTimeDistance(getLastVehicleLocation(vehicleID), last->getLocation());

   addCost = abs(newPathCost-last->getActualTime());
   //addCost = additionalCost;


   if(timeToPickup > (newTRpickup->getTime() + newTRpickup->getMaxDelay()) && tr->getTypeID() < maxT ) {
        //  delete last;
        //  delete spo;
          EV << " The vehicle " << vehicleID << " can not serve the Request " << tr->getID() <<" of type "<<tr->getType()<<endl;
          return NULL;
   }

   newTRpickup->setActualTime(timeToPickup);

   newTRpickup->setActualNumberOfPassengers(newTRpickup->getNumberOfPassengers());

   newTRdropoff->setActualTime(newTRpickup->getActualTime() +distancePtoD + (boardingTime*newTRpickup->getActualNumberOfPassengers()));
   newTRdropoff->setActualNumberOfPassengers(0);


    reOrderedList.push_back(newTRpickup);
    reOrderedList.push_back(newTRdropoff);
    EV<<"New request with more high priority is positioned in 1st place!"<<endl;



  //  additionalCost = newTRdropoff->getActualTime() + netmanager->getTimeDistance(newTRdropoff->getLocation(), spl.front()->getLocation()) + alightingTime*abs(newTRdropoff->getNumberOfPassengers());
    last = newTRdropoff;









            spo=new StopPointOrderingProposal(vehicleID,vehicleID, addCost, newTRpickup->getActualTime(), reOrderedList);
            EV<<"Vehicle  "<<vehicleID<<" Proposed sp List: ";
                                      std::list<StopPoint *> spList = spo->getSpList();
                                      for(std::list<StopPoint*>::const_iterator it = spList.begin(); it != spList.end(); it++) {
                                          int idTR = pendingRequests[(*it)->getRequestID()]->getTypeID();
                                          if((*it)->getIsPickup())
                                                                              EV<<idTR<<"pk-"<<(*it)->getActualNumberOfPassengers()<<" ";
                                                                              else
                                                                                  EV<<idTR<<"dr-"<<(*it)->getActualNumberOfPassengers()<< " ";
                                      }











    }*/




if(!listComplete) {

        for ( it; it != spl.end(); ++it) {
            StopPoint *tmpSp = new StopPoint(**it);
            reOrderedList.push_back(tmpSp);

           // la nuova richiesta a maggiore priorita influisce solamente sul tempo, non sul numero di
            // passeggeri in quanto sp di pick-up e drop-off sono uno dietro l'altro.
            tmpSp->setActualTime(tmpSp->getActualTime()+addCost);
           // tmpSp->setActualNumberOfPassengers(tmpSp->getNumberOfPassengers());



            last = tmpSp;
        }




        spo=new StopPointOrderingProposal(vehicleID,vehicleID, addCost, newTRpickup->getActualTime(), reOrderedList);
        EV<<"Vehicle  "<<vehicleID<<" Proposed sp List: ";
                                  std::list<StopPoint *> spList = spo->getSpList();
                                  for(std::list<StopPoint*>::const_iterator it = spList.begin(); it != spList.end(); it++) {
                                      int idTR = pendingRequests[(*it)->getRequestID()]->getTypeID();
                                      if((*it)->getIsPickup())
                                                                          EV<<idTR<<"pk-"<<(*it)->getActualNumberOfPassengers()<<" ";
                                                                          else
                                                                              EV<<idTR<<"dr-"<<(*it)->getActualNumberOfPassengers()<< " ";
                                  }


}

return spo;



}




std::list<StopPointOrderingProposal*> HeuristicCoord::addStopPointToTrip(int vehicleID, std::list<StopPoint*> spl, StopPoint* newSP, int dropOffLocation)
{
    int passengers = 0;
    int delay_BA = 0; // boarding/alighting delay.
    int vehicleSeats = getVehicleByID(vehicleID)->getSeats();
    double cost = -1.0;
    double actualTime = 0.0;
    double minResidual = 0.0;
    bool constrainedPosition = false;
    bool stopPositioning = false;
    bool afterLastDrpoff = false;
    bool forbiddenCheckPos = true;
    int maxTInList = getMaxTInSPList(spl);
    int maxT = getMaxTypeRequestPriority();


   // bool skippedProposal = false;
    double distanceTo, distanceFrom, dt, df = 0.0;

    int i = 0;
    std::list<StopPointOrderingProposal*> mylist;

    std::list<double> residualTimes;
    std::list<StopPoint*>::const_iterator it = spl.begin();
    std::list<StopPoint*>::const_iterator it2, it3 = spl.end();

    int reqTypeID = pendingRequests[newSP->getRequestID()]->getTypeID();




    // Si crea a partire dalla lista spl una lista ridotta, contenente gli StopPoint che fanno riferimento tutti a delle richieste dello
    // stesso tipo di quella nuova

    std::list<StopPoint *> redSpl(0);


    for(it; it != spl.end(); ++it) {
        StopPoint *sp = (*it);
        //    EV<<"Spl LIst El num "<<j<<endl;
        if(reqTypeID == pendingRequests[sp->getRequestID()]->getTypeID()) {
            redSpl.push_back(sp);
            //     EV<<"Added El num "<<j<<endl;
            //  EV<<"EL REq_Type_ID"<<pendingRequests[sp->getRequestID()]->getTypeID()<<endl;
        }
    }


    it = spl.begin();


    // A partire dalla lista ridotta vengono calcolati  i residui temporali

    if(newSP->getIsPickup())
    {
        delay_BA = (boardingTime*newSP->getNumberOfPassengers());
        residualTimes = getResidualTime(redSpl,-1);
    }
    else
    {
        delay_BA = (alightingTime*abs(newSP->getNumberOfPassengers()));
        residualTimes = getResidualTime(redSpl,newSP->getRequestID());
        //Move until the pickup stop point

        for (it; it != spl.end(); ++it) {
            if((*it)->getRequestID() == newSP->getRequestID())
                break;
        }

    }



    if(redSpl.size()>0)
    redSpl.clear();




    for (it; it!=spl.end(); ++it) {
        it2 = it;
        i++;

        if(pendingRequests[(*it)->getRequestID()]->getTypeID() == reqTypeID )
            residualTimes.pop_front();


         // i casi particolari in cui ad esempio si ha 1122 perchè non è stato possibile inserire
        // la richiesta 22 al primo posto per mancanza di posti l'inserimento puo essere fatto solo
        // dopo il secondo uno se la richiesta e a priorita 2 altrimenti alla fine se a priorita 1
      /*  if( spl.size() > 2) {

            if (it2 != (std::prev(spl.end()))) {
             it2++;

             int itPSucc = pendingRequests[(*it)->getRequestID()]->getTypeID();
             int it2PSucc = pendingRequests[(*it2)->getRequestID()]->getTypeID();
            if(itPSucc == maxTInList || (it2PSucc == maxTInList && reqTypeID >= it2PSucc)) {
              forbiddenCheckPos = false;
              EV<<"Succ SP type "<<it2PSucc<<" New sp type "<<reqTypeID<<endl;
             it2 = it;
             EV<<"This is poisition of SP with max P "<<itPSucc<<" of pos: "<<i<<endl;

            }

        }

        }
        else
            forbiddenCheckPos = false;

        if(forbiddenCheckPos)
            continue;*/



        if((*it)->getActualNumberOfPassengers() + newSP->getNumberOfPassengers() <= vehicleSeats)
        {
            passengers = (*it)->getActualNumberOfPassengers();



            //Distance from prev SP to new SP
            if((*it)->getIsPickup())
                dt = netmanager->getTimeDistance((*it)->getLocation(), newSP->getLocation()) + (boardingTime*(*it)->getNumberOfPassengers());
            else
                dt = netmanager->getTimeDistance((*it)->getLocation(), newSP->getLocation()) + alightingTime*abs((*it)->getNumberOfPassengers());

            EV << " Distance from " << (*it)->getLocation() << " to " <<  newSP->getLocation() << " is " << dt << endl;


            if(it2 != (std::prev(spl.end())))
            {
                it2++;

                // potrebbe essere sfavorevole ...

        /*
                if((*it)->getLocation() == (*it2)->getLocation()  && (!(*it2)->getIsPickup()) && pendingRequests[(*it2)->getRequestID()]->getTypeID() >= reqTypeID) //Two stop point with the same location.
                {
                    EV << "Two Stop point with location " << (*it)->getLocation() << endl;
                    continue;
                }
*/


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

            distanceTo = dt;
            distanceFrom = df;
            cost = c;
            it3 = it;
            std::list<StopPoint*> orderedList;
            StopPointOrderingProposal* proposal = new StopPointOrderingProposal();
            StopPoint* newSPcopy = new StopPoint(*newSP);


            /*Before new Stop point*/
            for (std::list<StopPoint*>::const_iterator it = spl.begin(), end = std::next(it3); it != end; ++it) {
                //      EV << " Before new SP pushing SP " << (*it)->getLocation() << ". Actual Time: " << (*it)->getActualTime() << ". PASSENGERS: " << (*it)->getActualNumberOfPassengers() << endl;
                orderedList.push_back(new StopPoint(**it));
            }



            bool okIns = false;
            StopPoint *newListBack = orderedList.back();
            StopPoint *newListSucc;
            int typeIDnewListSucc = -1;
            int typeIDnewListBack = pendingRequests[newListBack->getRequestID()]->getTypeID();
            EV<<"Prec Request : "<<typeIDnewListBack<<endl;
            if(orderedList.size() < spl.size())  {
                newListSucc = *std::next(it3);
                typeIDnewListSucc = pendingRequests[newListSucc->getRequestID()]->getTypeID();
                EV<<"Succ Request : "<<typeIDnewListSucc<<endl;

            }



            //gestiti i casi particolari del tipo 1122 in cui occorre inserire  22 almeno dopo 11
            //non può essere inserito prima  perchè l'ordinamento 1122 iniziale è conseguenza del numero insufficente di posti(11 sono due sp di drop-off
            //in uno scenario dove i posti sono solo 2)
         /*   if(typeIDnewListSucc > typeIDnewListBack ) {

                EV<<"New request of type "<<reqTypeID<<"can't pass req of type"<<typeIDnewListBack<<": num seats available "<<newListBack->getActualNumberOfPassengers()<<endl;

                if( reqTypeID > typeIDnewListSucc) {


                    if(newListBack->getActualTime() + dt > (newSPcopy->getTime()+newSPcopy->getMaxDelay()) && reqTypeID < maxT)
                                                        {
                                                            EV << "Stop search: from here will be unreachable within its deadline!" << endl;
                                                            break;
                                                        }

                    okIns = true;


                }
                else if(reqTypeID == typeIDnewListSucc) {


                     if(newListBack->getActualTime() + dt > (newSPcopy->getTime()+newSPcopy->getMaxDelay()) && reqTypeID < maxT )
                                       {
                                           EV << "Stop search: from here will be unreachable within its deadline!" << endl;
                                           break;
                                       }


                    minResidual = *(std::min_element(std::begin(residualTimes), std::end(residualTimes)));
                    EV << " Min residual from " << (*it)->getLocation() << " is " <<  minResidual << endl;

                    if(minResidual > c) {
                        okIns = true;
                        EV<<"Adding new proposal.."<<endl;
                    }



                }


            }*/



            if( (*it)->getActualTime() +dt > newSPcopy->getTime()+newSPcopy->getMaxDelay() && reqTypeID < maxT) {
                                     EV<<"TYPe REq: "<<reqTypeID<< "MAX T"<<maxT<<endl;
                                     EV<<"New SP will be unreachable within its deadline!"<<endl;
                                 break;
                                 }


            // gestiti i casi classici 2211, 1111, 2222 ecc
           // else {
                int pDiffPrec = reqTypeID - typeIDnewListBack;
                int pDiffSucc = 0;
                if(orderedList.size() < spl.size()) {
                    newListSucc = *std::next(it3);
                    //  if(!newListSucc == NULL)
                    //  pDiffSucc = pendingRequests[newSPcopy->getRequestID()]->getTypeID() - pendingRequests[newListSucc->getRequestID()]->getTypeID();
                    //  }
                    pDiffSucc = reqTypeID - typeIDnewListSucc;

                }



                if(/*(pDiffPrec == 0 || pDiffPrec <= -1) && */(pDiffSucc == 0 || pDiffSucc >= 1) ||  orderedList.size() == spl.size()) {






                    // Si effettua un inserimento in posizione non ultima solo se
                    //  presente un residuo temporale minimo sufficente.
                    if(residualTimes.size() >= 1)
                    {





                     /*   if(newListBack->getActualTime() + dt > (newSPcopy->getTime()+newSPcopy->getMaxDelay()) && reqTypeID < maxT )
                        {
                            EV << "Stop search: from here will be unreachable within its deadline!" << endl;
                            break;
                        }*/


                        EV<<"Residual Times size:"<<residualTimes.size()<<endl;
                        EV<<"Prec request-Actual proposal type diff:  "<<pDiffPrec<<endl;
                        EV<<"Succ request-Actual proposal type diff:  "<<pDiffSucc<<endl;



                        minResidual = *(std::min_element(std::begin(residualTimes), std::end(residualTimes)));
                        EV << " Min residual from " << (*it)->getLocation() << " is " <<  minResidual << "the cost is "<<c<<endl;


                        // per inserire un nuovo sp di pick-up va verificato che nell'eventualita si debba inserire successivamente
                        // il nuovo sp di drop-off perchè i posti a disposizione sono esauriti si deve calcolare il tempo in più che serve per portare al drop-off la persona e
                        // il tempo per permettere al veicolo di arrivare allo sp succesivo.
                      /*  if(newSPcopy->getIsPickup() && passengers + newSPcopy->getNumberOfPassengers() ==  vehicleSeats) {
                        double toDropOffDist = netmanager->getTimeDistance(newSPcopy->getLocation(), dropOffLocation) + boardingTime*newSPcopy->getNumberOfPassengers();
                        double distFromNewDropOffToSucc = netmanager->getTimeDistance(dropOffLocation, newListSucc->getLocation())+alightingTime*(newSPcopy->getNumberOfPassengers());
                        c += toDropOffDist+distFromNewDropOffToSucc;
                        }*/

                        if(minResidual > c ) {
                            okIns = true;
                            EV<<"Adding new proposal.."<<endl;
                        }



                        }




                    else  {



                         //   okIns = true;




                    //Si evita di effettuare altri  tentativi per creare nuove proposal  se possibile in quanto
                    // i successivi  sp fanno riferimento  a tipi richieste con priorita inferiore
                    if(((pDiffSucc>=1 ||  orderedList.size()== spl.size()) && pDiffPrec>=1 && mylist.size()>0 ) ) {
                       // okIns = false;

                        stopPositioning = true;
                    }
                    else {
                        if(orderedList.size() == spl.size())  {
                            EV<<"Adding new SP in last position.."<<endl;
                        }
                        else
                        EV<<"Adding new SP in last ideal postion.."<<" priority diff with prec "<<" "<<pDiffPrec<<" and priority diff with after "<<pDiffSucc<<endl;

                        okIns = true;

                    }



                    }


                }

           // }
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
                    //    EV << " After new SP pushing " << tmp->getLocation() << ". Previous actualTime: " << prevActualTime << ". Current actualTime: " <<tmp->getActualTime() << " max time: " << tmp->getTime() + tmp->getMaxDelay() << ". PASSENGERS: " << tmp->getActualNumberOfPassengers()<< endl;
                    orderedList.push_back(tmp);
                }

                proposal->setVehicleID(vehicleID);
                proposal->setAdditionalCost(cost);
                proposal->setActualPickupTime(newSPcopy->getActualTime());
                proposal->setSpList(orderedList);



                // se it corrente e di drop-off vuoldire che il costo della nuova proposta risulta inferiore inferiore alla precedente
                if(!(*it)->getIsPickup() && !mylist.empty())
                {
                    StopPointOrderingProposal* toDelete = mylist.front();
                    if(proposal->getAdditionalCost()<toDelete->getAdditionalCost()) {
                        mylist.pop_front();
                        delete toDelete;
                    }
                }
                mylist.push_back(proposal);




                if(stopPositioning)
                    break;

            }

            else {

                while(orderedList.size() > 0) {
                    StopPoint *toDel = orderedList.front();
                    orderedList.pop_front();
                    delete toDel;
                }

                if(stopPositioning)
                   break;

            }

        }
        else
            EV << "Too many passengers after SP in location " << (*it)->getLocation() << " and RequestID " << (*it)->getRequestID() << endl;

        if(constrainedPosition)
            break;
    }

    return mylist;
}
























/*

bool HeuristicCoord::isLastPickUpSP(int reqID, int reqTypeID, std::list<StopPoint *> spList) {

    int pos = -1;
    for(int i = 0; i<spList.size(); i++) {
        StopPoint * sp = spList.back();
        if(sp->getRequestID() == reqID && sp->getIsPickup()) {
            pos = i;
        }
        if(i > pos && sp->getIsPickup()) {
            if(pendingRequests[sp->getRequestID()]->getTypeID() != reqTypeID)


        }
     }
*/





//Get the additional time-cost allowed by each stop point

std::list<double> HeuristicCoord::getResidualTime(std::list<StopPoint*> spl, int requestID)
{
    std::list<double> residuals(0);



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





