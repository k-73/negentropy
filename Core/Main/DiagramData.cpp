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
    
    if (auto componentsNode = diagram.child("Components")) {
        for (pugi::xml_node componentNode : componentsNode.children()) {
            if (auto component = CreateComponent(componentNode.name())) {
                component->xml_deserialize(componentNode);
                m_components.push_back(std::move(component));
            }
        }
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
        if (strcmp(child.name(), "Group") == 0) {
            auto id = child.attribute("id").as_string();
            auto name = child.attribute("name").as_string();
            m_groups[id] = parentGroupId;
            m_groupNames[id] = name;
            LoadHierarchy(child, id);
        } else if (strcmp(child.name(), "Component") == 0) {
            auto type = child.attribute("type").as_string();
            if (auto component = CreateComponent(type)) {
                component->groupId = parentGroupId;
                component->xml_deserialize(child);
                m_components.push_back(std::move(component));
            }
        }
    }
}

void DiagramData::SaveHierarchy(pugi::xml_node node, const std::string& groupId) const {
    std::vector<std::pair<std::string, std::string>> groupsInOrder;
    std::vector<Diagram::ComponentBase*> componentsInOrder;
    
    for (const auto& [id, parent] : m_groups) {
        if (parent == groupId) {
            groupsInOrder.emplace_back(id, m_groupNames.contains(id) ? m_groupNames.at(id) : id);
        }
    }
    
    for (const auto& component : m_components) {
        if (component->groupId == groupId) {
            componentsInOrder.push_back(component.get());
        }
    }
    
    std::sort(groupsInOrder.begin(), groupsInOrder.end(), [&](const auto& a, const auto& b) {
        auto itA = std::ranges::find_if(m_components, [&](const auto& c) { return c->groupId == a.first; });
        auto itB = std::ranges::find_if(m_components, [&](const auto& c) { return c->groupId == b.first; });
        return itA < itB;
    });
    
    for (const auto& [id, name] : groupsInOrder) {
        auto groupNode = node.append_child("Group");
        groupNode.append_attribute("id").set_value(id.c_str());
        groupNode.append_attribute("name").set_value(name.c_str());
        SaveHierarchy(groupNode, id);
    }
    
    for (const auto& component : componentsInOrder) {
        auto componentNode = node.append_child("Component");
        componentNode.append_attribute("id").set_value(("comp" + std::to_string(reinterpret_cast<uintptr_t>(component))).c_str());
        componentNode.append_attribute("type").set_value(component->GetTypeName().c_str());
        component->xml_serialize(componentNode);
    }
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
