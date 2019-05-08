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

#include <omnetpp.h>
#include<fstream>
#include<iostream>
#include "TripRequest.h"
#include "AbstractNetworkManager.h"

class TripRequestSubmitter : public cSimpleModule
{
    private:
        // configuration
        int myAddress;
        int x_coord;
        int y_coord;

        double maxSubmissionTime;
        double minTripLength;
        int destAddresses;

        cPar *sendIATime;
        cPar *maxDelay;
        AbstractNetworkManager *netmanager;

        cMessage *generatePacket;
        long pkCounter;

        // signals
        simsignal_t tripRequest;

        int req_count;

      public:
        TripRequestSubmitter();
        virtual ~TripRequestSubmitter();

      protected:
        virtual void initialize();
        virtual void handleMessage(cMessage *msg);
        virtual TripRequest* buildTripRequest();
        virtual std::map<int, std::string> readAllRequestTypes();
};

Define_Module(TripRequestSubmitter);

TripRequestSubmitter::TripRequestSubmitter()
{
    generatePacket = NULL;
    netmanager = NULL;
}

TripRequestSubmitter::~TripRequestSubmitter()
{
    cancelAndDelete(generatePacket);
}

void TripRequestSubmitter::initialize()
{
    bool reqGen = getParentModule()->par("isRequestGenerator").boolValue();


    myAddress = par("address");
    destAddresses = par("destAddresses");
    minTripLength = par("minTripLength");
    sendIATime = &par("sendIaTime");  // volatile parameter
    maxDelay = &par("maxDelay");
    maxSubmissionTime = par("maxSubmissionTime");

    x_coord = getParentModule()->par("x");
    y_coord = getParentModule()->par("y");

    double rows = getParentModule()->getParentModule()->par("width");
    double columns = getParentModule()->getParentModule()->par("height");
    req_count = 0;



    netmanager = check_and_cast<AbstractNetworkManager *>(getParentModule()->getParentModule()->getSubmodule("netmanager"));
//    if(!netmanager->isValidDestinationAddress(myAddress)) {
    generatePacket = new cMessage("nextPacket");
    tripRequest = registerSignal("tripRequest");

    if(reqGen) {
         EV<<"TripRequest not generated for node x,y"<<getParentModule()->par("x").longValue()<<" "<<getParentModule()->par("y").longValue()<<endl;
         if (maxSubmissionTime < 0 || sendIATime->doubleValue() < maxSubmissionTime)
                scheduleAt(sendIATime->doubleValue(), generatePacket);
     }

  //  }


}


void TripRequestSubmitter::handleMessage(cMessage *msg)
{
   // bool reqGen = getParentModule()->par("isRequestGenerator").boolValue();
    //EMIT a TRIP REQUEST
   // if(reqGen) {
    if (msg == generatePacket)
    {
        TripRequest *tr = nullptr;

     //   if(simTime().dbl() > 7920 )
       //     return;

/*
        if(simTime().dbl() > 38000 ){
                                   EV<<"Pk stopped!"<<endl;
                                   return;
                  }*/


        tr = buildTripRequest();
         std::string s("REQUEST-"+tr->getType());
        if (ev.isGUI()) getParentModule()->bubble(s.c_str());



        EV << "Requiring a new trip of type"<<tr->getType()<< " from/to: " << tr->getPickupSP()->getLocation() << "/" << tr->getDropoffSP()->getLocation() << ". I am node: " << myAddress << endl;
        EV << "Requested pickupTime: " << tr->getPickupSP()->getTime() << ". DropOFF required time: " << tr->getDropoffSP()->getTime() << ". Passengers: " << tr->getPickupSP()->getNumberOfPassengers() << endl;

        emit(tripRequest, tr);

        //Schedule the next request
        simtime_t nextTime = simTime() + sendIATime->doubleValue();
        if (maxSubmissionTime < 0 || nextTime.dbl() < maxSubmissionTime)
        {
            EV << "Next request from node " << myAddress << "scheduled at: " << nextTime.dbl() << endl;
            scheduleAt(nextTime, generatePacket);
        }
    }
//}
}


/**
 * Build a new Trip Request
 */
TripRequest* TripRequestSubmitter::buildTripRequest()
{
    TripRequest *request = new TripRequest();
    double simtime = simTime().dbl();





    // generate a  dinamic request type
    std::map<int, std::string> reqTypes = readAllRequestTypes();
    std::map<int, std::string>::iterator it = reqTypes.begin();
    int firstId = it->first;
    it = reqTypes.end();
    it--;
    int lastId = it->first;
   /* int requestId = intuniform(firstId, lastId, 3);      // test type 1 end type 2
    request->setTypeID(requestId);
   request->setType(reqTypes[requestId]);*/

   if(req_count >=0 && req_count<3) {
       request->setTypeID(lastId);
       request->setType(reqTypes[lastId]);
       req_count++;
   }
   else {
       int requestId = intuniform(firstId, lastId-1, 3);      // test type 1 end type 2
           request->setTypeID(requestId);
          request->setType(reqTypes[requestId]);

       req_count=0;

   }

   // request->setTypeID(2);
  //  request->setType(reqTypes[2]);



   // TEST FOR COORDINATOR
 /*  if(myAddress == 15 ) {
        request->setTypeID(2);
        request->setType(reqTypes[2]);
   }
   else {
       request->setTypeID(1);
       request->setType(reqTypes[1]);
   }*/



   // int destAddress =  netmanager->getValidDestinationAddress(request->getTypeID());
    int destAddress =  netmanager->getCloserValidDestinationAddress(myAddress, request->getTypeID());


    StopPoint *pickupSP = new StopPoint(request->getID(), myAddress, true, simtime, maxDelay->doubleValue());
    pickupSP->setXcoord(x_coord);
    pickupSP->setYcoord(y_coord);
    pickupSP->setNumberOfPassengers(par("passengersPerRequest"));

    StopPoint *dropoffSP = new StopPoint(request->getID(), destAddress, false, simtime + netmanager->getTimeDistance(myAddress, destAddress), maxDelay->doubleValue());

    request->setPickupSP(pickupSP);
    request->setDropoffSP(dropoffSP);

    return request;
}









std::map<int, std::string>  TripRequestSubmitter::readAllRequestTypes() {

    std::map<int, std::string> requestTypes;
    std::string line;
  //  std::fstream nodeTypesFile("C:\\omnetpp-4.6\\newprojects\\CopyofAMoD_Simulator\\src\\networkmanager\\nodeTypes.txt", std::ios::in);
    std::fstream requestTypesFile(par("requestTypesFile").stringValue(), std::ios::in);
       while(getline(requestTypesFile, line, '\n'))
       {
           if (line.empty() || line[0] == '#')
               continue;
           std::vector<std::string> tokens = cStringTokenizer(line.c_str()).asVector();
           if (tokens.size() != 2)
               throw cRuntimeError("wrong line in module file: 2 items required, line: \"%s\"", line.c_str());

           // get fields from tokens
           int requestTypeId = atoi(tokens[0].c_str());
           const char *requestType = tokens[1].c_str();

           requestTypes.insert(std::pair<int, std::string>(requestTypeId, requestType));


}
       return requestTypes;
}





