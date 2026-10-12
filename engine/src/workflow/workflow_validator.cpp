#include "engine/workflow/workflow_validator.h"
#include <unordered_set>
#include <queue>

namespace engine{

    ValidationResult WorkflowValidator::validate(const Workflow& workflow) {
        ValidationResult result;

        // Run Check 1: Start node
        Node* start_node = validate_start_node(workflow, result);

        // Run Check 2: Reachability (only if a start node was found)
        if (start_node) {
            validate_reachability(workflow, start_node, result);
        }

        // Run Check 3: Cycle detection (DAG check)
        validate_cycles(workflow,result);

        // Run Check 4: Condition branching completeness
        validate_condition_branches(workflow, result);

        return result;
    }

    Node* WorkflowValidator::validate_start_node(const Workflow& workflow, ValidationResult& result) {
        if (workflow.nodes().empty()) {
            result.add_error("Workflow contains no nodes");
            return nullptr;
        }

        // Count incoming edges for each node (in-degree)
        std::vector<Node*> start_nodes;
        for (const auto& node : workflow.nodes()) {
            bool has_incoming = false;
            for (const auto& edge: workflow.edges()) {
                if (edge.target_node_id == node->id()) {
                    has_incoming = true;
                    break;
                }
            }
            if (!has_incoming) {
                start_nodes.push_back(node.get());
            }
        }

        if (start_nodes.empty()) {
            result.add_error("Workflow has no start node (entry point with in-degree 0)");
            return nullptr;
        }

        if (start_nodes.size() > 1) {
            std::string msg = "Workflow has multiple start nodes (ambiguous entry points): ";
            for (size_t i=0; i<start_nodes.size(); i++) {
                msg += start_nodes[i]->id() + (i + 1 < start_nodes.size() ? ", " : "");
            }
            result.add_error(msg);
            return start_nodes[0];
        }

        return start_nodes[0];
    }

    void WorkflowValidator::validate_reachability(const Workflow& workflow, Node* start_node, ValidationResult& result) {
        std::unordered_set<std::string> visited;
        std::queue<std::string> queue;

        queue.push(start_node->id());
        visited.insert(start_node->id());

        // BFS Traversal
        while (!queue.empty()) {
            std::string current_id = queue.front();
            queue.pop();

            for (const auto& edge: workflow.get_outgoing_edges(current_id)) {
                if (visited.find(edge.target_node_id) == visited.end()) {
                    visited.insert(edge.target_node_id);
                    queue.push(edge.target_node_id);
                }
            }
        }

        // Check for any node in the graph that was never visited
        for (const auto& node : workflow.nodes()) {
            if (visited.find(node->id()) == visited.end()) {
                result.add_error("Unreachable node detected: '" + node->id() + "'");
            }
        }
    }

    void WorkflowValidator::validate_cycles(const Workflow& workflow, ValidationResult& result) {
        std::unordered_map<std::string, NodeState> state;

        for (const auto& node : workflow.nodes()) {
            state[node->id()] = NodeState::UNVISITED;
        }

        std::string cycle_node_id;
        for (const auto& node: workflow.nodes()) {
            if (state[node->id()] == NodeState::UNVISITED) {
                if (dfs_cycle_detect(workflow, node->id(), state, cycle_node_id)) {
                    result.add_error("Cycle detected involving node '" + cycle_node_id + "'");
                    return; // Reporting one cycle is sufficient
                }
            }
        }
    }

    bool WorkflowValidator::dfs_cycle_detect(const Workflow& workflow, const std::string& current_id, std::unordered_map<std::string, NodeState>& state, std::string& cycle_node_id) {

        state[current_id] = NodeState::VISITING;

        for (const auto& edge : workflow.get_outgoing_edges(current_id)) {

            const std::string& target_id = edge.target_node_id;
            if (state[target_id] == NodeState::VISITING) {
                // Back-edge found! We reached an ancestor on the current DFS call stack.
                cycle_node_id = target_id;
                return true;
            }
            if (state[target_id] == NodeState::UNVISITED) {
                if (dfs_cycle_detect(workflow, target_id, state, cycle_node_id)) {
                    return true;
                }
            }
        }
        state[current_id] = NodeState::VISITED;
        return false;
    }

    void WorkflowValidator::validate_condition_branches(const Workflow& workflow, ValidationResult& result) {
        for (const auto& node : workflow.nodes()) {
            if (node->type() == NodeType::CONDITION) {
                auto outgoing = workflow.get_outgoing_edges(node->id());
                if (outgoing.empty()) {
                    result.add_error("Condition node '" + node->id() + "' has no outgoing branch edges");
                }
            }
        }
    }
}