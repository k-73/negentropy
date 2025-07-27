#pragma once

#include <memory>
#include <functional>
#include "../Diagram/Component.hpp"

class ComponentSelection {
public:
    using Component = Diagram::Component;
    using ComponentPtr = std::weak_ptr<Component>;
    using SelectionCallback = std::function<void(Component*)>;

    static ComponentSelection& Instance() noexcept {
        static ComponentSelection instance;
        return instance;
    }

    void Select(Component* component) noexcept {
        m_selected = component;
        if (m_onSelectionChanged) m_onSelectionChanged(component);
    }

    void Clear() noexcept { 
        m_selected = nullptr; 
        if (m_onSelectionChanged) m_onSelectionChanged(nullptr);
    }

    Component* GetSelected() const noexcept { return m_selected; }
    bool HasSelection() const noexcept { return m_selected != nullptr; }

    void SetSelectionCallback(SelectionCallback callback) noexcept {
        m_onSelectionChanged = std::move(callback);
    }

private:
    Component* m_selected = nullptr;
    SelectionCallback m_onSelectionChanged;
};