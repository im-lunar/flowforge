#include <iostream>
#include <cassert>
#include <memory>

#include "engine/workflow/edge.h"
#include "engine/workflow/workflow.h"
#include "engine/core/execution_context.h"
#include "engine/core/execution_result.h"
#include "engine/nodes/trigger_node.h"
#include "engine/nodes/condition_node.h"
#include "engine/nodes/action_node.h"
#include "engine/execution/workflow_executor.h"

using namespace engine;

int tests_passed = 0;
int tests_failed = 0;

void check(bool condition, const std::string& test_name) {
    if (condition) {
        std::cout << " PASS: " << test_name << "\n"; 
        tests_passed++;
    } else {
        std::cout << " FAIL: " << test_name << "\n";
        tests_failed++;
    }
}

void test_execution_context() {
    std::cout << "\n[test_execution_context]\n";

    ExecutionContext ctx;
    check(!ctx.has("temperature"), "key does not exist initially");

    ctx.set("temperature", 94.5);
    check(ctx.has("temperature"), "has() returns true after set");
    check(ctx.get("temperature") == 94.5, "get() retrieves stored float");

    bool threw = false;
    try {
        ctx.get("pressure");
    } catch (const std::out_of_range&) {
        threw = true;
    }

    check(threw, "get() on nonexistent key throws out_of_range");
}

void test_linear_workflow() {
    std::cout << "\n[test_linear_workflow]\n";

    Workflow wf("wf_linear", "Linear Test");

    json trigger_config = {
        {"payload", {{"machine_id", "M001"}}}
    };
    wf.add_node(std::make_unique<TriggerNode>("t1", "Sensor Trigger", trigger_config));

    json action_config = {
        {"set_field", "status"},
        {"value", "PROCESSED"}
    };
    wf.add_node(std::make_unique<ActionNode>("a1", "Process Action", action_config));

    wf.add_edge(Edge("e1", "t1", "a1"));

    ExecutionContext ctx;
    WorkflowExecutor executor;
    ExecutionResult result = executor.execute(wf, ctx);

    check(result.is_success(), "linear workflow executed successfully");
    check(ctx.get("machine_id") == "M001", "context retained trigger payload");
    check(ctx.get("status") == "PROCESSED", "action node mutated context");
}

void test_branching_true_path() {
    std::cout << "\n[test_branching_true_path]\n";

    Workflow wf("wf_overheat", "Overheat Alert Workflow");

    // 1. Trigger with temperature = 97.0°C (> 90°C)
    json trigger_config = {
        {"payload", {
            {"machine_id", "M001"},
            {"temperature", 97.0}
        }}
    };
    wf.add_node(std::make_unique<TriggerNode>("sensor", "Sensor Event", trigger_config));

    // 2. Condition: temperature > 90
    json cond_config = {
        {"field", "temperature"},
        {"operator", ">"},
        {"value", 90.0}
    };
    wf.add_node(std::make_unique<ConditionNode>("check_temp", "Temperature Check", cond_config));

    // 3. Alert Action
    json alert_config = {
        {"action_type", "alert"},
        {"message", "CRITICAL: Machine M001 overheating!"}
    };
    wf.add_node(std::make_unique<ActionNode>("alert", "Create Alert", alert_config));

    // 4. Continue Action
    json continue_config = {
        {"set_field", "status"},
        {"value", "NORMAL"}
    };
    wf.add_node(std::make_unique<ActionNode>("continue", "Normal Operation", continue_config));

    // Edges: sensor -> check_temp -> [true: alert, false: continue]
    wf.add_edge(Edge("e1", "sensor", "check_temp"));
    wf.add_edge(Edge("e2", "check_temp", "alert", "true"));
    wf.add_edge(Edge("e3", "check_temp", "continue", "false"));

    ExecutionContext ctx;
    WorkflowExecutor executor;
    ExecutionResult result = executor.execute(wf, ctx);

    check(result.is_success(), "workflow executed successfully");
    check(ctx.has("alert_triggered") && ctx.get("alert_triggered") == true, "alert was triggered");
    check(ctx.get("alert_message") == "CRITICAL: Machine M001 overheating!", "alert message is correct");
    check(!ctx.has("status"), "false branch was NOT executed");
}

void test_branching_false_path() {
    std::cout << "\n[test_branching_false_path]\n";

    Workflow wf("wf_normal", "Normal Temperature Workflow");

    // 1. Trigger with normal temperature = 72.0°C (< 90°C)
    json trigger_config = {
        {"payload", {
            {"machine_id", "M001"},
            {"temperature", 72.0}
        }}
    };
    wf.add_node(std::make_unique<TriggerNode>("sensor", "Sensor Event", trigger_config));

    json cond_config = {
        {"field", "temperature"},
        {"operator", ">"},
        {"value", 90.0}
    };
    wf.add_node(std::make_unique<ConditionNode>("check_temp", "Temperature Check", cond_config));

    json alert_config = {
        {"action_type", "alert"},
        {"message", "CRITICAL: Overheating!"}
    };
    wf.add_node(std::make_unique<ActionNode>("alert", "Create Alert", alert_config));

    json continue_config = {
        {"set_field", "status"},
        {"value", "NORMAL"}
    };
    wf.add_node(std::make_unique<ActionNode>("continue", "Normal Operation", continue_config));

    wf.add_edge(Edge("e1", "sensor", "check_temp"));
    wf.add_edge(Edge("e2", "check_temp", "alert", "true"));
    wf.add_edge(Edge("e3", "check_temp", "continue", "false"));

    ExecutionContext ctx;
    WorkflowExecutor executor;
    ExecutionResult result = executor.execute(wf, ctx);

    check(result.is_success(), "workflow executed successfully");
    check(!ctx.has("alert_triggered"), "alert was NOT triggered");
    check(ctx.get("status") == "NORMAL", "false branch executed properly");
}

void test_condition_missing_field_failure() {
    std::cout << "\n[test_condition_missing_field_failure]\n";

    Workflow wf("wf_fail", "Missing Field Workflow");

    // Trigger doesn't supply temperature
    wf.add_node(std::make_unique<TriggerNode>("sensor", "Sensor Event"));

    json cond_config = {
        {"field", "temperature"},
        {"operator", ">"},
        {"value", 90.0}
    };
    wf.add_node(std::make_unique<ConditionNode>("check_temp", "Temperature Check", cond_config));
    wf.add_edge(Edge("e1", "sensor", "check_temp"));

    ExecutionContext ctx;
    WorkflowExecutor executor;
    ExecutionResult result = executor.execute(wf, ctx);

    check(!result.is_success(), "executor caught failure");
    check(!result.error_message.empty(), "failure produced error message");
}

void test_infinite_loop_protection() {
    std::cout << "\n[test_infinite_loop_protection]\n";

    Workflow wf("wf_loop", "Loop Workflow");

    wf.add_node(std::make_unique<TriggerNode>("n1", "Node 1"));
    wf.add_node(std::make_unique<ActionNode>("n2", "Node 2"));

    // Cycle: n1 -> n2 -> n1 ...
    wf.add_edge(Edge("e1", "n1", "n2"));
    wf.add_edge(Edge("e2", "n2", "n1"));

    ExecutionContext ctx;
    WorkflowExecutor executor(10); // max 10 steps
    ExecutionResult result = executor.execute(wf, ctx);
    
    check(!result.is_success(), "executor terminated infinite loop");
}

int main() {
    std::cout << "===== Workflow Engine Execution Tests =====\n";

    test_execution_context();
    test_linear_workflow();
    test_branching_true_path();
    test_branching_false_path();
    test_condition_missing_field_failure();
    test_infinite_loop_protection();

    std::cout << "\n=== Results ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}