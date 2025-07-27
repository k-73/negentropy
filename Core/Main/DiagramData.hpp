#pragma once

#include <vector>
#include <string>
#include <memory>
#include "../Diagram/Component.hpp"
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"
#include "../Diagram/Grid.hpp"

class DiagramData {
public:
    DiagramData() noexcept;

    DiagramData(const DiagramData&) = delete;
    DiagramData& operator=(const DiagramData&) = delete;
    DiagramData(DiagramData&&) = delete;
    DiagramData& operator=(DiagramData&&) = delete;

    void Load(const std::string& filePath);
    void Save(const std::string& filePath) const;

    const std::vector<std::unique_ptr<Diagram::Component>>& GetComponents() const noexcept { return m_components; }
    std::vector<std::unique_ptr<Diagram::Component>>& GetComponents() noexcept { return m_components; }
    
    std::vector<Diagram::Block*> GetBlocks() const noexcept {
        std::vector<Diagram::Block*> blocks;
        for (const auto& comp : m_components) {
            if (auto* block = dynamic_cast<Diagram::Block*>(comp.get())) 
                blocks.push_back(block);
        }
        return blocks;
    }

    const Diagram::Camera& GetCamera() const noexcept { return m_camera; }
    Diagram::Camera& GetCamera() noexcept { return m_camera; }

    const Diagram::Grid& GetGrid() const noexcept { return m_grid; }
    Diagram::Grid& GetGrid() noexcept { return m_grid; }

private:
    std::vector<std::unique_ptr<Diagram::Component>> m_components;
    Diagram::Camera m_camera;
    Diagram::Grid m_grid;
};
