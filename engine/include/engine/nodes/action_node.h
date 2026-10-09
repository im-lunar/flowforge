#pragma once
#include "engine/workflow/node.h"

namespace engine {
    class ActionNode: public Node {

        public:
            ActionNode (std::string id, std::string name, json config = json::object()) 
                : Node(std::move(id), std::move(name), NodeType::ACTION, std::move(config))
            {}

            std::string type_name() const override {
                return "ActionNode";
            }

            ExecutionResult execute(ExecutionContext& context) override {
                // If the action is configured to set a field in context, record it
                if (config().contains("set_field") && config().contains("value")) {
                    std::string field = config()["set_field"];
                    context.set(field, config()["value"]);
                }

                // If action is configured as an alert, flag it in the context
                if (config().value("action_type", "") == "alert") {
                    context.set("alert_triggered", true);
                    if (config().contains("message")) {
                        context.set("alert_message", config()["message"]);
                    }
                }

                return ExecutionResult::success();
            }

    };
}