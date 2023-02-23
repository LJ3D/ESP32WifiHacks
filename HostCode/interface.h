#include "networksList.h"
#include "beaconSniffer.h"
#include "beaconSpammer.h"
#include "deauther.h"
#include "deauthDetector.h"

class ESP32Interface{
private:
    std::string port;
    int baudrate;
    networksList networks;
    beaconSniffer sniffer;
    beaconSpammer spammer;
    deauther deauth;
    deauthDetector deauthDetect;

public:
    ESP32Interface(std::string port="/dev/ttyUSB0", int baudrate=B115200){
        this->port = port;
        this->baudrate = baudrate;
        this->sniffer.config(&this->networks, port, baudrate);
        this->spammer.config(port, baudrate);
        this->deauth.config(&this->networks, port, baudrate);
        this->deauthDetect.config(port, baudrate);
    }
    
    ~ESP32Interface(){
    }

    void update(){
        this->networks.draw(0, 0, 1, 0.5, ImGuiCond_Always);
        this->sniffer.draw(0, 0.51, 0.25, 1, ImGuiCond_Always);
        this->spammer.draw(0.26, 0.51, 0.5, 1, ImGuiCond_Always);
        this->deauth.draw(0.51, 0.51, 0.75, 1, ImGuiCond_Always);
        this->deauthDetect.draw(0.76, 0.51, 1, 1, ImGuiCond_Always);
    }

};