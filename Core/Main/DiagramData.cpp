#include "DiagramData.hpp"
#include <pugixml.hpp>
#include <iostream>
#include <unordered_map>
#include <functional>
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

    static const std::unordered_map<std::string, std::function<std::unique_ptr<Diagram::Component>()>> componentFactory = {
        {"block", []() { return std::make_unique<Diagram::Block>(); }}
    };

    auto loadComponents = [&](pugi::xml_node parent) {
        for (pugi::xml_node componentNode : parent.children()) {
            std::string type = componentNode.name();
            if (auto it = componentFactory.find(type); it != componentFactory.end()) {
                auto component = it->second();
                component->xml_deserialize(componentNode);
                m_components.push_back(std::move(component));
            }
        }
    };

    if (auto componentsNode = diagram.child("components")) {
        loadComponents(componentsNode);
    } else if (auto blocksNode = diagram.child("blocks")) {
        loadComponents(blocksNode);
    }
}

void DiagramData::Save(const std::string& filePath) const {
    pugi::xml_document doc;
    auto diagram = doc.append_child("diagram");

    auto cameraNode = diagram.append_child("camera");
    m_camera.xml_serialize(cameraNode);

    auto gridNode = diagram.append_child("grid");
    m_grid.xml_serialize(gridNode);

    auto componentsNode = diagram.append_child("components");
    for (const auto& component : m_components) {
        auto typeName = component->GetTypeName();
        auto componentNode = componentsNode.append_child(typeName.c_str());
        component->xml_serialize(componentNode);
    }

    doc.save_file(filePath.c_str());
}
