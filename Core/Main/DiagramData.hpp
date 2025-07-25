#pragma once

#include <vector>
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"

class DiagramData {
public:
    DiagramData() noexcept {
        m_blocks = {
            {{100.0f, 100.0f}, {120.0f, 60.0f}, "Start", Diagram::BlockType::Start, {0.2f, 0.8f, 0.2f, 1.0f}},
            {{320.0f, 180.0f}, {140.0f, 60.0f}, "Process", Diagram::BlockType::Process, {0.35f, 0.47f, 0.78f, 1.0f}},
            {{560.0f, 120.0f}, {120.0f, 60.0f}, "End", Diagram::BlockType::End, {0.8f, 0.2f, 0.2f, 1.0f}}
        };
    }

    DiagramData(const DiagramData&) = delete;
    DiagramData& operator=(const DiagramData&) = delete;
    DiagramData(DiagramData&&) = delete;
    DiagramData& operator=(DiagramData&&) = delete;

    [[nodiscard]] const std::vector<Diagram::Block>& GetBlocks() const noexcept { return m_blocks; }
    [[nodiscard]] std::vector<Diagram::Block>& GetBlocks() noexcept { return m_blocks; }

    [[nodiscard]] const Diagram::Camera& GetCamera() const noexcept { return m_camera; }
    [[nodiscard]] Diagram::Camera& GetCamera() noexcept { return m_camera; }

private:
    std::vector<Diagram::Block> m_blocks;
    Diagram::Camera m_camera;
};
