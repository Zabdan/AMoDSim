package src.coordinator;


simple BaseCoord like ICoord
{
    parameters:
        double minTripLength @unit(m) = default(1000m);
        int requestAssignmentStrategy = default(0); //Allowed are: 0(MinCost) - 1(MinWaitingTime)
      
        int minListSizeForPriorityUpdate = default(10); //Min SP numbers 
        double updateRequestPriorityTime @unit(s) = default(10000s);
        string requestVehicleMatchingFile = "../src/common/requestVehicleMatching.txt";
        string requestTypesFile = "../src/common/requestTypes.txt";
        
        bool requestsReallocation = default(false); 
        int maxReallocForRedCodes = default(1);
        
        @display("i=block/cogwheel_s");
        
        // ------ SIGNALS -------- //
        @signal[traveledDistance](type="double");       	//Traveled distance per vehicle
        @statistic[traveledDistance](title="Traveled distance per vehicle (km)";source="traveledDistance";unit=km;record=vector,stats;interpolationmode=none);

        @signal[waitingTime](type="double");				//Pick-up waiting time per request
        @statistic[waitingTime](title="Waiting time (minute)";source="waitingTime";unit=minute;record=vector,stats;interpolationmode=none);

        @signal[waitingTimeForYellowCodes](type="double");				//Pick-up waiting time per request
        @statistic[waitingTimeForYellowCodes](title="Waiting time For Yellow Codes (minute)";source="waitingTimeForYellowCodes";unit=minute;record=vector,stats;interpolationmode=none);
       
        @signal[waitingTimeForRedCodes](type="double");				//Pick-up waiting time per request
        @statistic[waitingTimeForRedCodes](title="Waiting time For Red Codes (minute)";source="waitingTimeForRedCodes";unit=minute;record=vector,stats;interpolationmode=none);
        
        

        @signal[actualTripTime](type="double");				//Actual time scored on vehicle (per request)
        @statistic[actualTripTime](title="Actual Trip Time (minute)";source="actualTripTime";unit=minute;record=vector,stats;interpolationmode=none);

       
        @signal[actualTripTimeForYellowCodes](type="double");				//Actual time scored on vehicle (per request)
        @statistic[actualTripTimeForYellowCodes](title="Actual Trip Time For Yellow Codes (minute)";source="actualTripTimeForYellowCodes";unit=minute;record=vector,stats;interpolationmode=none);
       
         
        @signal[actualTripTimeForRedCodes](type="double");				//Actual time scored on vehicle (per request)
        @statistic[actualTripTimeForRedCodes](title="Actual Trip Time For Red Codes (minute)";source="actualTripTimeForRedCodes";unit=minute;record=vector,stats;interpolationmode=none); 
       
         
        @signal[normalizedTripTimeForYellowCodes](type="double");				//Actual time scored on vehicle (per request)
        @statistic[normalizedTripTimeForYellowCodes](title="Normalized Trip Time For Yellow Codes (minute)";source="normalizedTripTimeForYellowCodes";unit=minute;record=vector,stats;interpolationmode=none);
       
         
        @signal[normalizedTripTimeForRedCodes](type="double");				//Actual time scored on vehicle (per request)
        @statistic[normalizedTripTimeForRedCodes](title="Normalized Trip Time For Red Codes (minute)";source="normalizedTripTimeForRedCodes";unit=minute;record=vector,stats;interpolationmode=none);  
         
         
         
         
        @signal[outOfTimeForYellowCodes](type="double");				//Actual time scored on vehicle (per request)
        @statistic[outOfTimeForYellowCodes](title="Out of  Time For Yellow Codes (minute)";source="outOfTimeForYellowCodes";unit=minute;record=vector,stats;interpolationmode=none);  
         
        @signal[outOfTimeForRedCodes](type="double");				//Actual time scored on vehicle (per request)
        @statistic[outOfTimeForRedCodes](title="Out of  Time For Red Codes (minute)";source="outOfTimeForRedCodes";unit=minute;record=vector,stats;interpolationmode=none);   
         
         
        @signal[outOfTimeForGreenCodes](type="double");				//Actual time scored on vehicle (per request)
        @statistic[outOfTimeForGreenCodes](title="Out of  Time For Green Codes (minute)";source="outOfTimeForGreenCodes";unit=minute;record=vector,stats;interpolationmode=none);    
         
       
        @signal[stretch](type="double");
        @statistic[stretch](title="Stretch";source="stretch";record=vector,stats;interpolationmode=none);

        @signal[tripDistance](type="double");				//Distance per trip
        @statistic[tripDistance](title="Trip Distance (km)";source="tripDistance";unit=km;record=vector,stats;interpolationmode=none);

        @signal[passengersOnBoard](type="double");			//Passengers on board per vehicle at snapshot time
        @statistic[passengersOnBoard](title="Passengers On Board";source="passengersOnBoard";record=vector,stats;interpolationmode=none);

        @signal[toDropoffRequests](type="double");       	//Picked-up requests not yet dropped-off (at snapshot time)
        @statistic[toDropoffRequests](title="Picked-up Requests";source="toDropoffRequests";record=vector,stats;interpolationmode=none);

        @signal[toPickupRequests](type="double");       	//Requests assigned but not yet picked-up
        @statistic[toPickupRequests](title="Requests To Pickup";source="toPickupRequests";record=vector,stats;interpolationmode=none);

        @signal[requestsAssignedPerVehicle](type="double");  //Number of requests assigned per vehicle
        @statistic[requestsAssignedPerVehicle](title="Requests assigned per vehicle";source="requestsAssignedPerVehicle";record=vector,stats;interpolationmode=none);
        
        @signal[travel*-time](type="double"); 
		@statisticTemplate[travelTime](title="Travel Time Per State (minute)";unit=minute;record=vector,stats;interpolationmode=none);
		
		@signal[totalRequestsPerTime](type="double");
		@statistic[totalRequestsPerTime](title="Total Requests Per Time";source="totalRequestsPerTime";record=vector;interpolationmode=none);
		
		@signal[totalRedCodesPerTime](type="double");
		@statistic[totalRedCodesPerTime](title="Total RedCodes Per Time";source="totalRedCodesPerTime";record=vector;interpolationmode=none);
		
		@signal[totalYellowCodesPerTime](type="double");
		@statistic[totalYellowCodesPerTime](title="Total YellowCodes Per Time";source="totalYellowCodesPerTime";record=vector;interpolationmode=none);
		
		
		@signal[assignedRequestsPerTime](type="double");
		@statistic[assignedRequestsPerTime](title="Assigned Requests Per Time";source="assignedRequestsPerTime";record=vector;interpolationmode=none);
		       
        @signal[pickedupRequestsPerTime](type="double");
		@statistic[pickedupRequestsPerTime](title="Picked-up Requests Per Time";source="pickedupRequestsPerTime";record=vector;interpolationmode=none);
        
        @signal[pickedupRedCodesPerTime](type="double");
		@statistic[pickedupRedCodesPerTime](title="Picked-up RedCodes Per Time";source="pickedupRedCodesPerTime";record=vector;interpolationmode=none);
    
        @signal[pickedupYellowCodesPerTime](type="double");
		@statistic[pickedupYellowCodesPerTime](title="Picked-up YellowCodes Per Time";source="pickedupYellowCodesPerTime";record=vector;interpolationmode=none);
        
        @signal[droppedoffRequestsPerTime](type="double");
		@statistic[droppedoffRequestsPerTime](title="Dropped-off Requests Per Time";source="droppedoffRequestsPerTime";record=vector;interpolationmode=none);
		
		@signal[droppedoffRedCodesPerTime](type="double");
		@statistic[droppedoffRequestsPerTime](title="Dropped-off RedCodes Per Time";source="droppedoffRedCodesPerTime";record=vector;interpolationmode=none);
		
		@signal[droppedoffYellowCodesPerTime](type="double");
		@statistic[droppedoffYellowPerTime](title="Dropped-off YellowCodes Per Time";source="droppedoffYellowCodesPerTime";record=vector;interpolationmode=none);
		
		
		@signal[freeVehiclesPerTime](type="double");
		@statistic[freeVehiclesPerTime](title="Free Vehicles Per Time";source="freeVehiclesPerTime";record=vector;interpolationmode=none);
}