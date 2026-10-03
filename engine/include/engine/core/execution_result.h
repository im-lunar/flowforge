#pragma once

#include <string>
#include "engine/core/types.h"

namespace engine {

    struct ExecutionResult {
        ExecutionStatus status;
        std::string output_branch;
        std::string error_message;

        // General Constructor
        ExecutionResult (ExecutionStatus s, std::string branch = "", std::string error = "")
            : status(s),
            output_branch(branch),
            error_message(error)
        {}

        // Static helper function for a successful result
        static ExecutionResult success (std::string branch = "") {
            return ExecutionResult(ExecutionStatus::SUCCESS, std::move(branch), "");
        }

        // Static helper function for a failed result
        static ExecutionResult failure (std::string error) {
            return ExecutionResult(ExecutionStatus::FAILURE, "", std::move(error));
        }

        // Quick check helper
        bool is_success() const {
            return status == ExecutionStatus::SUCCESS;
        }
    };

}