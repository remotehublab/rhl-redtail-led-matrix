
#ifndef COMPUTER_FAN_SIMULATION_H
#define COMPUTER_FAN_SIMULATION_H

#include "../labsland/simulations/simulation.h"
#include <string>
#include <cstring>
#include <sstream>
#include <stdio.h>
#include <vector>

using namespace std;

#define MAX_RPM 3000
#define MAX_WATTS 100.0
#define MAX_USAGE 100
#define T_AMBIENT 25.0
#define dT 0.1
#define C_TH 10.0

namespace RHLab::ComputerFan {
    const int BITS_PER_TEMP = 8;
    const int BITS_PER_RPM = 12;
    
    const int INPUTS = 3; // Latch, Pulse and RPM
    const int OUTPUTS = 3; // Latch, Pulse, and Temperature
    

    // information received from the web browser (only the state)
    struct ComputerFanRequest : public BaseInputDataType {

        char state = 'I'; // 'I' is idle, 'U' in use and 'L' is under load

        bool deserialize(std::string const & input) {
            if (input.size()) {
                if (input == "I") // idle
                    state = 'I';
                else if (input == "U") // in use
                    state = 'U';
                else if (input == "L") // under load
                    state = 'L';
                else {
                    // no error management for now
                    return false;
                }
                // If there was any change
                return true;
            }
            return false;
        }
    };

    // information sent to the web browser
    struct ComputerFanData : public BaseOutputDataType {
        double usage;
        double temperature;
        int rpm;
        char state;
        

        public:
            ComputerFanData() {
                usage = 0.0;
                temperature = T_AMBIENT;
                rpm = 600;
                state = 'I';
            }

            void updateUsage(char state) {
                // State usage bounds
                double low, high;
                if (state == 'I') { // idle
                    low = 0;
                    high = 20;
                    state = 'I';
                } else if (state == 'U') { // in use
                    low = 30;
                    high = 60;
                    state = 'U';
                } else { // 'L' (under load)
                    low = 70;
                    high = MAX_USAGE;
                    state = 'L';
                }
                // Random walk on usage
                double delta = ((double)rand() / RAND_MAX) * 10.0 - 5.0;
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
