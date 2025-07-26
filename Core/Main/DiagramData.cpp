#include "DiagramData.hpp"
#include <pugixml.hpp>
#include <iostream>
#include "../Utils/Path.hpp"
#include "../Utils/XMLSerialization.hpp"

DiagramData::DiagramData() noexcept {
    Load((Utils::GetWorkspacePath() / "Default.xml").string());
}

void DiagramData::Load(const std::string& filePath) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filePath.c_str());
    if (!result) {
        std::cerr << "Error loading file: " << result.description() << std::endl;
        return;
    }

    m_blocks.clear();
    auto diagram = doc.child("diagram");
    if (!diagram) return;

    // Load camera using new serialization system
    if (auto cameraNode = diagram.child("camera")) {
        m_camera.xml_deserialize(cameraNode);
    }

    // Load blocks using new serialization system
    for (pugi::xml_node blockNode : diagram.child("blocks").children("block")) {
        Diagram::Block block;
        block.xml_deserialize(blockNode);
        m_blocks.push_back(block);
    }
}

void DiagramData::Save(const std::string& filePath) {
    pugi::xml_document doc;
    auto diagram = doc.append_child("diagram");

    // Save camera using new serialization system
    auto cameraNode = diagram.append_child("camera");
    m_camera.xml_serialize(cameraNode);

    // Save blocks using new serialization system
    auto blocksNode = diagram.append_child("blocks");
    for (const auto& block : m_blocks) {
        auto blockNode = blocksNode.append_child("block");
        block.xml_serialize(blockNode);
    }
    
    doc.save_file(filePath.c_str());
}
