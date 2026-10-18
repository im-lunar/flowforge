#include "engine/simulator/machine.h"
#include <cmath>
#include <algorithm>

namespace engine {
    
    Machine::Machine(std::string id, MachineConfig config) 
        : id_(std::move(id))
        , config_(std::move(config))
        , state_(std::move(MachineState::NORMAL))
        , temperature_(config_.base_temperature)
        , pressure_(config_.base_pressure)
        , vibration_(config_.base_vibration)
        , rpm_(config_.base_rpm)
        , rng_(std::random_device{}())
    {};

    void Machine::step() {
        switch (state_)
        {
            case MachineState::NORMAL: {
                std::normal_distribution<double> temp_dist(config_.base_temperature, config_.temp_stddev);
                std::normal_distribution<double> press_dist(config_.base_pressure, config_.pressure_stddev);
                std::normal_distribution<double> vib_dist(config_.base_vibration, config_.vibration_stddev);
                std::normal_distribution<double> rpm_dist(config_.base_rpm, config_.rpm_stddev);

                temperature_ = temp_dist(rng_);
                pressure_ = std::max(0.0, press_dist(rng_));
                vibration_ = std::max(0.0, vib_dist(rng_));
                rpm_ = std::max(0, static_cast<int>(rpm_dist(rng_)));
                break;
            };
            
            case MachineState::WARNING: {
                // Elevated baseline
                std::normal_distribution<double> temp_dist(config_.base_temperature + 12.0, config_.temp_stddev * 1.5);
                std::normal_distribution<double> vib_dist(config_.base_vibration * 1.8, config_.vibration_stddev);
                temperature_ = temp_dist(rng_);
                vibration_   = std::max(0.0, vib_dist(rng_));
                break;
            };
            
            case MachineState::OVERHEATING: {
                // Thermal drift: heat increases steadily with a little random jitter
                std::normal_distribution<double> noise(0.0, 0.4);
                temperature_ += 3.5 + noise(rng_);
                vibration_   += 0.15; // Increased friction
                
                // If temperature reaches dangerous extreme, trip into FAULT
                if (temperature_ > 110.0) {
                    state_ = MachineState::FAULT;
                }
                break;
            };

            case MachineState::FAULT: {
                // Emergency shutdown: motor stops, machine cools down
                rpm_ = 0;
                pressure_ = 0.0;
                temperature_ -= (temperature_ - 25.0) * 0.05; // Gradual cooling towards 25°C
                break;
            };
        }
    }

    json Machine::read_telemetry() const {
        return json{
            {"machine_id", id_},
            {"type", config_.type},
            {"state", machine_state_to_string(state_)},
            {"temperature", std::round(temperature_ * 10.0) / 10.0},
            {"pressure", std::round(pressure_ * 100.0) / 100.0},
            {"vibration", std::round(vibration_ * 100.0) / 100.0},
            {"rpm", rpm_}
        };
    }

    void Machine::trigger_anomaly(MachineState state) {
        state_ = state;
    }

    void Machine::reset() {
        state_ = MachineState::NORMAL;
        temperature_ = config_.base_temperature;
        pressure_ = config_.base_pressure;
        vibration_ = config_.base_vibration;
        rpm_ = config_.base_rpm;
    }

} // namespace engine