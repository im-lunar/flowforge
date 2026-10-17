#pragma once

#include <string>
#include <vector>
#include "engine/core/types.h"

namespace engine {

    struct NodeExecutionRecord
    {
        std::string node_id;
        ExecutionStatus status;
        std::string error_message;
        long duration_ms;
        int attempts;
    };

    struct WorkflowExecutionReport {
        ExecutionStatus overall_status;
        std::vector<NodeExecutionRecord> node_records;
        long total_duration_ms;
        std::string error_message;

        // helper so callers can easily check if the run succeeded!
        bool is_success() {
            return overall_status == ExecutionStatus::SUCCESS;
        }
    };
    
}