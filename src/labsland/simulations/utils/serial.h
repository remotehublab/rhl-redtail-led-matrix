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
#include <memory>
#include "../simulation.h"

namespace LabsLand::Simulations::Utils {
    extern const char DEFAULT_PULSE_NAME [];

    template <size_t N_INPUTS, size_t N_OUTPUTS, const char * PULSE_NAME = DEFAULT_PULSE_NAME>
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
                        while (this->targetDevice->getGpio(PULSE_NAME) == 0) {}

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
                        while (this->targetDevice->getGpio(PULSE_NAME) == 1) {}
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

    template <size_t N_OUTPUTS, const char * PULSE_NAME = DEFAULT_PULSE_NAME>
    class OutputSerialCommunicator {
        private:
            std::shared_ptr<SerialCommunicator<0, N_OUTPUTS, PULSE_NAME>> communicator;
            std::array<std::bitset<0>, 0>& input_buffer;

        public:
            OutputSerialCommunicator(
                std::shared_ptr<LabsLand::Utils::TargetDevice> targetDevice, 
                const std::array<std::string, N_OUTPUTS> output_gpios
            ) {
                std::array<std::string, 0> input_gpios;
                this->communicator = std::make_shared<SerialCommunicator<0, N_OUTPUTS, PULSE_NAME>>(targetDevice, input_gpios, output_gpios);
            }

            template <size_t OUT_PULSES>
            bool sendSerialData(
                const std::array<std::bitset<OUT_PULSES>, N_OUTPUTS>& output_buffer
            ) {
                return this->communicator->performSerialCommunication<0, N_OUTPUTS>(input_buffer, output_buffer);
            }
    };

    template <size_t N_INPUTS, const char * PULSE_NAME = DEFAULT_PULSE_NAME>
    class InputSerialCommunicator {
        private:
            std::shared_ptr<SerialCommunicator<N_INPUTS, 0, PULSE_NAME>> communicator;
            const std::array<std::bitset<0>, 0> output_buffer;

        public:
            InputSerialCommunicator(
                std::shared_ptr<LabsLand::Utils::TargetDevice> targetDevice, 
                const std::array<std::string, N_INPUTS> input_gpios
            ) {
                std::array<std::string, 0> output_gpios;
                this->communicator = std::make_shared<SerialCommunicator<N_INPUTS, 0, PULSE_NAME>>(targetDevice, input_gpios, output_gpios);
            }

            template <size_t IN_PULSES>
            bool receiveSerialData(
                std::array<std::bitset<IN_PULSES>, N_INPUTS>& input_buffer
            ) {
                return this->communicator->performSerialCommunication<N_INPUTS, 0>(input_buffer, this->output_buffer);
            }
    };

    template <const char* PULSE_NAME = DEFAULT_PULSE_NAME>
    class SingleSerialCommunicator {
        private:
            std::shared_ptr<SerialCommunicator<1, 1, PULSE_NAME>> communicator;

        public:
            SingleSerialCommunicator(
                std::shared_ptr<LabsLand::Utils::TargetDevice> targetDevice, 
                const std::string inputGpioName,
                const std::string outputGpioName
            ) {
                const std::array<std::string, 1> input_gpios({inputGpioName});
                const std::array<std::string, 1> output_gpios({outputGpioName});
                this->communicator = std::make_shared<SerialCommunicator<1, 1, PULSE_NAME>>(targetDevice, input_gpios, output_gpios);
            }

            template <size_t IN_PULSES, size_t OUT_PULSES>
            bool performSerialCommunication(
                std::array<std::bitset<IN_PULSES>, 1>& input_buffer, 
                const std::bitset<OUT_PULSES>& output_buffer
            ) {
                const std::array<std::bitset<OUT_PULSES>, 1> realOutputBuffer({ output_buffer });
                return this->communicator->performSerialCommunication<IN_PULSES, OUT_PULSES>(input_buffer, realOutputBuffer);
            }

            template <size_t IN_PULSES, size_t OUT_PULSES>
            bool performSerialCommunication(
                std::bitset<IN_PULSES> & input_buffer, 
                const std::bitset<OUT_PULSES>& output_buffer
            ) {
                std::array<std::bitset<IN_PULSES>, 1> temporaryInputBuffer;
                const std::array<std::bitset<OUT_PULSES>, 1> realOutputBuffer({ output_buffer });
                bool result = this->communicator->template 
                    performSerialCommunication<IN_PULSES, OUT_PULSES>(temporaryInputBuffer, realOutputBuffer);
                input_buffer = temporaryInputBuffer[0];
                return result;
            }
    };


    template <const char * PULSE_NAME = DEFAULT_PULSE_NAME>
    class SingleInputSerialCommunicator {
        private:
            std::shared_ptr<SerialCommunicator<1, 0, PULSE_NAME>> communicator;
            std::array<std::bitset<0>, 0> output_buffer;

        public:
            SingleInputSerialCommunicator(
                std::shared_ptr<LabsLand::Utils::TargetDevice> targetDevice, 
                const std::string gpioName
            ) {
                std::array<std::string, 0> output_gpios;
                const std::array<std::string, 1> input_gpios({gpioName});
                this->communicator = std::make_shared<SerialCommunicator<1, 0, PULSE_NAME>>(targetDevice, input_gpios, output_gpios);
            }

            template <size_t IN_PULSES>
            bool receiveSerialData(
                std::array<std::bitset<IN_PULSES>, 1>& input_buffer
            ) {
                return this->communicator->performSerialCommunication<IN_PULSES, 0>(input_buffer, this->output_buffer);
            }

            template <size_t IN_PULSES>
            bool receiveSerialData(
                std::bitset<IN_PULSES>& input_buffer
            ) {
                std::array<std::bitset<IN_PULSES>, 1> temporaryInputBuffer;
                bool result = this->communicator->template 
                    performSerialCommunication<IN_PULSES, 0>(temporaryInputBuffer, this->output_buffer);
                input_buffer = temporaryInputBuffer[0];
                return result;
            }
    };

    template <const char * PULSE_NAME = DEFAULT_PULSE_NAME>
    class SingleOutputSerialCommunicator {
        private:
            std::shared_ptr<SerialCommunicator<0, 1, PULSE_NAME>> communicator;
            std::array<std::bitset<0>, 0> input_buffer;

        public:
            SingleOutputSerialCommunicator(
                std::shared_ptr<LabsLand::Utils::TargetDevice> targetDevice, 
                const std::string & gpioName
            ) {
                std::array<std::string, 0> input_gpios;
                std::array<std::string, 1> output_gpios({ gpioName });
                this->communicator = std::make_shared<SerialCommunicator<0, 1, PULSE_NAME>>(targetDevice, input_gpios, output_gpios);
            }

            template <size_t OUT_PULSES>
            bool sendSerialData(
                const std::bitset<OUT_PULSES>& output_buffer
            ) {
                const std::array<std::bitset<OUT_PULSES>, 1> finalOutputBuffer({ output_buffer });
                return this->communicator->performSerialCommunication<0, 1>(input_buffer, finalOutputBuffer);
            }
    };


    class DefaultSingleSerialCommunicator
        : public SingleSerialCommunicator<DEFAULT_PULSE_NAME>
    {
        public:
            using SingleSerialCommunicator<DEFAULT_PULSE_NAME>::SingleSerialCommunicator;
    };

    class DefaultSingleInputSerialCommunicator
        : public SingleInputSerialCommunicator<DEFAULT_PULSE_NAME> 
    {
        public:
            using SingleInputSerialCommunicator<DEFAULT_PULSE_NAME>::SingleInputSerialCommunicator;
    };

    class DefaultSingleOutputSerialCommunicator
        : public SingleOutputSerialCommunicator<DEFAULT_PULSE_NAME> 
    {
        public:
            using SingleOutputSerialCommunicator<DEFAULT_PULSE_NAME>::SingleOutputSerialCommunicator;
    };
}

#endif