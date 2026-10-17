#include <iostream>
#include <memory>
#include <cassert>

#include "engine/workflow/workflow.h"
#include "engine/workflow/edge.h"
#include "engine/workflow/workflow_validator.h"
#include "engine/nodes/trigger_node.h"
#include "engine/nodes/condition_node.h"
#include "engine/nodes/action_node.h"
#include "engine/execution/workflow_executor.h"

using namespace engine;

int tests_passed = 0;
int tests_failed = 0;

void check(bool condition, const std::string& test_name) {
    if (condition) {
        std::cout << "  PASS: " << test_name << "\n";
        tests_passed++;
    } else {
        std::cout << "  FAIL: " << test_name << "\n";
        tests_failed++;
    }
}

void test_empty_workflow() {
    std::cout << "\n[test_empty_workflow]\n";

    Workflow wf("wf_empty", "Empty Workflow");
    WorkflowValidator validator;
    ValidationResult result = validator.validate(wf);

    check(!result.is_valid(), "empty workflow is invalid");
    check(!result.errors.empty(), "validation produced error for empty workflow");
}

void test_multiple_start_nodes() {
    std::cout << "\n[test_multiple_start_nodes]\n";

    Workflow wf("wf_multi_start", "Multiple Starts");
    wf.add_node(std::make_unique<TriggerNode>("start_1", "Start 1"));
    wf.add_node(std::make_unique<TriggerNode>("start_2", "Start 2"));
    wf.add_node(std::make_unique<ActionNode>("action", "Action"));

    // Both start nodes point to action -> two in-degree 0 nodes!
    wf.add_edge(Edge("e1", "start_1", "action"));
    wf.add_edge(Edge("e2", "start_2", "action"));

    WorkflowValidator validator;
    ValidationResult result = validator.validate(wf);

    check(!result.is_valid(), "workflow with multiple start nodes is invalid");
}

void test_unreachable_node() {
    std::cout << "\n[test_unreachable_node]\n";

    Workflow wf("wf_unreachable", "Unreachable Node Test");
    wf.add_node(std::make_unique<TriggerNode>("n1", "Start"));
    wf.add_node(std::make_unique<ActionNode>("n2", "Step 2"));
    wf.add_node(std::make_unique<ActionNode>("n3_orphan", "Orphan")); // Disconnected!

    wf.add_edge(Edge("e1", "n1", "n2"));

    WorkflowValidator validator;
    ValidationResult result = validator.validate(wf);

    check(!result.is_valid(), "workflow with unreachable node is invalid");
    bool found_orphan_msg = false;
    for (const auto& err : result.errors) {
        if (err.find("n3_orphan") != std::string::npos) {
            found_orphan_msg = true;
            break;
        }
    }
    check(found_orphan_msg, "error message identifies orphaned node");
}

void test_cycle_detection() {
    std::cout << "\n[test_cycle_detection]\n";

    Workflow wf("wf_cycle", "Cycle Workflow");
    wf.add_node(std::make_unique<TriggerNode>("n1", "Node 1"));
    wf.add_node(std::make_unique<ActionNode>("n2", "Node 2"));
    wf.add_node(std::make_unique<ActionNode>("n3", "Node 3"));

    // Cycle: n1 -> n2 -> n3 -> n2
    wf.add_edge(Edge("e1", "n1", "n2"));
    wf.add_edge(Edge("e2", "n2", "n3"));
    wf.add_edge(Edge("e3", "n3", "n2")); // Back-edge!

    WorkflowValidator validator;
    ValidationResult result = validator.validate(wf);

    check(!result.is_valid(), "cyclic workflow is invalid");
    bool found_cycle_msg = false;
    for (const auto& err : result.errors) {
        if (err.find("Cycle detected") != std::string::npos) {
            found_cycle_msg = true;
            break;
        }
    }
    check(found_cycle_msg, "error message specifies cycle was detected");
}

void test_condition_node_missing_branches() {
    std::cout << "\n[test_condition_node_missing_branches]\n";

    Workflow wf("wf_no_branch", "Condition Missing Branches");
    wf.add_node(std::make_unique<TriggerNode>("sensor", "Sensor"));

    json cond_config = {
        {"field", "temperature"},
        {"operator", ">"},
        {"value", 90.0}
    };
    wf.add_node(std::make_unique<ConditionNode>("check", "Check", cond_config));
    wf.add_edge(Edge("e1", "sensor", "check"));
    // check has NO outgoing edges!

    WorkflowValidator validator;
    ValidationResult result = validator.validate(wf);

    check(!result.is_valid(), "condition node with no branches is invalid");
}

void test_valid_workflow() {
    std::cout << "\n[test_valid_workflow]\n";

    Workflow wf("wf_valid", "Valid Workflow");
    wf.add_node(std::make_unique<TriggerNode>("sensor", "Sensor"));

    json cond_config = {
        {"field", "temperature"},
        {"operator", ">"},
        {"value", 90.0}
    };
    wf.add_node(std::make_unique<ConditionNode>("check", "Check", cond_config));
    wf.add_node(std::make_unique<ActionNode>("alert", "Alert"));
    wf.add_node(std::make_unique<ActionNode>("continue", "Continue"));

    wf.add_edge(Edge("e1", "sensor", "check"));
    wf.add_edge(Edge("e2", "check", "alert", "true"));
    wf.add_edge(Edge("e3", "check", "continue", "false"));

    WorkflowValidator validator;
    ValidationResult result = validator.validate(wf);

    check(result.is_valid(), "valid workflow passes all validation checks");
    check(result.errors.empty(), "zero validation errors on valid workflow");
}

void test_executor_rejects_invalid_workflow() {
    std::cout << "\n[test_executor_rejects_invalid_workflow]\n";

    // Cycle workflow
    Workflow wf("wf_bad", "Bad Workflow");
    wf.add_node(std::make_unique<TriggerNode>("n1", "Node 1"));
    wf.add_node(std::make_unique<ActionNode>("n2", "Node 2"));
    wf.add_edge(Edge("e1", "n1", "n2"));
    wf.add_edge(Edge("e2", "n2", "n1")); // Cycle

    ExecutionContext ctx;
    WorkflowExecutor executor;
    auto result = executor.execute(wf, ctx);

    check(!result.is_success(), "executor aborted invalid workflow");
    check(result.error_message.find("Workflow validation failed") != std::string::npos, 
          "executor error indicates validation failure");
}

int main() {
    std::cout << "=== Workflow Engine Validation Tests (Phase 3) ===\n";

    test_empty_workflow();
    test_multiple_start_nodes();
    test_unreachable_node();
    test_cycle_detection();
    test_condition_node_missing_branches();
    test_valid_workflow();
    test_executor_rejects_invalid_workflow();

    std::cout << "\n=== Results ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}