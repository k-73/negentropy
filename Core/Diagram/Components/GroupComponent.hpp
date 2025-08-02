#pragma once

#include "Interface/Component.hpp"

namespace Diagram {
    class GroupComponent : public Component {
    public:
        GroupComponent(const std::string& name = "Group") {
            this->id = name;
        }

        template<typename... T>
        GroupComponent(const std::string& name, std::unique_ptr<T>... children) {
            this->id = name;
            (AddChild(std::move(children)), ...);
        }

        template<typename T>
        GroupComponent& Add(std::unique_ptr<T> child) {
            AddChild(std::move(child));
            return *this;
        }

        std::string GetTypeName() const override { return "GroupComponent"; }
    };

} // namespace Diagram
