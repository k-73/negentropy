#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>
#include "../Diagram/Component.hpp"
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"
#include "../Diagram/Grid.hpp"
#include "../Diagram/TreeRenderer.hpp"

class DiagramData {
public:
    DiagramData() noexcept;

    static DiagramData* GetInstance() noexcept { return s_instance; }
    static void SetInstance(DiagramData* instance) noexcept { s_instance = instance; }

    DiagramData(const DiagramData&) = delete;
    DiagramData& operator=(const DiagramData&) = delete;
    DiagramData(DiagramData&&) = delete;
    DiagramData& operator=(DiagramData&&) = delete;

    void Load(const std::string& filePath);
    void Save(const std::string& filePath) const;

    const std::vector<std::unique_ptr<Diagram::ComponentBase>>& GetComponents() const noexcept { return m_components; }
    std::vector<std::unique_ptr<Diagram::ComponentBase>>& GetComponents() noexcept { return m_components; }
    
    template<typename T>
    std::vector<T*> GetComponentsOfType() const noexcept {
        std::vector<T*> result;
        for (const auto& comp : m_components) {
            if (auto* typed = dynamic_cast<T*>(comp.get())) 
                result.push_back(typed);
        }
        return result;
    }

    const Diagram::Camera& GetCamera() const noexcept { return m_camera; }
    Diagram::Camera& GetCamera() noexcept { return m_camera; }

    const Diagram::Grid& GetGrid() const noexcept { return m_grid; }
    Diagram::Grid& GetGrid() noexcept { return m_grid; }

    Diagram::TreeRenderer::GroupState GetGroupState() const noexcept { 
        return {m_groups, m_groupNames, m_groupExpanded, nullptr, nullptr}; 
    }
    void UpdateGroups(const std::map<std::string, std::string>& groups) noexcept { m_groups = groups; }
    void UpdateGroupExpanded(const std::map<std::string, bool>& expanded) noexcept { m_groupExpanded = expanded; }

    void AddBlock(bool useCursorPosition = false, SDL_Window* window = nullptr) noexcept;

private:
    inline static DiagramData* s_instance = nullptr;
    
    std::unique_ptr<Diagram::ComponentBase> CreateComponent(const std::string& type) const;
    void LoadHierarchy(pugi::xml_node node, const std::string& parentGroupId);
    void SaveHierarchy(pugi::xml_node node, const std::string& groupId) const;
    
    std::vector<std::unique_ptr<Diagram::ComponentBase>> m_components;
    Diagram::Camera m_camera;
    Diagram::Grid m_grid;
    std::map<std::string, std::string> m_groups;
    std::map<std::string, std::string> m_groupNames;
    std::map<std::string, bool> m_groupExpanded;
};
