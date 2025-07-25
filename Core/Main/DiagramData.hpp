#pragma once

#include <vector>
#include <string>
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"

class DiagramData {
public:
    DiagramData() noexcept;

    DiagramData(const DiagramData&) = delete;
    DiagramData& operator=(const DiagramData&) = delete;
    DiagramData(DiagramData&&) = delete;
    DiagramData& operator=(DiagramData&&) = delete;

    void Load(const std::string& filePath);
    void Save(const std::string& filePath);

    [[nodiscard]] const std::vector<Diagram::Block>& GetBlocks() const noexcept { return m_blocks; }
    [[nodiscard]] std::vector<Diagram::Block>& GetBlocks() noexcept { return m_blocks; }

    [[nodiscard]] const Diagram::Camera& GetCamera() const noexcept { return m_camera; }
    [[nodiscard]] Diagram::Camera& GetCamera() noexcept { return m_camera; }

private:
    std::vector<Diagram::Block> m_blocks;
    Diagram::Camera m_camera;
};
