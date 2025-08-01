#pragma once
#include <pugixml.hpp>
#include <boost/pfr.hpp>
#include <magic_enum/magic_enum.hpp>
#include <type_traits>
#include <string_view>
#include <glm/glm.hpp>

namespace XML::detail {
    template<class T>
    struct component_policy {
        static std::string name(std::size_t i) { return "c" + std::to_string(i); }
    };

    template<int L, typename T, glm::qualifier Q>
    struct component_policy<glm::vec<L, T, Q>> {
        static std::string name(std::size_t i) {
            switch (i) {
                case 0: return "x";
                case 1: return "y";
                case 2: return "z";
                case 3: return "w";
                default: return "c" + std::to_string(i);
            }
        }
    };

    template<class T>
    concept HasLength = requires { T::length(); };

    template<class T>
    constexpr std::size_t comp_count() {
        if constexpr (HasLength<T>) return T::length();
        else return 0;
    }

    template<class T>
    concept Indexable = (comp_count<T>() > 0) && requires(T v) { v[0]; };

    template<class T>
    constexpr bool use_text_v =
        std::is_arithmetic_v<T> ||
        std::is_same_v<T, std::string> ||
        std::is_enum_v<T>;
}

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
                if (auto attr = node.attribute(component_policy<T>::name(i).c_str())) {
                    field[i] = static_cast<std::remove_reference_t<decltype(field[i])>>(attr.as_double());
                }
            }
        } else {
            auto_deserialize(field, node);
        }
    }

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
        void XmlSerialize(pugi::xml_node& n)  const { auto_serialize(static_cast<const T&>(*this), n); }
        void XmlDeserialize(const pugi::xml_node& n) { auto_deserialize(static_cast<T&>(*this), n); }
    };
}
