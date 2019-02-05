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

#ifndef VEHICLE_H_
#define VEHICLE_H_

#include <Packet_m.h>

class Vehicle: public Packet {
    protected:
        static int nextID;
        int id;
        int seats;
        int traveledDistance;
        int typeId;
        std::string type;
        int state;


    public:
    //    enum class Types{EMERGENCY_0=0, EMERGENCY_1=1, EMERGENCY_2=2};
        Vehicle();
        virtual ~Vehicle();
        virtual int getID() const;
        virtual double getTraveledDistance() const;
        virtual void setTraveledDistance(double distance);
        virtual int getSeats() const;
        virtual void setSeats(int seats);
        virtual void setState(int state);
        virtual int getState() const;
        virtual void setType(std::string type);
        virtual void setTypeId(int typeId);
        virtual std::string getType() const;
        virtual int getTypeId() const;

};

#endif /* VEHICLE_H_ */
