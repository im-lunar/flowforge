#include "engine/execution/workflow_executor.h"
#include "engine/workflow/workflow_validator.h"
#include "engine/core/execution_report.h"

#include <chrono>
#include <thread>

namespace engine {

    WorkflowExecutor::WorkflowExecutor(size_t max_steps) 
        : max_steps_(max_steps)
    {};

    WorkflowExecutionReport WorkflowExecutor::execute(const Workflow& workflow, ExecutionContext& context) {

        // Workflow start timestamp for WorkflowExecutionReport
        auto start = std::chrono::steady_clock::now();
        std::vector<NodeExecutionRecord> node_records;

        auto make_failure = [&](const std::string& msg) -> WorkflowExecutionReport {
            auto end = std::chrono::steady_clock::now();
            long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            WorkflowExecutionReport r;
            r.overall_status = ExecutionStatus::FAILURE;
            r.error_message = msg;
            r.total_duration_ms = ms;
            r.node_records = node_records;

            return r;
        };

        // Step 0: Pre-execution Gra ph Validation
        WorkflowValidator validator;
        ValidationResult val_result = validator.validate(workflow);

        if (!val_result.is_valid()) {
            std::string error_summary = "Workflow validation failed: ";
            for (size_t i = 0; i < val_result.errors.size(); ++i) {
                error_summary += val_result.errors[i] + (i + 1 < val_result.errors.size() ? "; " : "");
            }
            return make_failure(error_summary);
        }

        // Step 1: Find entry point of the workflow
        Node* current_node = workflow.find_start_node();
        if (!current_node) {
            return make_failure("Workflow has no start node (entry point)");
        }

        // Step 2: Traverse and execute
        size_t steps = 0;

        while (current_node != nullptr) {
            if (++steps > max_steps_) {
                return make_failure("Execution step limit exceeded (infinite loop detected)");
            }

            //  1. Read retry config (defaults: 0 retries, 100ms delay)
            int max_retries = current_node->config().value("max_retries", 0);
            int retry_delay_ms = current_node->config().value("retry_delay_ms", 100);

            // Total tries allowed = 1 initial try + max_retries
            int total_attempts = 1 + max_retries;
            int attempts_taken = 0;

            ExecutionResult result = ExecutionResult::failure("Not executed");

            auto node_start = std::chrono::steady_clock::now();

            // retry loop
            for (int attempt = 1; attempt <= total_attempts; ++attempt) {
                attempts_taken = attempt;
                result = current_node->execute(context);

                if (result.is_success()) {
                    break;
                }

                if (attempt < total_attempts) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
                    retry_delay_ms *= 2;
                }
            }

            auto node_end = std::chrono::steady_clock::now();
            long node_duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(node_end - node_start).count();

            // Record this node's execution
            NodeExecutionRecord record;
            record.node_id = current_node->id();
            record.status = result.status;
            record.error_message = result.error_message;
            record.duration_ms = node_duration_ms;
            record.attempts = attempts_taken;

            node_records.push_back(std::move(record));

            // If it failed even after retries, halt the workflow
            if (!result.is_success()) {
                return make_failure("Node '" + current_node->id() + "' failed: " + result.error_message);
            }

            // Step 3: Find outgoing edges from this node
            auto outgoing_edges = workflow.get_outgoing_edges(current_node->id());
            if (outgoing_edges.empty()) {
                // Terminal node reached - workflow completed sucessfully
                break;
            }

            // Step 4: Determine which edge to traverse
            const Edge* next_edge = nullptr;
            
            if (!result.output_branch.empty()) {
                // This was a branching node (e.g. ConditionNode returning "true" or "false")
                for (const auto& edge : outgoing_edges) {
                    if (edge.conditional_branch == result.output_branch) {
                        next_edge = &edge;
                        break;
                    }
                }

            } else {
                // Non-branching node: pick the unconditional edge (empty conditional_branch)
                for (const auto& edge : outgoing_edges) {
                    if (edge.conditional_branch.empty()) {
                        next_edge = &edge;
                        break;
                    }
                }

                // Fallback: if no edge is marked "", take the first outgoing edge
                if (!next_edge && !outgoing_edges.empty()) {
                    next_edge = &outgoing_edges[0];
                }
            }

            if (!next_edge) {
                // No matching path for this branch (e.g. condition false with no false edge)
                // Workflow halts safely
                break;
            }

            // Move to the next node in the graph
            current_node = workflow.get_node(next_edge->target_node_id);
            if (!current_node) {
                return make_failure("Target node not found: " + next_edge->target_node_id);
            }
        }

        auto end = std::chrono::steady_clock::now();
        long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        WorkflowExecutionReport report;
        report.overall_status = ExecutionStatus::SUCCESS;
        report.total_duration_ms = ms;
        report.node_records = std::move(node_records);
        report.error_message = "";
        // report.node_records  ← will fill this in next step
        return report;
         
    }
}