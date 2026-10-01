#pragma once

#include "engine/workflow/node.h"
#include "engine/workflow/edge.h"
#include <string>
#include <vector>
#include <memory>


namespace engine{
    
    class Workflow {
        
        public:
            Workflow(std::string id, std::string name);

            // Add a node, Workflow takes ownership
            void add_node(std::unique_ptr<Node> node);

            // Add edge between two nodes
            void add_edge(Edge edge);

            // Lookup
            Node* get_node(const std::string& node_id) const;
            std::vector<Edge> get_outgoing_edges(const std::string& node_id) const;

            // Finding the start node
            Node* find_start_node() const;

            // Getters
            const std::string& id() const { return id_; }
            const std::string& name() const { return name_; }
            const std::vector<std::unique_ptr<Node>>& nodes() const { return nodes_; }
            const std::vector<Edge>& edges() const { return edges_; }

        private:
            std::string id_;
            std::string name_;
            std::vector<std::unique_ptr<Node>> nodes_;
            std::vector<Edge> edges_;
    };
}