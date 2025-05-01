
#ifndef COMPUTER_FAN_SIMULATION_H
#define COMPUTER_FAN_SIMULATION_H

#include "../labsland/simulations/simulation.h"
#include <chrono>
#include <string>
#include <cstring>
#include <sstream>
#include <stdio.h>
#include <vector>

using namespace std;

#define RPM_MIN 600     // Min RPM Speed
#define RPM_MAX 3000    // Max RPM Speed

#define WATTS_MAX 100.0     // Watts Peak
#define WATTS_THROTTLE 70   // Watts when thermal throttling

#define OFF       0     // Off
#define USAGE_MIN 1     // Min Usage
#define USAGE_MAX 100   // Max Usage

#define T_AMBIENT 25.0  // Temp of surroundings
#define T_THROTTLE 90   // Temp to start throttling
#define T_SHUTDOWN 100  // Temp to shutdown

#define C_PASSIVE 0.2   // Passive Cooling Factor
#define C_TH 25.0       // Heating Rate (higher=slower)

namespace RHLab::ComputerFan {
    const int BITS_PER_TEMP = 8;
    const int BITS_PER_RPM = 12;
    
    const int INPUTS = 3; // Latch, Pulse and RPM
    const int OUTPUTS = 3; // Latch, Pulse, and Temperature
    

    // information received from the web browser (only the state)
    struct ComputerFanRequest : public BaseInputDataType {

        char state = 'L'; // Low, Medium, & High

        bool deserialize(std::string const & input) {
            if (input.size()) {
                if (input == "L") // idle
                    state = 'L';
                else if (input == "M") // in use
                    state = 'M';
                else if (input == "H") // under load
                    state = 'H';
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
        chrono::steady_clock::time_point t_prev;

        

        public:
            ComputerFanData() {
                reset();
            }

            void reset() {
                usage = USAGE_MIN;
                temperature = T_AMBIENT;
                rpm = RPM_MIN;
                state = 'L';
                t_prev = std::chrono::steady_clock::now();
            }

            void shutdown() {
                this->state = 'S';
                this->usage = OFF;
                this->rpm = OFF;
            }

            void updateUsage(char state) {
                // Shutdown
                if (this->state == 'S') { return; }
                
                // Validate State
                if (state != 'L' && state != 'M' && state != 'H') {
                    cout << "Invalid State: " << state << endl;
                    state = this->state;
                }
                
                // Set High and Low
                double low, high;
                if (state == 'L') {   // Low Load
                    low = USAGE_MIN;
                    high = USAGE_MIN + 0.25 * (USAGE_MAX - USAGE_MIN);
                } else if (state == 'M') {  // Medium Load
                    low = USAGE_MIN + 0.26 * (USAGE_MAX - USAGE_MIN);
                    high = USAGE_MIN + 0.60 * (USAGE_MAX - USAGE_MIN);
                } else {                    // 'H' High Load
                    low = USAGE_MIN + 0.61 * (USAGE_MAX - USAGE_MIN);
                    high = USAGE_MAX;
                }

                // New State so set usage to middle
                if (this->state != state) {
                    this-> usage = (low + high) / 2;
                    this->state = state;
                }

                // Random walk on usage
                double delta = ((double)rand() / RAND_MAX) * 10.0 - 5.0;
                this->usage += delta / 3;
                if (this->usage < low) this->usage = low;
                if (this->usage > high) this->usage = high;
            }

            void setTemperature(double temperature) {
                if (temperature > T_SHUTDOWN) {
                    shutdown();
                    return;
                }
                this->temperature = temperature;
            }

            void setRPM(int rpm) {
                if (state == 'S') { return; }
                this->rpm = rpm;
            }

            double get_power() {
                if (state == 'S') { return OFF; }
                if (this->temperature >= T_THROTTLE) {
                    return WATTS_THROTTLE;
                } else {
                    return WATTS_MAX;
                }
            }
            
            double get_dt() {
                chrono::steady_clock::time_point t1 = this->t_prev;
                chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
                this->t_prev = t2;

                std::chrono::duration<double> dt = t2 - t1;
                return dt.count();
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
