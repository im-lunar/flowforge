#pragma once
#include <string>

namespace engine{

    // Identify which kind of Node it is
    enum NodeType {
        TRIGGER,
        CONDITION,
        ACTION
    };

    // Result of executing a single Node
    enum ExecutionStatus {
        SUCCESS,
        FAILURE,
        PENDING,
        SKIPPED
    };

    // Inline Function to convert NodeType to String
    inline std::string NodeType_to_String (NodeType type) {
        switch (type)
        {
        case NodeType::TRIGGER: return "TRIGGER";
        case NodeType::CONDITION: return "CONDITION";
        case NodeType::ACTION: return "ACTION";

        default:
            return "UNKNOWN";
        }
    }
}