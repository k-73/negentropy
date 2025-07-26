#pragma once
#include <pugixml.hpp>
#include <boost/pfr.hpp>
#include <magic_enum/magic_enum.hpp>
#include <type_traits>
#include <string>
#include <string_view>

// ---------- policies ----------
namespace XML::detail {
    template<class T>
    struct component_policy {
        // domyślne nazwy atrybutów: c0, c1, ...
        static std::string name(std::size_t i) { return "c" + std::to_string(i); }
    };

    // specjalizacja np. dla glm::vec*
    template<class T>
    concept HasLength = requires { T::length(); };

    template<class T>
    constexpr std::size_t comp_count() {
        if constexpr (HasLength<T>) return T::length();
        else return 0;
    }

    template<class T>
    concept Indexable = (comp_count<T>() > 0) && requires(T v) { v[0]; };

    // text-value writers
    template<class T>
    constexpr bool use_text_v =
        std::is_arithmetic_v<T> ||
        std::is_same_v<T, std::string> ||
        std::is_enum_v<T>;
}

// ---------- serialize / deserialize single field ----------
namespace XML {
    using namespace detail;

    template<class T>
    void serialize_field(pugi::xml_node& parent, std::string_view name, const T& field) {
        auto node = parent.append_child(name.data());

        if constexpr (std::is_enum_v<T>) {
            node.text().set(magic_enum::enum_name(field).data());
        } else if constexpr (std::is_arithmetic_v<T>) {
            node.text().set(field);
        } else if constexpr (std::is_same_v<T, std::string>) {
            node.text().set(field.c_str());
        } else if constexpr (Indexable<T>) {
            constexpr std::size_t N = comp_count<T>();
            for (std::size_t i = 0; i < N; ++i) {
                node.append_attribute(component_policy<T>::name(i).c_str()).set_value(field[i]);
            }
        } else {
            // fallback: recurse into aggregates
            auto_serialize(field, node);
        }
    }

    template<class T>
    void deserialize_field(const pugi::xml_node& parent, std::string_view name, T& field) {
        auto node = parent.child(name.data());
        if (!node) return;

        if constexpr (std::is_enum_v<T>) {
            if (auto v = magic_enum::enum_cast<T>(node.text().as_string())) field = *v;
        } else if constexpr (std::is_same_v<T, bool>) {
            field = node.text().as_bool();
        } else if constexpr (std::is_integral_v<T>) {
            field = static_cast<T>(node.text().as_llong());
        } else if constexpr (std::is_floating_point_v<T>) {
            field = static_cast<T>(node.text().as_double());
        } else if constexpr (std::is_same_v<T, std::string>) {
            field = node.text().as_string();
        } else if constexpr (Indexable<T>) {
            constexpr std::size_t N = comp_count<T>();
            for (std::size_t i = 0; i < N; ++i) {
                auto attr = node.attribute(component_policy<T>::name(i).c_str());
                if (attr) field[i] = static_cast<typename T::value_type>(attr.as_double());
            }
        } else {
            auto_deserialize(field, node);
        }
    }

    // ---------- aggregate helpers ----------
    template<typename T>
    void auto_serialize(const T& obj, pugi::xml_node& node) {
        boost::pfr::for_each_field(obj, [&]<typename F>(const F& f, std::size_t i) {
            constexpr auto names = boost::pfr::names_as_array<T>();
            serialize_field(node, names[i], f);
        });
    }

    template<typename T>
    void auto_deserialize(T& obj, const pugi::xml_node& node) {
        boost::pfr::for_each_field(obj, [&]<typename F>(F& f, std::size_t i) {
            constexpr auto names = boost::pfr::names_as_array<T>();
            deserialize_field(node, names[i], f);
        });
    }

    template<typename T>
    struct Serializable {
        void xml_serialize(pugi::xml_node& n)  const { auto_serialize(static_cast<const T&>(*this), n); }
        void xml_deserialize(const pugi::xml_node& n) { auto_deserialize(static_cast<T&>(*this), n); }
    };
}
