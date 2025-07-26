#pragma once

#include <vector>
#include <memory>
#include <SDL.h>
#include "DiagramElement.hpp"
#include "Camera.hpp"

namespace Diagram {
    class DiagramManager {
    public:
        using ElementPtr = std::unique_ptr<DiagramElement>;
        
        void HandleEvent(const SDL_Event& event, Camera& camera) noexcept;
        void Render(SDL_Renderer* renderer, const Camera& camera) const noexcept;
        
        void AddElement(ElementPtr element);
        void RemoveElement(size_t index);
        void Clear();
        
        [[nodiscard]] size_t GetElementCount() const noexcept { return m_elements.size(); }
        [[nodiscard]] DiagramElement* GetElement(size_t index) const noexcept;
        
        void xml_serialize(pugi::xml_node& node) const;
        void xml_deserialize(const pugi::xml_node& node);
        
    private:
        std::vector<ElementPtr> m_elements;
    };
}