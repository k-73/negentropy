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

    if (auto rootNode = diagram.child("Root")) {
        LoadHierarchy(rootNode, "");
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

    auto rootNode = diagram.append_child("Root");
    SaveHierarchy(rootNode, "");


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

void DiagramData::LoadHierarchy(pugi::xml_node node, const std::string& parentGroupId) {
    for (auto child : node.children()) {
        const std::string name = child.name();
        if (name == "Group") {
            const std::string id = child.attribute("id").as_string();
            m_groups[id] = parentGroupId;
            m_groupNames[id] = child.attribute("name").as_string();
            m_groupExpanded[id] = child.attribute("expanded").as_bool(true);
            LoadHierarchy(child, id);
        } else if (name == "Component") {
            if (auto component = CreateComponent(child.attribute("type").as_string())) {
                component->groupId = parentGroupId;
                component->id = child.attribute("id").as_string();
                component->xml_deserialize(child);
                m_components.push_back(std::move(component));
            }
        }
    }
}

void DiagramData::SaveHierarchy(pugi::xml_node node, const std::string& groupId) const {
    for (const auto& [id, parent] : m_groups) {
        if (parent == groupId) {
            auto groupNode = node.append_child("Group");
            groupNode.append_attribute("id").set_value(id.c_str());
            groupNode.append_attribute("name").set_value(m_groupNames.contains(id) ? m_groupNames.at(id).c_str() : id.c_str());
            groupNode.append_attribute("expanded").set_value(m_groupExpanded.contains(id) && m_groupExpanded.at(id));
            SaveHierarchy(groupNode, id);
        }
    }
    
    for (const auto& component : m_components) {
        if (component->groupId == groupId) {
            auto componentNode = node.append_child("Component");
            componentNode.append_attribute("id").set_value(component->id.empty() ? 
                ("comp" + std::to_string(reinterpret_cast<uintptr_t>(component.get()))).c_str() : 
                component->id.c_str());
            componentNode.append_attribute("type").set_value(component->GetTypeName().c_str());
            component->xml_serialize(componentNode);
        }
    }
}

void DiagramData::AddBlock(bool useCursorPosition, SDL_Window* window) noexcept {
    size_t blockCount = GetComponentsOfType<Diagram::Block>().size();
    auto newBlock = std::make_unique<Diagram::Block>();
    
    if (useCursorPosition && window) {
        const ImVec2 mousePos = ImGui::GetMousePos();
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        const glm::vec2 screenCenter(w * 0.5f, h * 0.5f);
        newBlock->data.position = (glm::vec2(mousePos.x, mousePos.y) - screenCenter) / m_camera.data.zoom + m_camera.data.position;
    } else {
        newBlock->data.position = m_camera.data.position - newBlock->data.size * 0.5f;
    }
    
    newBlock->data.label = "Block " + std::to_string(blockCount + 1);
    newBlock->id = "block_" + std::to_string(blockCount + 1);
    m_components.push_back(std::move(newBlock));
}

