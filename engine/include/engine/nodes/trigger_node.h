#pragma once
#include "engine/workflow/node.h"

namespace engine {

    class TriggerNode: public Node {
        public:
            // Constructor
            TriggerNode (std::string id, std::string name, json config = json::object()) 
                : Node(std::move(id), std::move(name), NodeType::TRIGGER, std::move(config))
            {}

            std::string type_name() const override {
                return "TriggerNode";
            }

            // Actual work happens here
            ExecutionResult execute(ExecutionContext& context) override {

                // If the trigger has a payload defined in its config, load it into context
                if(config().contains("payload") && config()["payload"].is_object()) {
                    for (const auto& [key, value] : config()["payload"].items()) {
                        context.set(key, value);
                    }
                }

                return ExecutionResult::success();
            }
    };
}