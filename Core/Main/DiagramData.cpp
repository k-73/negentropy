#include "DiagramData.hpp"
#include <pugixml.hpp>
#include <iostream>
#include "../Utils/Path.hpp"
#include "../Diagram/Block.hpp"

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

    auto loadComponents = [&](pugi::xml_node parent) {
        for (pugi::xml_node componentNode : parent.children()) {
            if (auto component = CreateComponent(componentNode.name())) {
                component->xml_deserialize(componentNode);
                m_components.push_back(std::move(component));
            }
        }
    };

    if (auto componentsNode = diagram.child("components")) {
        loadComponents(componentsNode);
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

std::unique_ptr<Diagram::Component> DiagramData::CreateComponent(const std::string& type) const {
    if (type == "block") return std::make_unique<Diagram::Block>();
    return nullptr;
}
