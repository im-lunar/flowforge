#include <cassert>
#include <iostream>
#include <memory>

#include "engine/simulator/machine.h"

using namespace engine;

int tests_passed = 0;
int tests_failed = 0;

void check(bool condition, const std::string& test_name) {
    if (condition) {
        std::cout << "  PASS:   " << test_name << "\n";
        tests_passed++;
    } 
    else {
        std::cout << "  FAILED:   " << test_name << "\n";
        tests_failed++;
    }
}

void test_read_telemetry() {
    std::cout << "\n[test_read_telemetry]\n";

    Machine pump("PUMP-01");
    json data = pump.read_telemetry();

    check(data.is_object(), "telemetry output is a JSON object");
    check(data["machine_id"] == "PUMP-01", "machine_id matches");
    check(data["type"] == "HYDRAULIC_PUMP", "machine type is HYDRAULIC_PUMP");
    check(data["state"] == "NORMAL", "initial state string is NORMAL");

    // Check that all physical sensor fields are present
    check(data.contains("temperature"), "contains temperature field");
    check(data.contains("pressure"), "contains pressure field");
    check(data.contains("vibration"), "contains vibration field");
    check(data.contains("rpm"), "contains rpm field");

    // Check baseline values match preset
    check(data["temperature"] == 65.0, "initial temperature is 65.0");
    check(data["rpm"] == 1750, "initial rpm is 1750");
}

void test_step() {
    std::cout << "\n[test_step]\n";

    Machine pump("PUMP-01");

    // Step 5 times and verify readings fluctuate within realistic bounds
    for (int i = 0; i < 5; ++i) {
        pump.step();
    }

    // Normal pump baseline is 65°C with stddev 1.2°C; should remain between 60°C and 70°C
    check(pump.temperature() >= 60.0 && pump.temperature() <= 70.0, 
          "temperature stays within realistic normal bounds");

    // Pressure should stay positive and near 6.0 bar
    check(pump.pressure() >= 4.0 && pump.pressure() <= 8.0, 
          "pressure fluctuates in normal operating range");

    // Motor should be actively spinning near 1750 RPM
    check(pump.rpm() >= 1600 && pump.rpm() <= 1900, 
          "rpm stays in normal motor operating range");

    check(pump.state() == MachineState::NORMAL, "state remains NORMAL after normal steps");
}

void test_trigger_anomaly() {
    std::cout << "\n[test_trigger_anomaly - Overheating & Fault]\n";

    Machine pump("PUMP-03");

    double start_temp = pump.temperature();

    // Trigger overheating
    pump.trigger_anomaly(MachineState::OVERHEATING);
    check(pump.state() == MachineState::OVERHEATING, "state changed to OVERHEATING");

    // Step 10 ticks: temperature should climb steadily (+3.5°C per tick)
    for (int i = 0; i < 10; ++i) {
        pump.step();
    }

    check(pump.temperature() > start_temp + 20.0, 
          "temperature drifted significantly upwards (+20°C or more)");

    // Step 20 more ticks: temperature will cross the 110°C critical threshold
    for (int i = 0; i < 20; ++i) {
        pump.step();
    }

    // Machine should have automatically tripped into emergency FAULT
    check(pump.state() == MachineState::FAULT, 
          "extreme heat (>110°C) automatically tripped machine into FAULT");

    check(pump.rpm() == 0, "emergency shutdown stopped the motor (rpm == 0)");
    check(pump.pressure() == 0.0, "emergency shutdown dropped pressure to 0");
}

void test_reset() {
    std::cout << "\n[test_reset]\n";

    Machine pump("PUMP-04");

    // Put machine in bad state
    pump.trigger_anomaly(MachineState::OVERHEATING);

    for (int i = 0; i < 5; ++i) {
        pump.step();

    }
    check(pump.state() == MachineState::OVERHEATING, "machine is in OVERHEATING state before reset");

    // Reset machine
    pump.reset();

    check(pump.state() == MachineState::NORMAL, "reset restored state to NORMAL");
    check(pump.temperature() == 65.0, "reset restored baseline temperature");
    check(pump.rpm() == 1750, "reset restored baseline rpm");
    check(pump.pressure() == 6.0, "reset restored baseline pressure");
}


int main() {
    std::cout << "=== Machine Simulator tests === \n" ;
    
    test_read_telemetry();
    test_step();
    test_trigger_anomaly();
    test_reset();

    std::cout << "\n=== Results ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";
    
    return tests_failed > 0 ? 1 : 0;
}