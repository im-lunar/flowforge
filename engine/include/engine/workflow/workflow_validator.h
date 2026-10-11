#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "engine/workflow/workflow.h"

namespace engine {

    struct ValidationResult {
        std::vector<std::string> errors;

        bool is_valid() const {
            return errors.empty();
        }

        void add_error(std::string error) {
            errors.push_back(std::move(error));
        }
    };

    class WorkflowValidator {
        public:
            // Runs all validation checks on the workflow
            ValidationResult validate(const Workflow& workflow);

        private: 
            // Check 1: Workflow must not be empty and must have exactly ONE start node
            Node* validate_start_node(const Workflow& workflow, ValidationResult& result);

            // Check 2: All nodes must be reachable from the start node
            void validate_reachability(const Workflow& workflow, Node* start_node, ValidationResult& result);

            // Check 3: Graph must be a DAG (no cycles) using 3-state DFS
            void validate_cycles(const Workflow& workflow, ValidationResult& result);

            enum class NodeState { UNVISITED, VISITING, VISITED };
            bool dfs_cycle_detect(const Workflow& workflow, 
                                  const std::string& current_id,
                                  std::unordered_map<std::string, NodeState>& state,
                                  std::string& cycle_node_id
                                 );

            // Check 4: Condition nodes must have appropriate outgoing branch edges
            void validate_condition_branches(const Workflow& workflow, ValidationResult& result);
    };
}