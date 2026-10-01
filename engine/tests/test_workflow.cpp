#include <iostream>
#include <memory>
#include <cassert>
#include "engine/workflow/workflow.h"
#include "engine/workflow/edge.h"
#include "engine/workflow/node.h"

using namespace engine;

class TestNode : public Node {
    public:
        TestNode(std::string id, std::string name, NodeType type, json config = json::object())
            : Node(std::move(id), std::move(name), type, std::move(config)) 
        {}

        std::string type_name() const override { return "TestNode"; } 
};

// ---------Helper Function-------------------------------------------------
int tests_passed = 0;
int tests_failed = 0;

void check(bool condition, const std::string& test_name) {
    if (condition) {
        std::cout << "  PASS: " << test_name << "\n";
        tests_passed++;
    } else {
        std::cout << "  FAIL: " << test_name << "\n";
        tests_failed++;
    }
} 

// -------Tests--------------------------------------------------------------------------------

void test_node_creation() {
    std::cout << "\n[test_node_creation]\n";

    auto node = std::make_unique<TestNode>("n1", "Sensor Trigger", NodeType::TRIGGER);
    
    check(node->id() == "n1", "node ID is correct");
    check(node->name() == "Sensor Trigger", "node name is correct");
    check(node->type() == NodeType::TRIGGER, "node type is TRIGGER");
    check(node->type_name() == "TestNode", "type_name returns derived class value");
    check(node->config().is_object(), "default config is empty JSON object");
}

void test_node_with_config() {
    std::cout << "\n[test_node_with_config]\n";

    json config = {
        {"field", "temperature"},
        {"operator", ">"},
        {"value", 90}
    };
    auto node = std::make_unique<TestNode>("n2", "Temp Check", NodeType::CONDITION, config);

    check(node->config()["field"] == "temperature", "config field is correct");
    check(node->config()["value"] == 90, "config value is correct");
}

void test_workflow_add_nodes() {
    std::cout << "\n[test_workflow_add_nodes]\n";

    Workflow wf("wf1", "Test Workflow");

    wf.add_node(std::make_unique<TestNode>("n1", "Start", NodeType::TRIGGER));
    wf.add_node(std::make_unique<TestNode>("n2", "Check", NodeType::CONDITION));
    wf.add_node(std::make_unique<TestNode>("n3", "Alert", NodeType::ACTION));

    check(wf.nodes().size() == 3, "workflow has 3 nodes");
    check(wf.get_node("n1") != nullptr, "can find node n1");
    check(wf.get_node("n2") != nullptr, "can find node n2");
    check(wf.get_node("n999") == nullptr, "unknown node returns nullptr");
}

void test_workflow_duplicate_node_id() {
    std::cout << "\n[test_workflow_duplicate_node_id]\n";

    Workflow wf("wf1", "Test");

    wf.add_node(std::make_unique<TestNode>("n1", "First", NodeType::TRIGGER));

    bool threw = false;
    try {
        wf.add_node(std::make_unique<TestNode>("n1", "Duplicate", NodeType::ACTION));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "duplicate node ID throws exception");
}

void test_workflow_edges() {
    std::cout << "\n[test_workflow_edges]\n";

    Workflow wf("wf1", "Test");

    wf.add_node(std::make_unique<TestNode>("n1", "Start", NodeType::TRIGGER));
    wf.add_node(std::make_unique<TestNode>("n2", "Check", NodeType::CONDITION));
    wf.add_node(std::make_unique<TestNode>("n3", "Alert", NodeType::ACTION));
    wf.add_node(std::make_unique<TestNode>("n4", "End", NodeType::ACTION));

    // n1 -> n2 (unconditional)
    wf.add_edge(Edge("e1", "n1", "n2"));
    // n2 -> n3 (true branch), n2 -> n4 (false branch)
    wf.add_edge(Edge("e2", "n2", "n3", "true"));
    wf.add_edge(Edge("e3", "n2", "n4", "false"));

    check(wf.edges().size() == 3, "workflow has 3 edges");
    auto outgoing = wf.get_outgoing_edges("n2");
    check(outgoing.size() == 2, "condition node has 2 outgoing edges");
    auto terminal_edges = wf.get_outgoing_edges("n3");
    check(terminal_edges.size() == 0, "terminal node has 0 outgoing edges");
}

void test_workflow_invalid_edge() {
    std::cout << "\n[test_workflow_invalid_edge]\n";

    Workflow wf("wf1", "Test");

    wf.add_node(std::make_unique<TestNode>("n1", "Start", NodeType::TRIGGER));
    bool threw = false;
    try {
        wf.add_edge(Edge("e1", "n1", "n999"));  // n999 doesn't exist
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "edge to nonexistent node throws exception");
}

void test_find_start_node() {
    std::cout << "\n[test_find_start_node]\n";

    Workflow wf("wf1", "Test");

    wf.add_node(std::make_unique<TestNode>("n1", "Start", NodeType::TRIGGER));
    wf.add_node(std::make_unique<TestNode>("n2", "Middle", NodeType::CONDITION));
    wf.add_node(std::make_unique<TestNode>("n3", "End", NodeType::ACTION));
    wf.add_edge(Edge("e1", "n1", "n2"));
    wf.add_edge(Edge("e2", "n2", "n3"));

    Node* start = wf.find_start_node();

    check(start != nullptr, "start node found");
    check(start->id() == "n1", "start node is n1 (no incoming edges)");
}

void test_branching_workflow() {
    std::cout << "\n[test_branching_workflow]\n";

    // Build:  Sensor -> Condition --(true)--> Alert
    //                            --(false)--> End

    Workflow wf("wf1", "Overheat Detection");

    json condition_config = {
        {"field", "temperature"},
        {"operator", ">"},
        {"value", 90}
    };
    wf.add_node(std::make_unique<TestNode>("sensor", "Machine Sensor", NodeType::TRIGGER));
    wf.add_node(std::make_unique<TestNode>("check", "Temp Check", NodeType::CONDITION, condition_config));
    wf.add_node(std::make_unique<TestNode>("alert", "Create Alert", NodeType::ACTION));
    wf.add_node(std::make_unique<TestNode>("end", "Continue", NodeType::ACTION));

    wf.add_edge(Edge("e1", "sensor", "check"));
    wf.add_edge(Edge("e2", "check", "alert", "true"));
    wf.add_edge(Edge("e3", "check", "end", "false"));

    // Verify structure
    check(wf.find_start_node()->id() == "sensor", "workflow starts at sensor");
    check(wf.get_outgoing_edges("sensor").size() == 1, "sensor has 1 outgoing edge");
    check(wf.get_outgoing_edges("check").size() == 2, "condition has 2 branches");
    check(wf.get_outgoing_edges("alert").size() == 0, "alert is terminal");
    check(wf.get_outgoing_edges("end").size() == 0, "end is terminal");

    // Verify condition config stored correctly
    Node* check_node = wf.get_node("check");
    check(check_node->config()["field"] == "temperature", "condition field is temperature");
    check(check_node->config()["operator"] == ">", "condition operator is >");
    check(check_node->config()["value"] == 90, "condition threshold is 90");
}


//----------Main-----------------------------------------------------------------------
int main () {
    std::cout << "===Worflow Engine Test==== \n";

    test_node_creation();
    test_node_with_config();
    test_workflow_add_nodes();
    test_workflow_duplicate_node_id();
    test_workflow_edges();
    test_workflow_invalid_edge();
    test_find_start_node();
    test_branching_workflow();

    std::cout << "\n=== Results ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}