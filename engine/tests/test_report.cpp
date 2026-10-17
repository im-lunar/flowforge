#include <iostream>
#include <memory>
#include <cassert>

#include "engine/workflow/workflow.h"
#include "engine/workflow/edge.h"
#include "engine/core/execution_context.h"
#include "engine/core/execution_report.h"
#include "engine/nodes/trigger_node.h"
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

// A node that fails `failures_before_success` times before succeeding
class FlakyNode : public Node {
public:
    FlakyNode(std::string id, std::string name, int failures_before_success, json config = json::object())
        : Node(std::move(id), std::move(name), NodeType::ACTION, std::move(config)),
          failures_needed_(failures_before_success),
          call_count_(0)
    {}

    std::string type_name() const override { return "FlakyNode"; }

    ExecutionResult execute(ExecutionContext& /*context*/) override {
        call_count_++;
        if (call_count_ <= failures_needed_) {
            return ExecutionResult::failure("Temporary sensor timeout on attempt " + std::to_string(call_count_));
        }
        return ExecutionResult::success();
    }

    int call_count() const { return call_count_; }

private:
    int failures_needed_;
    int call_count_;
};

void test_retry_eventual_success() {
    std::cout << "\n[test_retry_eventual_success]\n";

    Workflow wf("wf_retry_ok", "Retry Success Workflow");
    wf.add_node(std::make_unique<TriggerNode>("start", "Start Node"));

    // Flaky node fails 2 times, but is allowed 3 retries (total 4 attempts)
    json flaky_config = {
        {"max_retries", 3},
        {"retry_delay_ms", 10} // fast delay for unit tests
    };
    wf.add_node(std::make_unique<FlakyNode>("flaky", "Flaky Sensor", 2, flaky_config));
    wf.add_edge(Edge("e1", "start", "flaky"));

    ExecutionContext ctx;
    WorkflowExecutor executor;
    WorkflowExecutionReport report = executor.execute(wf, ctx);

    check(report.is_success(), "workflow succeeded after retrying");
    check(report.node_records.size() == 2, "report has 2 node records");

    const auto& flaky_rec = report.node_records[1];
    check(flaky_rec.node_id == "flaky", "second record is flaky node");
    check(flaky_rec.attempts == 3, "flaky node succeeded on 3rd attempt");
    check(flaky_rec.status == ExecutionStatus::SUCCESS, "flaky node status is SUCCESS");
}

void test_retry_exhausted_failure() {
    std::cout << "\n[test_retry_exhausted_failure]\n";

    Workflow wf("wf_retry_fail", "Retry Failure Workflow");
    wf.add_node(std::make_unique<TriggerNode>("start", "Start Node"));

    // Flaky node fails 5 times, but only allowed 2 retries (total 3 attempts)
    json flaky_config = {
        {"max_retries", 2},
        {"retry_delay_ms", 10}
    };
    wf.add_node(std::make_unique<FlakyNode>("flaky", "Always Failing Node", 5, flaky_config));
    wf.add_edge(Edge("e1", "start", "flaky"));

    ExecutionContext ctx;
    WorkflowExecutor executor;
    WorkflowExecutionReport report = executor.execute(wf, ctx);

    check(!report.is_success(), "workflow failed when retries exhausted");
    check(!report.error_message.empty(), "report contains error message");

    const auto& flaky_rec = report.node_records[1];
    check(flaky_rec.attempts == 3, "node attempted exactly 3 times before giving up");
    check(flaky_rec.status == ExecutionStatus::FAILURE, "final node status is FAILURE");
}

void test_execution_timing_records() {
    std::cout << "\n[test_execution_timing_records]\n";

    Workflow wf("wf_timing", "Timing Workflow");
    wf.add_node(std::make_unique<TriggerNode>("t1", "Trigger"));

    json action_config = {{"set_field", "result"}, {"value", 100}};
    wf.add_node(std::make_unique<ActionNode>("a1", "Action", action_config));
    wf.add_edge(Edge("e1", "t1", "a1"));

    ExecutionContext ctx;
    WorkflowExecutor executor;
    WorkflowExecutionReport report = executor.execute(wf, ctx);

    check(report.is_success(), "workflow executed successfully");
    check(report.total_duration_ms >= 0, "total duration is recorded");
    for (const auto& rec : report.node_records) {
        check(rec.duration_ms >= 0, "node duration is recorded: " + rec.node_id);
    }
}

int main() {
    std::cout << "=== Phase 4: Retries & Execution Reports Tests ===\n";

    test_retry_eventual_success();
    test_retry_exhausted_failure();
    test_execution_timing_records();

    std::cout << "\n=== Results ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}