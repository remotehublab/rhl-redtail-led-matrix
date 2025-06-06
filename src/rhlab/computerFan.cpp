#include <iostream>
#include <stdio.h>
#include <string.h>
#include <array>
#include <vector>
#include <cmath>
#include <chrono>
#include "computerFan.h"


using namespace std;
using namespace RHLab::ComputerFan;

void ComputerFanSimulation::initialize(){
    this->communicator = make_shared<LabsLand::Simulations::Utils::DefaultSingleSerialCommunicator>(
        this->targetDevice, 
        "rpm", // inputs
        "temperature" // outputs
    );

    this->targetDevice->initializeSimulation({"temperature"}, {"latch", "pulse", "rpm"});

    setReportWhenMarked(true);
}

void ComputerFanSimulation::update(double delta) {

    ComputerFanRequest userRequest;
    bool requestWasRead = readRequest(userRequest);
    if(requestWasRead) {
        if (userRequest.reset) {
            this->mState.reset();
            requestReportState();
            return;
        }
        this->mState.updateUsage(userRequest.state);
    } else {
        this->mState.updateUsage(this->mState.state);
    }
    this->log() << "Updating CPU: S: " << this->mState.state << " U:" << this->mState.usage << " T: " << this->mState.temperature << " RPM:" << this->mState.rpm << endl;

    double C = C_PASSIVE + (double)this->mState.rpm / RPM_MAX;
    double P = this->mState.usage / USAGE_MAX * this->mState.get_power();

    // dTdt = (Power Ratio - (Cooling Ratio * (T - 25))) / 10
    double dTdt = (P - C * (this->mState.temperature - T_AMBIENT)) / C_TH;
    double dt = this->mState.get_dt();
    //this->log() << "dt: " << dt << "  |  dT: " << (dTdt * dt) << endl;
    double T = this->mState.temperature + (dTdt * dt);
    //this->log() << "T in: " << this->mState.temperature << "  |  T out: " << T << endl;
    this->mState.setTemperature(T);

    // If latch is low, there's nothing to process.
    if (this->targetDevice->getGpio("latch") == 0) {
        requestReportState();
        return;
    }
    
    int temp_int = static_cast<int>(round(this->mState.temperature));
    for (int i = BITS_PER_TEMP; i >= 0; i--) {
        targetDeviceOutputData[i] = (temp_int >> (BITS_PER_TEMP - 1 - i)) & 1;
    }

    // Wait for latch to become 0
    while (this->targetDevice->getGpio("latch") == 1) {}

    // Process the received data and update
    if (this->communicator->performSerialCommunication<BITS_PER_RPM, BITS_PER_TEMP>(targetDeviceInputData, targetDeviceOutputData)) {
        // Fan speed update
        int rpm = 0;
        for (int i = BITS_PER_RPM-1; i >= 0; i--) {
            rpm += targetDeviceInputData[i] ? pow(2, i) : 0;
        }
        this->log() << "rpm array size: " << targetDeviceInputData.size() << endl;
        this->log() << "rpm: " << rpm << endl;
        if (rpm < 600) rpm = 600;
        if (rpm > 3000) rpm = 3000;

        this->mState.setRPM(rpm);
        this->log() << "TEMP: " << this->mState.temperature << " RPM: " << this->mState.rpm << endl;
        this->log() << "Reporting:" << this->mState.serialize() << endl;
        requestReportState();
    }
}
