#include "DiagramManager.hpp"
#include "Block.hpp"
#include <algorithm>

namespace Diagram {
    void DiagramManager::HandleEvent(const SDL_Event& event, Camera& camera) noexcept {
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            for (int i = static_cast<int>(m_elements.size()) - 1; i >= 0; --i) {
                if (m_elements[i]->HandleEvent(event, camera)) {
                    std::rotate(m_elements.begin() + i, m_elements.begin() + i + 1, m_elements.end());
                    break;
                }
            }
        } else {
            for (auto& element : m_elements) {
                element->HandleEvent(event, camera);
            }
        }
    }

    void DiagramManager::Render(SDL_Renderer* renderer, const Camera& camera) const noexcept {
        for (const auto& element : m_elements) {
            element->Render(renderer, camera);
        }
    }

    void DiagramManager::AddElement(ElementPtr element) {
        m_elements.push_back(std::move(element));
    }

    void DiagramManager::RemoveElement(size_t index) {
        if (index < m_elements.size()) {
            m_elements.erase(m_elements.begin() + index);
        }
    }

    void DiagramManager::Clear() {
        m_elements.clear();
    }

    DiagramElement* DiagramManager::GetElement(size_t index) const noexcept {
        if (index < m_elements.size()) {
            return m_elements[index].get();
        }
        return nullptr;
    }

    void DiagramManager::xml_serialize(pugi::xml_node& node) const {
        for (const auto& element : m_elements) {
            auto elementNode = node.append_child(element->GetTypeName());
            element->xml_serialize(elementNode);
        }
    }

    void DiagramManager::xml_deserialize(const pugi::xml_node& node) {
        m_elements.clear();
        
        for (const auto& child : node.children()) {
            std::string typeName = child.name();
            
            if (typeName == "Block") {
                auto block = std::make_unique<Block>();
                block->xml_deserialize(child);
                m_elements.push_back(std::move(block));
            }
        }
    }
}