#include <iostream>
#include <stdio.h>
#include <string.h>
#include <array>
#include "matrix.h"

using namespace std;
using namespace RHLab::LEDMatrix;

void MatrixSimulation::initialize(){
    this->targetDevice->initializeSimulation({}, {"latch", "pulse", "green", "red"});

    this->communicator = std::make_shared<LabsLand::Simulations::Utils::InputSerialCommunicator<BITS_PER_LED>>(
        this->targetDevice,
        std::array<std::string, 2>({
            "green", "red"
        })
    );

    setReportWhenMarked(true);
}

void MatrixSimulation::update(double delta) {
    // If latch is low, there's nothing to process.
    if (this->targetDevice->getGpio("latch") == 0) {
        return;
    }
    
    // 2D ROWS * COLS x BITS_PER_LED (inputGPIOs.size()), bools for input data
    std::array<std::bitset<ROWS*COLS>, BITS_PER_LED> targetDeviceInputData;

    // Wait for latch to become 0
    while (this->targetDevice->getGpio("latch") == 1) {}

    // Process the received data and update the matrix
    if (this->communicator->receiveSerialData<ROWS*COLS>(targetDeviceInputData)) {
        for (int row = 0; row < ROWS; row++) {
            for (int col = 0; col < COLS; col++) {
                // Map the received data to LED matrix states
                int bitIndex = row * COLS + col;
                bool green = targetDeviceInputData[0][bitIndex];
                bool red = targetDeviceInputData[1][bitIndex];
                char color = ' ';
                if (red && green) {
                    color = 'Y';
                } else if (green) {
                    color = 'G';
                } else if (red) {
                    color = 'R';
                } else {
                    color = 'B';
                }
                this->mState.setLed(row, col, color);
            }
        }
        this->log() << "Reporting:" << this->mState.serialize() << endl;
        requestReportState();
    }
}
