#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"
#include "../Diagram/Component.hpp"
#include "../Diagram/Grid.hpp"
#include "../Diagram/TreeRenderer.hpp"

class DiagramData
{
public:
	DiagramData() noexcept;

	static DiagramData* GetInstance() noexcept { return instance; }
	static void SetInstance(DiagramData* newInstance) noexcept { instance = newInstance; }

	DiagramData(const DiagramData&) = delete;
	DiagramData& operator=(const DiagramData&) = delete;
	DiagramData(DiagramData&&) = delete;
	DiagramData& operator=(DiagramData&&) = delete;

	void Load(const std::string& filePath);
	void Save(const std::string& filePath) const;

	const std::vector<std::unique_ptr<Diagram::ComponentBase>>& GetComponentList() const noexcept { return componentList; }
	std::vector<std::unique_ptr<Diagram::ComponentBase>>& GetComponentList() noexcept { return componentList; }

	template<typename T>
	std::vector<T*> GetComponentsOfType() const noexcept {
		std::vector<T*> result;
		for(const auto& comp: componentList) {
			if(auto* typed = dynamic_cast<T*>(comp.get()))
				result.push_back(typed);
		}
		return result;
	}

	const Diagram::Camera& GetCamera() const noexcept { return cameraData; }
	Diagram::Camera& GetCamera() noexcept { return cameraData; }

	const Diagram::Grid& GetGrid() const noexcept { return gridData; }
	Diagram::Grid& GetGrid() noexcept { return gridData; }

	Diagram::TreeRenderer::GroupState GetGroupState() const noexcept {
		return {groupMap, groupNameMap, isGroupExpandedMap, nullptr, nullptr};
	}
	void UpdateGroups(const std::map<std::string, std::string>& groups) noexcept { groupMap = groups; }
	void UpdateGroupExpanded(const std::map<std::string, bool>& expanded) noexcept { isGroupExpandedMap = expanded; }

	void AddBlock(bool isUseCursorPosition = false, SDL_Window* window = nullptr) noexcept;

private:
	inline static DiagramData* instance = nullptr;

	std::unique_ptr<Diagram::ComponentBase> CreateComponent(const std::string& type) const;
	void LoadHierarchy(pugi::xml_node node, const std::string& parentGroupId);
	void SaveHierarchy(pugi::xml_node node, const std::string& groupId) const;

	std::vector<std::unique_ptr<Diagram::ComponentBase>> componentList;
	Diagram::Camera cameraData;
	Diagram::Grid gridData;
	std::map<std::string, std::string> groupMap;
	std::map<std::string, std::string> groupNameMap;
	std::map<std::string, bool> isGroupExpandedMap;
};
