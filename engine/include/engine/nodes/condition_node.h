#pragma once
#include "engine/workflow/node.h"

namespace engine {

    class ConditionNode: public Node {

        public:
            ConditionNode (std::string id, std::string name, json config = json::object()) 
                : Node(std::move(id), std::move(name), NodeType::CONDITION, std::move(config))
            {}

            std::string type_name() const override {
                return "ConditionNode";
            }

            ExecutionResult execute(ExecutionContext& context) override {
                if (!config().contains("field") || !config().contains("operator") || !config().contains("value")) {
                    return ExecutionResult::failure("ConditionNode missing required configuration (field, operator, value)");
                }

                std::string field = config()["field"];
                if(!context.has(field)) {
                    return ExecutionResult::failure("Context missing required field: " + field);
                }

                const auto& actual_value = context.get(field);
                const auto& target_value = config()["value"];
                std::string op = config()["operator"];

                bool matched = false;

                if (op == ">") {
                    matched = (actual_value > target_value);
                } else if (op == "<") {
                    matched = (actual_value < target_value);
                } else if (op == ">=") {
                    matched = (actual_value >= target_value);
                } else if (op == "<=") {
                    matched = (actual_value <= target_value);
                } else if (op == "==") {
                    matched = (actual_value == target_value);
                } else if (op == "!=") {
                    matched = (actual_value != target_value);
                } else {
                    return ExecutionResult::failure("Unsupported operator in ConditionNode: " + op);
                }

                // return the branch to follow
                return ExecutionResult::success(matched ? "true" : "false");
            }
    };
}