#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <cmath>
#include "computerFan.h"


using namespace std;
using namespace RHLab::ComputerFan;

void ComputerFanSimulation::initialize(){
    this->targetDevice->initializeSimulation({"temperature"}, {"latch", "pulse", "rpm"});

    setReportWhenMarked(true);
}

// Performs simultaneous serial send and recieve operations for GPIO-based communication with a DUT (Device Under Test).
//
// Parameters:
// - input_buffer: A 2D boolean vector to be filled with received signal values from the DUT. 
//                 Each inner vector corresponds to one GPIO pin's signal over time.
//                 The outer index of the buffer matches the index of GPIO names in `input_gpios`.
//
// - input_gpios: A list of GPIO names representing the input signals. Each name's position in the list
//                corresponds to the respective index in the `input_buffer`.
//                There must be a "latch" and a "pulse" GPIO.
//
// - output_buffer: A 2D boolean vector containing signal values to be sent to the DUT.
//                  Each inner vector represents one GPIO pin's signal over time.
//                  The outer index of the buffer matches the index of GPIO names in `output_gpios`.
//
// - output_gpios: A list of GPIO names representing the output signals. Each name's position in the list
//                 corresponds to the respective index in the `output_buffer`.
//
// Returns:
// - A boolean indicating whether the communication was successful.
bool ComputerFanSimulation::performSerialCommunication(
    vector<vector<bool>>& input_buffer, 
    vector<string>& input_gpios,
    vector<vector<bool>>& output_buffer,
    vector<string>& output_gpios
) {
    try {
        int num_pulses = 0;
        for (int i = 0; i < input_buffer.size(); i++) {
            if (input_buffer[i].size() > num_pulses) num_pulses = input_buffer[i].size();
        }
        for (int i = 0; i < output_buffer.size(); i++) {
            if (output_buffer[i].size() > num_pulses) num_pulses = output_buffer[i].size();
        }
        for (int pulse = num_pulses-1; pulse >= 0; pulse--) {
            // Wait for a rising edge on the pulse GPIO
            while (this->targetDevice->getGpio("pulse") == 0) {}

            this->log("begining pulse");

            // Read the data bit when the pulse is high
            for (int input_pin = 0; input_pin < input_buffer.size(); input_pin++) {
                if (pulse < input_buffer[pulse].size()) {
                    input_buffer[input_pin][pulse] = this->targetDevice->getGpio(input_gpios[input_pin]);
                }
            }
            this->log("input done");

            // Write the data bit when the pulse is high
            for (int output_pin = 0; output_pin < output_buffer.size(); output_pin++) {
                if (pulse < output_buffer[pulse].size()) {
                    this->targetDevice->setGpio(output_gpios[output_pin], output_buffer[output_pin][pulse]);
                } else {
                    this->targetDevice->setGpio(output_gpios[output_pin], false);
                }
            }
                
            // Wait for the pulse to go low again before reading the next bit
            while (this->targetDevice->getGpio("pulse") == 1) {}
        }
        printInput(input_buffer, input_gpios);
        printOutput(output_buffer, output_gpios);
    } catch (exception e) {
        this->log("exception!");
        return false;
    }
    return true;
}

void ComputerFanSimulation::printInput(vector<vector<bool>>& input_buffer, vector<string>& input_gpios) {
    for (int i = 0; i < input_buffer.size(); i++) {
        string row_result = "";
        for (int j = input_buffer[i].size()-1; j >= 0 ; j--) {
            row_result += input_buffer[i][j] ? "1" : "0";
        }
        this->log() << input_gpios[i] << " <-- " << row_result << endl;
    }
}

void ComputerFanSimulation::printOutput(vector<vector<bool>>& output_buffer, vector<string>& output_gpios) {
    for (int i = 0; i < output_buffer.size(); i++) {
        string row_result = "";
        for (int j = output_buffer[i].size()-1; j >= 0 ; j--) {
            row_result += output_buffer[i][j] ? "1" : "0";
        }
        this->log() << output_gpios[i] << " --> " << row_result << endl;
    }
}

void ComputerFanSimulation::update(double delta) {

    ComputerFanRequest userRequest;
    bool requestWasRead = readRequest(userRequest);
    if(requestWasRead) {
        this->mState.updateUsage(userRequest.state);
        this->log() << "Updating state to: " << userRequest.state << " U:" << this->mState.usage << " T: " << this->mState.temperature << " RPM:" << this->mState.rpm << endl;
    } else {
        this->mState.updateUsage(this->mState.state);
    }

    double C = this->mState.rpm / MAX_RPM;
    double P = this->mState.usage / MAX_USAGE * MAX_WATTS;
    double dTdt = (P - C * (this->mState.temperature - T_AMBIENT)) / C_TH;
    double T = this->mState.temperature + (dTdt * dT);
    this->mState.setTemperature(T);

    // If latch is low, there's nothing to process.
    if (this->targetDevice->getGpio("latch") == 0) {
        requestReportState();
        return;
    }
    
    // Input GPIO data and channel names
    vector<vector<bool>> targetDeviceInputData(1, vector<bool>(BITS_PER_RPM, false));
    vector<string> inputGPIOs = {"rpm"};
    
    // Output GPIO data and channel names
    vector<vector<bool>> targetDeviceOutputData(1, vector<bool>(BITS_PER_TEMP, false));
    int temp_int = static_cast<int>(round(this->mState.temperature));
    for (int i = BITS_PER_TEMP; i >= 0; i--) {
        targetDeviceOutputData[0][i] = (temp_int >> (BITS_PER_TEMP - 1 - i)) & 1;
    }
    vector<string> outputGPIOs = {"temperature"};

    // Wait for latch to become 0
    while (this->targetDevice->getGpio("latch") == 1) {}

    // Process the received data and update
    if (performSerialCommunication(targetDeviceInputData, inputGPIOs, targetDeviceOutputData, outputGPIOs)) {
        // Fan speed update
        int rpm = 0;
        for (int i = BITS_PER_RPM; i >= 0; i--) {
            rpm += targetDeviceInputData[0][i] ? pow(2, i) : 0;
        }
        this->log() << "rpm array size: " << targetDeviceInputData[0].size() << endl;
        this->log() << " " << rpm << endl;
        if (rpm < 600) rpm = 600;
        if (rpm > 3000) rpm = 3000;
        
        this->mState.setRPM(rpm);
        this->log() << "TEMP: " << this->mState.temperature << " RPM: " << this->mState.rpm << endl;
        this->log() << "Reporting:" << this->mState.serialize() << endl;
        requestReportState();
    }
}
