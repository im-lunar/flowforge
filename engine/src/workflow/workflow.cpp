#include "engine/workflow/workflow.h"
#include <stdexcept>

namespace engine {

    Workflow::Workflow(std::string id, std::string name)
     : id_(std::move(id)),
     name_(std::move(name))
    {}

    void Workflow::add_node(std::unique_ptr<Node> node) {
        // check if null
        if (!node) {
            throw std::invalid_argument("Cannot add null node to workflow");
        }

        // check for duplicate id
        for (const auto& existing: nodes_) {
            if (existing->id() == node->id()) {
                throw std::invalid_argument("Duplicate node ID: "+ node->id());
            }
        }

        // Not Null or duplicate the add to the Node object
        nodes_.push_back(std::move(node));
    }

    void Workflow::add_edge(Edge edge) {
        // verify both endpoints exists
        if (!get_node(edge.source_node_id)) {
            throw std::invalid_argument("Source node not found: "+ edge.source_node_id);
        }
        if (!get_node(edge.target_node_id)) {
            throw std::invalid_argument("Target node not found: "+ edge.target_node_id);
        }

        edges_.push_back(std::move(edge));
    }

    Node* Workflow::get_node(const std::string& node_id) const{
        for (const auto& node: nodes_) {
            if (node->id() == node_id) {
                return node.get();
            }
        }
        return nullptr;
    }

    std::vector<Edge> Workflow::get_outgoing_edges(const std::string& node_id) const {
        std::vector<Edge> result;

        for (const auto& edge: edges_) {
            if (edge.source_node_id == node_id) {
                result.push_back(edge);
            }
        }

        return result;
    }

    Node* Workflow::find_start_node() const {
        // A start node has no incoming edges
        for (const auto& node: nodes_) {
            bool has_incoming = false;
            for (const auto& edge: edges_) {
                if (edge.target_node_id == node->id()) {
                    has_incoming = true;
                    break;
                }
            }

            if (!has_incoming) {
                return node.get();
            }
        }
        return nullptr;
    }

}