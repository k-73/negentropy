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

    m_components.clear();
    auto diagram = doc.child("diagram");
    if (!diagram) return;

    if (auto cameraNode = diagram.child("camera")) {
        m_camera.xml_deserialize(cameraNode);
    }

    if (auto gridNode = diagram.child("grid")) {
        m_grid.xml_deserialize(gridNode);
    }

    for (pugi::xml_node blockNode : diagram.child("blocks").children("block")) {
        auto blockComponent = std::make_unique<Diagram::Block>();
        blockComponent->xml_deserialize(blockNode);
        m_components.push_back(std::move(blockComponent));
    }
}

void DiagramData::Save(const std::string& filePath) const {
    pugi::xml_document doc;
    auto diagram = doc.append_child("diagram");

    auto cameraNode = diagram.append_child("camera");
    m_camera.xml_serialize(cameraNode);

    auto gridNode = diagram.append_child("grid");
    m_grid.xml_serialize(gridNode);

    auto blocksNode = diagram.append_child("blocks");
    for (const auto& component : m_components) {
        if (auto* block = dynamic_cast<const Diagram::Block*>(component.get())) {
            auto blockNode = blocksNode.append_child("block");
            block->xml_serialize(blockNode);
        }
    }

    doc.save_file(filePath.c_str());
}
