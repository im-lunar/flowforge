#include "engine/execution/workflow_executor.h"

namespace engine {

    WorkflowExecutor::WorkflowExecutor(size_t max_steps) 
        : max_steps_(max_steps)
    {};

    ExecutionResult WorkflowExecutor::execute(const Workflow& workflow, ExecutionContext& context) {
        // Step 1: Find entry point of the workflow
        Node* current_node = workflow.find_start_node();
        if (!current_node) {
            return ExecutionResult::failure("Workflow has no start node (entry point)");
        }

        // Step 2: Traverse and execute
        size_t steps = 0;

        while (current_node != nullptr) {
            if (++steps > max_steps_) {
                return ExecutionResult::failure("Execution step limit exceeded (infinite loop detected)");
            }

            // Run the current node
            ExecutionResult result = current_node->execute(context);
            if (!result.is_success()) {
                return ExecutionResult::failure("Node '" + current_node->id() + "' failed: " + result.error_message);
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
                return ExecutionResult::failure("Target node not found: " + next_edge->target_node_id);
            }
        }
            
        return ExecutionResult::success(); 
    }
}