/*
 * Copyright (C) 2023 onwards LabsLand, Inc.
 * All rights reserved.
 *
 * This software is licensed as described in the file LICENSE, which
 * you should have received as part of this distribution.
 * 
 * The following code has been authored and contributed by:
 *   Colton Harris, Remote Hub Lab of the University of Washington
 */
#ifndef SIMULATION_UTILS_SERIAL_H
#define SIMULATION_UTILS_SERIAL_H

#include <bitset>
#include <array>
#include <string>
#include "../simulation.h"

namespace LabsLand::Simulations::Utils {

    template <size_t N_INPUTS, size_t N_OUTPUTS>
    class SerialCommunicator {
        private:
            std::shared_ptr<LabsLand::Utils::TargetDevice> targetDevice = nullptr;
            std::array<std::string, N_INPUTS> input_gpios;
            std::array<std::string, N_OUTPUTS> output_gpios;


        public:
            SerialCommunicator(
                std::shared_ptr<LabsLand::Utils::TargetDevice> targetDevice, 
                const std::array<std::string, N_INPUTS>& input_gpios, 
                const std::array<std::string, N_OUTPUTS>& output_gpios
            ): targetDevice(targetDevice), input_gpios(input_gpios), output_gpios(output_gpios) 
            {}

            virtual void log(std::string message) {
                this->targetDevice->log() << message << std::endl;
            }

            // Performs simultaneous serial send and recieve operations for GPIO-based communication with a DUT (Device Under Test).
            //
            // Parameters:
            // - input_buffer: A 2D boolean vector to be filled with received signal values from the DUT. 
            //                 Each inner vector corresponds to one GPIO pin's signal over time.
            //                 The outer index of the buffer matches the index of GPIO names in `input_gpios`.
            //
            // - output_buffer: A 2D boolean vector containing signal values to be sent to the DUT.
            //                  Each inner vector represents one GPIO pin's signal over time.
            //                  The outer index of the buffer matches the index of GPIO names in `output_gpios`.
            //
            // Cardinality:
            //   input_buffer has M x N elements (gpios x messages per GPIO)
            //   input_gpios must have M elements (gpios)
            //   output_buffer has M' x N' elements (gpios x messages per GPIO)
            //   output_gpios has M' elements (gpios)
            //
            // Returns:
            // - A boolean indicating whether the communication was successful.
            template <size_t IN_PULSES, size_t OUT_PULSES>
            bool performSerialCommunication(
                std::array<std::bitset<IN_PULSES>, N_INPUTS>& input_buffer, 
                const std::array<std::bitset<OUT_PULSES>, N_OUTPUTS>& output_buffer
            ) {
                try {
                    constexpr size_t MAX_PULSES = (IN_PULSES > OUT_PULSES ? IN_PULSES : OUT_PULSES);
                    
                    for (int pulse = static_cast<int>(MAX_PULSES)-1; pulse >= 0; pulse--) {
                        // Wait for a rising edge on the pulse GPIO
                        while (this->targetDevice->getGpio("pulse") == 0) {}

                        this->log("begining pulse");

                        // Read the data bit when the pulse is high
                        for (int input_pin = 0; input_pin < N_INPUTS; input_pin++) {
                            if (pulse < static_cast<int>(IN_PULSES)) {
                                input_buffer[input_pin][pulse] = this->targetDevice->getGpio(input_gpios[input_pin]);
                            }
                        }
                        this->log("input done");

                        // Write the data bit when the pulse is high
                        for (int output_pin = 0; output_pin < N_OUTPUTS; output_pin++) {
                            if (pulse < static_cast<int>(OUT_PULSES)) {
                                this->targetDevice->setGpio(output_gpios[output_pin], output_buffer[output_pin][pulse]);
                            } else {
                                this->targetDevice->setGpio(output_gpios[output_pin], false);
                            }
                        }
                            
                        // Wait for the pulse to go low again before reading the next bit
                        while (this->targetDevice->getGpio("pulse") == 1) {}
                    }
                    printInput<IN_PULSES>(input_buffer);
                    printOutput<OUT_PULSES>(output_buffer);
                } catch (std::exception e) {
                    this->log("exception!");
                    return false;
                }
                return true;
            }

            template <size_t IN_PULSES>
            void printInput(std::array<std::bitset<IN_PULSES>, N_INPUTS>& input_buffer) {
                for (int i = 0; i < input_buffer.size(); i++) {
                    std::string row_result = "";
                    for (int j = input_buffer[i].size()-1; j >= 0 ; j--) {
                        row_result += input_buffer[i][j] ? "1" : "0";
                    }
                    this->targetDevice->log() << this->input_gpios[i] << " <-- " << row_result << std::endl;
                }
            }
            
            template <size_t OUT_PULSES>
            void printOutput(const std::array<std::bitset<OUT_PULSES>, N_OUTPUTS>& output_buffer) {
                for (int i = 0; i < output_buffer.size(); i++) {
                    std::string row_result = "";
                    for (int j = output_buffer[i].size()-1; j >= 0 ; j--) {
                        row_result += output_buffer[i][j] ? "1" : "0";
                    }
                    this->targetDevice->log() << this->output_gpios[i] << " --> " << row_result << std::endl;
                }
            }
    };

}

#endif