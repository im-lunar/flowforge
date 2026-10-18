#pragma once

#include <string>
#include <random>
#include <nlohmann/json.hpp>

namespace engine {

    using json = nlohmann::json;

    enum class MachineState {
        NORMAL,
        WARNING,
        OVERHEATING,
        FAULT
    };

    inline std::string machine_state_to_string(MachineState state) {
        switch (state)
        {
        case MachineState::NORMAL:  return "NORMAL";
        case MachineState::WARNING:  return "WARNING";
        case MachineState::OVERHEATING:  return "OVERHEATING";
        case MachineState::FAULT:  return "FAULT";
        default:    return "UNKNOWN";
        }
    }

    struct MachineConfig {
        std::string type;
        // baseline parameters        
        double base_temperature = 70.0;
        double temp_stddev = 1.0;
        double base_pressure = 5.0;
        double pressure_stddev = 0.2;
        double base_vibration = 1.0;
        double vibration_stddev = 0.1;
        int base_rpm = 1800;
        int rpm_stddev = 25;
        
        // Convenient factory presets for different factory assets (like pump, turbine, conveyor)
        static MachineConfig pump_preset() {
            MachineConfig cfg;
            cfg.type = "HYDRAULIC_PUMP";
            cfg.base_temperature = 65.0;
            cfg.temp_stddev = 1.2;
            cfg.base_pressure = 6.0;
            cfg.pressure_stddev = 0.3;
            cfg.base_vibration = 0.8;
            cfg.base_rpm = 1750;
            return cfg;
        };

        static MachineConfig turbine_preset() {
            MachineConfig cfg;
            cfg.type = "WIND_TURBINE";
            cfg.base_temperature = 80.0;
            cfg.temp_stddev = 1.5;
            cfg.base_pressure = 12.0;
            cfg.pressure_stddev = 0.5;
            cfg.base_vibration = 2.2;
            cfg.base_rpm = 3000;
            return cfg;
        };

        static MachineConfig conveyor_preset() {
            MachineConfig cfg;
            cfg.type = "CONVEYOR_BELT";
            cfg.base_temperature = 45.0;
            cfg.temp_stddev = 0.8;
            cfg.base_pressure = 1.0;
            cfg.pressure_stddev = 0.1;
            cfg.base_vibration = 0.5;
            cfg.base_rpm = 120;
            return cfg;
        };        
    };


    class Machine {

        public:
            Machine (std::string id, MachineConfig config = MachineConfig::pump_preset());

            // Advances the physics by 1 simulation tick
            void step();

            // Generates current sensor readings as JSON
            json read_telemetry() const;
            
            // Simulates an anomaly event (e.g. OVERHEATING)
            void trigger_anamoly(MachineState state);

            // Resets the machine back to normal conditions
            void reset();

            // Accessors
            const std::string& id() const { return id_; }
            const std::string& type() const { return config_.type; }
            MachineState state() const { return state_; }
            double temperature() const { return temperature_; }
            double pressure() const { return pressure_; }
            double vibration() const { return vibration_; }
            int rpm() const { return rpm_; }


        private:
            std::string id_;
            MachineConfig config_;
            MachineState state_;

            double temperature_;
            double pressure_;
            double vibration_;
            int rpm_;

            // Modern C++ Mersenne Twister random engine
            mutable std::mt19937 rng_;
    };

} // namespace engine