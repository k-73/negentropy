#pragma once

#include <pugixml.hpp>
#include <concepts>
#include <type_traits>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <magic_enum/magic_enum.hpp>

namespace XML {
    template<typename T>
    concept XMLSerializable = requires(T& obj, const pugi::xml_node& node) {
        obj.serialize_to_xml(node);
        obj.deserialize_from_xml(node);
    };

    template<typename T>
    concept EnumType = std::is_enum_v<T>;

    template<typename T>
    concept ArithmeticType = std::is_arithmetic_v<T>;

    // Core serialization functions for basic types
    template<ArithmeticType T>
    void serialize(pugi::xml_node& node, const char* name, const T& value) {
        node.append_attribute(name).set_value(value);
    }

    template<ArithmeticType T>
    void deserialize(const pugi::xml_node& node, const char* name, T& value) {
        if (auto attr = node.attribute(name)) {
            if constexpr (std::is_same_v<T, float>) value = attr.as_float();
            else if constexpr (std::is_same_v<T, double>) value = attr.as_double();
            else if constexpr (std::is_same_v<T, int>) value = attr.as_int();
            else if constexpr (std::is_same_v<T, bool>) value = attr.as_bool();
        }
    }

    // String specialization
    inline void serialize(pugi::xml_node& node, const char* name, const std::string& value) {
        node.append_child(name).text().set(value.c_str());
    }

    inline void deserialize(const pugi::xml_node& node, const char* name, std::string& value) {
        if (auto child = node.child(name)) {
            value = child.text().as_string();
        }
    }

    // Enum specialization using magic_enum
    template<EnumType T>
    inline void serialize(pugi::xml_node& node, const char* name, const T& value) {
        node.append_attribute(name).set_value(magic_enum::enum_name(value).data());
    }

    template<EnumType T>
    inline void deserialize(const pugi::xml_node& node, const char* name, T& value) {
        if (auto attr = node.attribute(name)) {
            value = magic_enum::enum_cast<T>(attr.as_string()).value_or(T{});
        }
    }

    // GLM vec2 specialization
    inline void serialize(pugi::xml_node& node, const char* name, const glm::vec2& value) {
        auto child = node.append_child(name);
        serialize(child, "x", value.x);
        serialize(child, "y", value.y);
    }

    inline void deserialize(const pugi::xml_node& node, const char* name, glm::vec2& value) {
        if (auto child = node.child(name)) {
            deserialize(child, "x", value.x);
            deserialize(child, "y", value.y);
        }
    }

    // GLM vec4 specialization
    inline void serialize(pugi::xml_node& node, const char* name, const glm::vec4& value) {
        auto child = node.append_child(name);
        serialize(child, "r", value.r);
        serialize(child, "g", value.g);
        serialize(child, "b", value.b);
        serialize(child, "a", value.a);
    }

    inline void deserialize(const pugi::xml_node& node, const char* name, glm::vec4& value) {
        if (auto child = node.child(name)) {
            deserialize(child, "r", value.r);
            deserialize(child, "g", value.g);
            deserialize(child, "b", value.b);
            deserialize(child, "a", value.a);
        }
    }

    // CRTP base for automatic serialization
    template<typename Derived>
    class Serializable {
    public:
        void serialize_to_xml(pugi::xml_node& node) const {
            static_cast<const Derived*>(this)->xml_serialize(node);
        }

        void deserialize_from_xml(const pugi::xml_node& node) {
            static_cast<Derived*>(this)->xml_deserialize(node);
        }
    };

    // Macro for easy field registration
    #define XML_FIELD(node, field) \
        XML::serialize(node, #field, field)

    #define XML_FIELD_LOAD(node, field) \
        XML::deserialize(node, #field, field)
}