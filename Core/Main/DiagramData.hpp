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
    
    const std::vector<Diagram::Block>& GetBlocks() const noexcept { return m_blocks; }
    std::vector<Diagram::Block>& GetBlocks() noexcept { return m_blocks; }

    const Diagram::Camera& GetCamera() const noexcept { return m_camera; }
    Diagram::Camera& GetCamera() noexcept { return m_camera; }

    const Diagram::Grid& GetGrid() const noexcept { return m_grid; }
    Diagram::Grid& GetGrid() noexcept { return m_grid; }

private:
    std::vector<std::unique_ptr<Diagram::Component>> m_components;
    std::vector<Diagram::Block> m_blocks;
    Diagram::Camera m_camera;
    Diagram::Grid m_grid;
};
