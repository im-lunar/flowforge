#pragma once

#include "engine/core/types.h"
#include <string>
#include <nlohmann/json.hpp>

namespace engine {

    using json = nlohmann::json;

    class Node {
        public: 
            Node(std::string id, std::string name, NodeType type, json config = json::object())
            : id_(std::move(id)),
            name_(std::move(name)),
            type_(std::move(type)),
            config_(std::move(config))
            {}

            // virtual destructor
            virtual ~Node() = default;

            // Prevent copying - nodes are owned by unique_ptr, copying would be a bug
            Node(const Node&) = delete;
            Node& operator = (const Node&) = delete;

            // Allow moving
            Node(Node&&) = default;
            Node& operator = (Node&&) = default;

            // Accessors
            const std::string& id() const { return id_; }
            const std::string& name() const { return name_; }
            NodeType type() const { return type_; }
            const json& config() const { return config_; }

            // return the specific type name
            // each derived class overrides this
            virtual std::string type_name() const = 0;

        private:
            std::string id_;
            std::string name_;
            NodeType type_;
            json config_;
};
}