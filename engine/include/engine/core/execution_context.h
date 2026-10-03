#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <stdexcept>

namespace engine {

    using json = nlohmann::json;

    class ExecutionContext {
        
        public:
            ExecutionContext (): data_(json::object()) {};

            explicit ExecutionContext(json initial_data) {
                if (initial_data.is_null()) {
                    data_ = json::object();
                } else if (!initial_data.is_object()) {
                    throw std::invalid_argument("ExecutionContext must be initialized with a JSON object");
                } else {
                    data_ = std::move(initial_data);
                }
            }

            // Store or overwrite a value for a given key
            void set(std::string& key, json value) {
                data_[key] = std::move(value);
            }

            // Check whether a key exists in context
            bool has(const std::string& key) const {
                return data_.contains(key);
            }

            // Retrieve value for a key; throws if key does not exist
            const json& get(const std::string& key) const{
                if(!has(key)) {
                    throw std::out_of_range("Key not found in ExecutionContext: " + key);
                }
                return data_.at(key);
            }

            // Read-only access to the entire data payload (useful for logging/debugging/serialization)
            const json& data() const {
                return data_;
            }

        private:
            json data_;
    };
}