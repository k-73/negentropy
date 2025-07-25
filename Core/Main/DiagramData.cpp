#include "DiagramData.hpp"
#include <pugixml.hpp>
#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include "../Utils/Path.hpp"

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

    auto cameraNode = diagram.child("camera").child("position");
    if (cameraNode) {
        m_camera.position.x = cameraNode.attribute("x").as_float();
        m_camera.position.y = cameraNode.attribute("y").as_float();
    }

    for (pugi::xml_node blockNode : diagram.child("blocks").children("block")) {
        Diagram::Block block;
        block.type = magic_enum::enum_cast<Diagram::BlockType>(blockNode.attribute("type").as_string()).value_or(Diagram::BlockType::Process);
        block.position.x = blockNode.child("position").attribute("x").as_float();
        block.position.y = blockNode.child("position").attribute("y").as_float();
        block.size.x = blockNode.child("size").attribute("x").as_float();
        block.size.y = blockNode.child("size").attribute("y").as_float();
        block.label = blockNode.child("label").text().as_string();
        block.color.r = blockNode.child("color").attribute("r").as_float();
        block.color.g = blockNode.child("color").attribute("g").as_float();
        block.color.b = blockNode.child("color").attribute("b").as_float();
        block.color.a = blockNode.child("color").attribute("a").as_float();
        m_blocks.push_back(block);
    }
}

void DiagramData::Save(const std::string& filePath) {
    pugi::xml_document doc;
    auto diagram = doc.append_child("diagram");

    auto cameraNode = diagram.append_child("camera").append_child("position");
    cameraNode.append_attribute("x").set_value(m_camera.position.x);
    cameraNode.append_attribute("y").set_value(m_camera.position.y);

    auto blocksNode = diagram.append_child("blocks");
    for (const auto& block : m_blocks) {
        auto blockNode = blocksNode.append_child("block");
        blockNode.append_attribute("type").set_value(magic_enum::enum_name(block.type).data());
        blockNode.append_child("position").append_attribute("x").set_value(block.position.x);
        blockNode.child("position").append_attribute("y").set_value(block.position.y);
        blockNode.append_child("size").append_attribute("x").set_value(block.size.x);
        blockNode.child("size").append_attribute("y").set_value(block.size.y);
        blockNode.append_child("label").text().set(block.label.c_str());
        auto colorNode = blockNode.append_child("color");
        colorNode.append_attribute("r").set_value(block.color.r);
        colorNode.append_attribute("g").set_value(block.color.g);
        colorNode.append_attribute("b").set_value(block.color.b);
        colorNode.append_attribute("a").set_value(block.color.a);
    }
    doc.save_file(filePath.c_str());
}
