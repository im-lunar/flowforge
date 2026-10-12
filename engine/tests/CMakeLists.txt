add_executable(test_workflow test_workflow.cpp)
target_link_libraries(test_workflow PRIVATE workflow_engine)
add_test(NAME test_workflow COMMAND test_workflow)

# Phase 2 Execution tests:
add_executable(test_execution test_execution.cpp)
target_link_libraries(test_execution PRIVATE workflow_engine)
add_test(NAME test_execution COMMAND test_execution)

# Phase 3 Validation tests:
add_executable(test_validation test_validation.cpp)
target_link_libraries(test_validation PRIVATE workflow_engine)
add_test(NAME test_validation COMMAND test_validation)