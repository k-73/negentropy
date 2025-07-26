#pragma once

#include <pugixml.hpp>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <magic_enum/magic_enum.hpp>
#include <boost/pfr.hpp>
#include <type_traits>
#include <string_view>

namespace XML {
    template<typename T>
    concept IsGlmVec = std::is_same_v<T, glm::vec2> || std::is_same_v<T, glm::vec3> || std::is_same_v<T, glm::vec4>;

    template<typename T>
    void serialize_field(pugi::xml_node& parent, std::string_view name, const T& field) {
        auto node = parent.append_child(name.data());
        if constexpr (std::is_enum_v<T>) {
            node.text().set(magic_enum::enum_name(field).data());
        } else if constexpr (std::is_arithmetic_v<T>) {
            node.text().set(field);
        } else if constexpr (std::is_same_v<T, std::string>) {
            node.text().set(field.c_str());
        } else if constexpr (IsGlmVec<T>) {
            bool isColor = (name == "color");
            if constexpr (T::length() >= 1) {
                node.append_attribute(isColor ? "r" : "x").set_value(field.x);
            }
            if constexpr (T::length() >= 2) {
                node.append_attribute(isColor ? "g" : "y").set_value(field.y);
            }
            if constexpr (T::length() >= 3) {
                node.append_attribute(isColor ? "b" : "z").set_value(field.z);
            }
            if constexpr (T::length() >= 4) {
                node.append_attribute(isColor ? "a" : "w").set_value(field.w);
            }
        }
    }

    template<typename T>
    void deserialize_field(const pugi::xml_node& parent, std::string_view name, T& field) {
        auto node = parent.child(name.data());
        if (!node) return;

        if constexpr (std::is_enum_v<T>) {
            auto value = magic_enum::enum_cast<T>(node.text().as_string());
            if (value.has_value()) {
                field = value.value();
            }
        } else if constexpr (std::is_same_v<T, bool>) {
            field = node.text().as_bool();
        } else if constexpr (std::is_integral_v<T>) {
            field = node.text().as_llong();
        } else if constexpr (std::is_floating_point_v<T>) {
            field = node.text().as_double();
        } else if constexpr (std::is_same_v<T, std::string>) {
            field = node.text().as_string();
        } else if constexpr (IsGlmVec<T>) {
            bool isColor = (name == "color");
            if constexpr (T::length() >= 1) {
                field.x = node.attribute(isColor ? "r" : "x").as_float();
            }
            if constexpr (T::length() >= 2) {
                field.y = node.attribute(isColor ? "g" : "y").as_float();
            }
            if constexpr (T::length() >= 3) {
                field.z = node.attribute(isColor ? "b" : "z").as_float();
            }
            if constexpr (T::length() >= 4) {
                field.w = node.attribute(isColor ? "a" : "w").as_float();
            }
        }
    }

    template<typename T>
    void auto_serialize(const T& obj, pugi::xml_node& node) {
        boost::pfr::for_each_field(obj, [&]<typename FieldType>(const FieldType& field, size_t idx) {
            constexpr auto names = boost::pfr::names_as_array<T>();
            serialize_field(node, names[idx], field);
        });
    }

    template<typename T>
    void auto_deserialize(T& obj, const pugi::xml_node& node) {
        boost::pfr::for_each_field(obj, [&]<typename FieldType>(FieldType& field, size_t idx) {
            constexpr auto names = boost::pfr::names_as_array<T>();
            deserialize_field(node, names[idx], field);
        });
    }

    template<typename T>
    struct Serializable {
        void xml_serialize(pugi::xml_node& node) const {
            auto_serialize(static_cast<const T&>(*this), node);
        }

        void xml_deserialize(const pugi::xml_node& node) {
            auto_deserialize(static_cast<T&>(*this), node);
        }
    };
}