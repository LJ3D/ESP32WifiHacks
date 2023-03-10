#pragma once

#include <thread>

#include <imgui.h>

#include "ArduinoSerialIO/arduinoSerial.hpp"
#include "uiHelper.h"

#define sendCmdDelayMS 8

class beaconSpammer{
private:
    std::string port;
    int baudrate;
    arduinoSerial Serial;

    bool manualAPs = false;
    char SSID[33] = {0};
    char multiSSID[1024] = {0};
    int APCount = 10;
    
    std::thread spammerThread;
    std::mutex* espMutex;

    void spammer(){
        std::vector<char> ssidCmd;
        ssidCmd.push_back('b');
        ssidCmd.push_back(0x03);
        for(int i=0; i<this->APCount; i++){
            std::string ssid = std::string(this->SSID) + std::to_string(i);
            for(char c : ssid){
                ssidCmd.push_back(c);
            }
            ssidCmd.push_back(0x03);
        }
        ssidCmd.back() = '\n'; // Replace last 0x03 with '\n'

        for(auto c : ssidCmd){
            this->Serial.write_s(c);
            // Sleep for a little bit to allow the ESP to process the command
            std::this_thread::sleep_for(std::chrono::milliseconds(sendCmdDelayMS));
        }


        // Wait until this->spammerRunning is set to false
        while(this->spammerRunning){
            // Print anything the ESP sends (for debugging purposes)
            int c = this->Serial.read_s();
            if(c != -1 && (c >= 32 && c <= 126 || c == '\n')){
                std::cout << (char)c;
            }
        }
        // Send 'q' to tell the ESP to stop sending out beacon frames
        this->Serial.write_s('q');
    }

    void spammerMulti(std::vector<std::string> ssids){
        std::vector<char> ssidCmd;
        ssidCmd.push_back('b');
        ssidCmd.push_back(0x03);
        for(std::string ssid : ssids){
            for(char c : ssid){
                ssidCmd.push_back(c);
            }
            ssidCmd.push_back(0x03);
        }
        ssidCmd.back() = '\n'; // Replace last 0x03 with '\n'

        for(auto c : ssidCmd){
            this->Serial.write_s(c);
            // Sleep for a little bit to allow the ESP to process the command
            std::this_thread::sleep_for(std::chrono::milliseconds(sendCmdDelayMS));
        }

        // Wait until this->spammerRunning is set to false
        while(this->spammerRunning){
            // Print anything the ESP sends (for debugging purposes)
            int c = this->Serial.read_s();
            if(c != -1 && (c >= 32 && c <= 126 || c == '\n')){
                std::cout << (char)c;
            }
        }
        // Send 'q' to tell the ESP to stop sending out beacon frames
        this->Serial.write_s('q');
    }

public:
    bool spammerRunning = false;

    ~beaconSpammer(){
        if(this->spammerRunning){
            this->stopSpammer();
        }
    }

    void config(std::string port, int baudrate, std::mutex* m){
        this->port = port;
        this->baudrate = baudrate;
        this->espMutex = m;
    }

    /*
        Draws the menu for configuring and starting beacon spam
    */
    void draw(float wStartXNorm,
              float wStartYNorm,
              float wEndXNorm,
              float wEndYNorm,
              ImGuiCond wCondition=ImGuiCond_Always){
        uiHelper::setNextWindowSizeNormalised(wStartXNorm,
                                              wStartYNorm,
                                              wEndXNorm,
                                              wEndYNorm,
                                              wCondition);
        ImGui::Begin("Beacon spammer");
        std::string isRunning = "Spammer is ";
        isRunning += this->spammerRunning ? "running" : "not running";
        if(this->spammerRunning){
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
        }
        ImGui::Text(isRunning.c_str());
        if(this->spammerRunning){
            ImGui::PopStyleColor();
        }
        ImGui::Dummy(ImVec2(0, 2));
        ImGui::Checkbox("Manually specify all AP names", &this->manualAPs);

        if(!this->manualAPs){
            ImGui::Text("SSID to use for APs");
            ImGui::PushItemWidth(-1);
            ImGui::InputText("##SSID", this->SSID, 33);
            ImGui::Dummy(ImVec2(0, 1));
            ImGui::Text("Number of APs to create");
            ImGui::InputInt("##APCount", &this->APCount, 0, 0, 0);
        }else{
            ImGui::TextWrapped("Enter SSIDs, separated by \";\" (e.g. \"AP1;AP2;AP3\")");
            ImGui::PushItemWidth(-1);
            ImGui::InputText("##SSID", this->multiSSID, 1024);
        }
        ImGui::PopItemWidth();
        ImGui::Dummy(ImVec2(0, 2));

        if(this->spammerRunning){
            if(ImGui::Button("Stop beacon spam")){
                this->stopSpammer();
            }
        }else{
            if(ImGui::Button("Start beacon spam")){
                if(this->manualAPs){
                    this->parseMultiSSIDsStartSpammer();
                }else{
                    this->startSpammer();
                }
            }
        }


        ImGui::End();
    }

    void parseMultiSSIDsStartSpammer(){
        std::vector<std::string> ssids;
        std::stringstream ss(this->multiSSID);
        std::string ssid;
        while(std::getline(ss, ssid, ';')){
            ssids.push_back(ssid);
        }
        if(ssids.size() == 0){
            return;
        }

        if(!this->espMutex->try_lock()){
            return;
        }
        this->Serial.openPort(this->port);
        this->Serial.begin(this->baudrate);
        this->spammerRunning = true;
        this->spammerThread = std::thread(&beaconSpammer::spammerMulti, this, ssids);
    }

    void startSpammer(){
        if(!this->espMutex->try_lock()){
            return;
        }
        this->Serial.openPort(this->port);
        this->Serial.begin(this->baudrate);
        this->spammerRunning = true;
        this->spammerThread = std::thread(&beaconSpammer::spammer, this);
    }

    void stopSpammer(){
        this->spammerRunning = false;
        spammerThread.join();
        this->espMutex->unlock();
    }

};