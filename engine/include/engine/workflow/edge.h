#pragma once 

#include <string>

namespace engine {

    struct Edge {
        std::string id;
        std::string source_node_id;
        std::string target_node_id;

        // For conditional branching: 
        // "true"/"false" means conditional nodes
        // "" means unconditional (always follow this edge)
        std::string conditional_branch;

        Edge(std::string id, std::string source, std::string target, std::string branch = "")
            : id(std::move(id)),
            source_node_id(std::move(source)),
            target_node_id(std::move(target)),
            conditional_branch(std::move(branch))
        {}

    };
}