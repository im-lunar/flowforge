#pragma once

#include "engine/workflow/workflow.h"
#include "engine/core/execution_context.h"
#include "engine/core/execution_result.h"
#include "engine/core/execution_report.h"

namespace engine {

    class WorkflowExecutor {
        public: 
            explicit WorkflowExecutor(size_t max_steps=1000);

            // Execute a workflow sequentially from its start node
            WorkflowExecutionReport execute(const Workflow& workflow, ExecutionContext& context);

        private:
            size_t max_steps_;
    };
}