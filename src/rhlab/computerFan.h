
#ifndef COMPUTER_FAN_SIMULATION_H
#define COMPUTER_FAN_SIMULATION_H

#include "../labsland/simulations/simulation.h"
#include <string>
#include <cstring>
#include <sstream>
#include <stdio.h>
#include <vector>

using namespace std;

namespace RHLab::ComputerFan {
    const int BITS_PER_TEMP = 8;
    const int BITS_PER_RPM = 12;
    
    const int INPUTS = 3; // Latch, Pulse and RPM
    const int OUTPUTS = 3; // Latch, Pulse, and Temperature
    

    // struct that receives the string; NOT USED FOR NOW
    struct ComputerFanRequest : public BaseInputDataType {
        bool deserialize(std::string const & input) {
            return false;
        }
    };

    // struct that tracks the virtual LED states
    struct ComputerFanData : public BaseOutputDataType {
        string state;
        double usage;
        double temperature;
        int rpm;

        public:
            ComputerFanData() {
                state = "idle";
                usage = 0.0;
                temperature = 20.0;
                rpm = 600;
            }

            void setState(string state) {
                if (state == "idle" || state == "in_use" || state == "under_load") {
                    this->state = state;
                }
            }

            void updateUsage() {
                // State usage bounds
                double low, high;
                if (this->state == "idle") { 
                    low = 0;
                    high = 20;
                } else if (this->state == "in_use") {
                    low = 30;
                    high = 60;
                } else {
                    low = 70;
                    high = 100;
                }
                // Random walk on usage
                double delta = (rand() % 1001) / 100.0 - 5.0; // -5 to +5
                this->usage += delta;
                if (this->usage < low) this->usage = low;
                if (this->usage > high) this->usage = high;
            }

            void setTemperature(int temperature) {
                this->temperature = temperature;
            }

            void setRPM(int rpm) {
                this->rpm = rpm;
            }

            string serialize() const {
                stringstream stream;
                stream << temperature << ":" << rpm;
                return stream.str();
            }
    };

    class ComputerFanSimulation : public Simulation<ComputerFanData, ComputerFanRequest> {
        public:
            ComputerFanSimulation() = default;
            void update(double delta) override;
            bool performSerialCommunication(
                vector<vector<bool>>& input_buffer, 
                vector<string>& input_gpios,
                vector<vector<bool>>& output_buffer,
                vector<string>& output_gpios);
            void initialize() override;
            void printInput(vector<vector<bool>>& input_buffer, vector<string>& input_gpios);
            void printOutput(vector<vector<bool>>& output_buffer, vector<string>& output_gpios);
    };
}

#endif
