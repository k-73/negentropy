#pragma once

#include <vector>
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"

class DiagramData {
public:
    DiagramData() {
        // Initialize with some default blocks
        m_blocks = {
            {{100, 100, 120, 60}, "Start"},
            {{320, 180, 140, 60}, "Process"},
            {{560, 120, 120, 60}, "End"}
        };
    }

    const std::vector<Diagram::Block>& GetBlocks() const { return m_blocks; }
    std::vector<Diagram::Block>& GetBlocks() { return m_blocks; }

    const Diagram::Camera& GetCamera() const { return m_camera; }
    Diagram::Camera& GetCamera() { return m_camera; }

private:
    std::vector<Diagram::Block> m_blocks;
    Diagram::Camera m_camera;
};
