#pragma once

#include <pugixml.hpp>
#include <concepts>
#include <type_traits>
#include <string>
#include <tuple>
#include <iostream>
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

    template<typename T>
    concept StructuredBindable = requires(T& obj) {
        std::tuple_size<std::remove_cvref_t<T>>::value;
    };

    // Core serialization for basic types
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

    // Enum specialization
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

    // Helper to extract value from reference_wrapper or pass through
    template<typename T>
    decltype(auto) get_value(std::reference_wrapper<T> ref) {
        return ref.get();
    }

    template<typename T>
    decltype(auto) get_value(T&& value) {
        return std::forward<T>(value);
    }

    // Modern C++20 structured serialization using fold expressions
    template<typename... Args>
    inline void serialize_node(pugi::xml_node& parent, const char* name, Args&&... args) {
        auto child = parent.append_child(name);
        ((serialize(child, args.first, get_value(args.second))), ...);
    }

    template<typename... Args>
    inline void deserialize_node(const pugi::xml_node& parent, const char* name, Args&&... args) {
        if (auto child = parent.child(name)) {
            ((deserialize(child, args.first, get_value(args.second))), ...);
        }
    }

    // Field factory for cleaner syntax - fixed for deserialize
    template<typename T>
    constexpr auto f(const char* name, T& value) {
        return std::make_pair(name, std::ref(value));
    }

    // Const version for serialize
    template<typename T>
    constexpr auto f(const char* name, const T& value) {
        return std::make_pair(name, std::cref(value));
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

    // Ultra-compact macros for structured serialization
    #define XML_NODE(node, name, ...) \
        XML::serialize_node(node, name, __VA_ARGS__)

    #define XML_NODE_LOAD(node, name, ...) \
        XML::deserialize_node(node, name, __VA_ARGS__)
}