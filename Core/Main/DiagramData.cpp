#include "DiagramData.hpp"
#include <pugixml.hpp>
#include <iostream>
#include "../Utils/Path.hpp"
#include "../Diagram/Block.hpp"
#include "../Utils/Notification.hpp"

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
    auto diagram = doc.child("Diagram");
    if (!diagram) return;

    if (auto cameraNode = diagram.child("Camera")) {
        m_camera.xml_deserialize(cameraNode);
    }

    if (auto gridNode = diagram.child("Grid")) {
        m_grid.xml_deserialize(gridNode);
    }

    if (auto groupsNode = diagram.child("Groups")) {
        for (auto groupNode : groupsNode.children("Group")) {
            auto id = groupNode.attribute("id").as_string();
            auto name = groupNode.attribute("name").as_string();
            auto parent = groupNode.attribute("parent").as_string();
            m_groups[id] = parent;
            m_groupNames[id] = name;
        }
    }

    auto loadComponents = [&](pugi::xml_node parent) {
        for (pugi::xml_node componentNode : parent.children()) {
            if (auto component = CreateComponent(componentNode.name())) {
                component->xml_deserialize(componentNode);
                m_components.push_back(std::move(component));
            }
        }
    };

    if (auto componentsNode = diagram.child("Components")) {
        loadComponents(componentsNode);
    }
}

void DiagramData::Save(const std::string& filePath) const {
    pugi::xml_document doc;
    auto declarationNode = doc.append_child(pugi::node_declaration);
    declarationNode.append_attribute("version") = "1.0";
    declarationNode.append_attribute("encoding") = "UTF-8";

    auto diagram = doc.append_child("Diagram");

    auto cameraNode = diagram.append_child("Camera");
    m_camera.xml_serialize(cameraNode);

    auto gridNode = diagram.append_child("Grid");
    m_grid.xml_serialize(gridNode);

    if (!m_groups.empty()) {
        auto groupsNode = diagram.append_child("Groups");
        for (const auto& [id, parent] : m_groups) {
            auto groupNode = groupsNode.append_child("Group");
            groupNode.append_attribute("id").set_value(id.c_str());
            groupNode.append_attribute("name").set_value(m_groupNames.contains(id) ? m_groupNames.at(id).c_str() : id.c_str());
            groupNode.append_attribute("parent").set_value(parent.c_str());
        }
    }

    auto componentsNode = diagram.append_child("Components");
    for (const auto& component : m_components) {
        auto typeName = component->GetTypeName();
        auto componentNode = componentsNode.append_child(typeName.c_str());
        component->xml_serialize(componentNode);
    }

    if(doc.save_file(filePath.c_str())) {
        Notify::Success("Diagram saved successfully!");
    } else {
        Notify::Error("Error saving diagram!");
    }
}

std::unique_ptr<Diagram::ComponentBase> DiagramData::CreateComponent(const std::string& type) const {
    if (type == "Block") return std::make_unique<Diagram::Block>();
    return nullptr;
}

void DiagramData::AddBlock(bool useCursorPosition, SDL_Window* window) noexcept
{
    size_t blockCount = GetComponentsOfType<Diagram::Block>().size();

    auto newBlock = std::make_unique<Diagram::Block>();

    if (useCursorPosition && window) {
        ImVec2 mousePos = ImGui::GetMousePos();

        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glm::vec2 screenCenter(w / 2.0f, h / 2.0f);

        newBlock->data.position.x = (mousePos.x - screenCenter.x) / m_camera.data.zoom + m_camera.data.position.x;
        newBlock->data.position.y = (mousePos.y - screenCenter.y) / m_camera.data.zoom + m_camera.data.position.y;
    } else {
        newBlock->data.position = m_camera.data.position - newBlock->data.size / 2.0f;
    }

    newBlock->data.label = "Block " + std::to_string(blockCount + 1);
    m_components.push_back(std::move(newBlock));
}
