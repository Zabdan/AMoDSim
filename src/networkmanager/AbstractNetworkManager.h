//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef ABSTRACTNETWORKMANAGER_H_
#define ABSTRACTNETWORKMANAGER_H_


#include<fstream>
#include<iostream>


class AbstractNetworkManager : public cSimpleModule{

protected:
    std::map<int,std::list<int>> vehiclesPerNode;  //Number of vehicles per node at simulation start
    int numberOfVehicles;               //Number of vehicles in the network
    int numberOfNodes;                  //Number of crossroads(nodes) in the network
    double additionalTravelTime;        //Additional Travel Time due to acceleration and deceleration
   // BaseCoord *tcoord;
    simsignal_t updateSchedulingS;

    virtual void initialize() = 0;
    virtual void handleMessage(cMessage *msg) = 0;
    virtual void setDropOffNodes() = 0;
//    virtual void readAllNodeTypes(std::map<int, std::string> *nodeTypes)0;
  //  virtual void readAlldestNodesRequestsMatching(std::map<int, int> *nRMatch);
    virtual std::vector<cModule *>  getAllDestinationNodes(int nodeTypeId) = 0;


  public:
    virtual double getTimeDistance(int srcAddr, int dstAddr)=0;      //Get the time needed to go from srcAddr to dstAddr
    virtual double getSpaceDistance(int srcAddr, int dstAddr)=0;     //Get the space-distance from srcAddr to dstAddr
    virtual double getChannelLength(int nodeAddr, int gateIndex)=0;  //Get the length of the channel connected to the specified gate
    virtual int getOutputGate(int srcAddr, int destAddr)=0;          //Get the index of the gate where send the packet to reach the destAddr
    virtual std::list<int> getVehiclesPerNode(int nodeAddr)=0;                  //Get the number of vehicles located in the node at simulation start
    virtual bool isValidAddress(int nodeAddr)=0;                     //Check if the specified address is valid
    virtual bool isValidDestinationAddress(int destAddr)=0;
    virtual bool isValidDestinationAddress(int requestTypeId,int destAddr)=0;              //Check if the specified destination address is valid
  //  virtual int getValidDestinationAddress(int requestTypeId)=0;    // Get a random valid destination node address
    virtual int getCloserValidDestinationAddress(int srcAddress, int requestTypeId)=0;
    virtual cModule*getNodeFromCoords(int x, int y)=0;
    virtual std::vector<std::pair<int,int>> *getCenteredSquare(int mult)=0;

    inline int getNumberOfVehicles(){return numberOfVehicles;}       //Get the fleet size
    inline double getAdditionalTravelTime(){return additionalTravelTime;} //Get the additional travel time due to acceleration and deceleration



   // virtual int getMaxRisk();


    // velocita non sepre uguale quindi va calcolata prima la velocita media
    double setAdditionalTravelTime(double speed, double acceleration) //Evaluate Additional Travel Time due to acceleration and deceleration
    {
        if(acceleration<=0) {additionalTravelTime=0; return 0;}
        else{
            double Ta=speed/acceleration;
            double D = 0.5*acceleration*pow(Ta, 2);
            double Ta_prime = D/speed;

            additionalTravelTime = 2*(Ta - Ta_prime);
            return additionalTravelTime;
        }
    }





    void  readAllNodeTypes(std::map<int, std::string> *nodeTypes, const char * file) {


        std::string line;
        std::fstream f(file, std::ios::in);
           while(getline(f, line, '\n'))
           {
               if (line.empty() || line[0] == '#')
                   continue;

               std::vector<std::string> tokens = cStringTokenizer(line.c_str()).asVector();
               if (tokens.size() != 2)
                   throw cRuntimeError("wrong line in module file: 2 items required, line: \"%s\"", line.c_str());

               // get fields from tokens
               int nodeTypeId = atoi(tokens[0].c_str());
               const char *nodeTypeName = tokens[1].c_str();
              // EV<<"Node type id"<<nodeTypeid<<" name"<<nodeTypeName<<endl;
               nodeTypes->insert(std::pair<int, std::string>(nodeTypeId, nodeTypeName));


    }

    }

    void  readAlldestNodesRequestsMatching(std::map<int, int> *nRMatch, const char * file) {


        std::string line;
        std::fstream f(file, std::ios::in);
           while(getline(f, line, '\n'))
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
               int nodeTypeId = atoi(tokens[1].c_str());
             //  EV<<"Node type id"<<nodeTypeId<<" requestTypeId"<<requestTypeId<<endl;
               nRMatch->insert(std::pair<int, int>(requestTypeId, nodeTypeId));
    }




    }


    void  readAllRiskLevels(std::map<int, std::string> *rLevels, const char * file) {


           std::string line;
         //  std::fstream nodeTypesFile("C:\\omnetpp-4.6\\newprojects\\CopyofAMoD_Simulator\\src\\networkmanager\\nodeTypes.txt", std::ios::in);
           std::fstream f(file, std::ios::in);
         //  EV<<"File opened"<<nodeTypesFile.is_open()<<endl;
              while(getline(f, line, '\n'))
              {
                 // EV<<"Line "<<endl;
                  if (line.empty() || line[0] == '#')
                      continue;
                //  EV<<"Line "<< line<<endl;
                  std::vector<std::string> tokens = cStringTokenizer(line.c_str()).asVector();
                  if (tokens.size() != 2)
                      throw cRuntimeError("wrong line in module file: 2 items required, line: \"%s\"", line.c_str());

                  // get fields from tokens
                  int riskLevel = atoi(tokens[0].c_str());
                  std::string displayString =  tokens[1].c_str();
                 // EV<<"Node type id"<<nodeTypeId<<" name"<<nodeTypeName<<endl;
                  rLevels->insert(std::pair<int, std::string>(riskLevel, displayString));
       }




       }



    void  readAllRiskDelayFactorMatching(std::map<int, double> *rDMatch, const char * file) {


           std::string line;
         //  std::fstream nodeTypesFile("C:\\omnetpp-4.6\\newprojects\\CopyofAMoD_Simulator\\src\\networkmanager\\nodeTypes.txt", std::ios::in);
           std::fstream f(file, std::ios::in);
         //  EV<<"File opened"<<nodeTypesFile.is_open()<<endl;
              while(getline(f, line, '\n'))
              {
                 // EV<<"Line "<<endl;
                  if (line.empty() || line[0] == '#')
                      continue;
                //  EV<<"Line "<< line<<endl;
                  std::vector<std::string> tokens = cStringTokenizer(line.c_str()).asVector();
                  if (tokens.size() != 2)
                      throw cRuntimeError("wrong line in module file: 2 items required, line: \"%s\"", line.c_str());

                  // get fields from tokens
                  int riskLevel = atoi(tokens[0].c_str());
                  double delayIncrFactor =  atof(tokens[1].c_str());
                 // EV<<"Node type id"<<nodeTypeId<<" name"<<nodeTypeName<<endl;
                  rDMatch->insert(std::pair<int, double>(riskLevel, delayIncrFactor));
       }




       }







};

#endif /* ABSTRACTNETWORKMANAGER_H_ */
