#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../Diagram/Components/Interface/Component.hpp"
#include "../Diagram/Components/CameraComponent.hpp"
#include "../Diagram/Components/GridComponent.hpp"
#include "../Diagram/Components/MinimapComponent.hpp"

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

	const std::vector<std::unique_ptr<Diagram::Component>>& GetComponentList() const noexcept { return componentList; }
	std::vector<std::unique_ptr<Diagram::Component>>& GetComponentList() noexcept { return componentList; }

	template<typename T>
	std::vector<T*> GetComponentsOfType() const noexcept {
		std::vector<T*> result;
		for(const auto& comp: componentList) {
			if(auto* typed = dynamic_cast<T*>(comp.get()))
				result.push_back(typed);
		}
		return result;
	}

	const Diagram::CameraComponent& GetCamera() const noexcept { return *cameraData; }
	Diagram::CameraComponent& GetCamera() noexcept { return *cameraData; }

	const Diagram::GridComponent& GetGrid() const noexcept { return *gridData; }
	Diagram::GridComponent& GetGrid() noexcept { return *gridData; }

	const Diagram::MinimapComponent& GetMinimap() const noexcept { return *minimapData; }
	Diagram::MinimapComponent& GetMinimap() noexcept { return *minimapData; }

	void AddComponent(std::unique_ptr<Diagram::Component> component) noexcept;

private:
	inline static DiagramData* instance = nullptr;

	std::unique_ptr<Diagram::Component> CreateComponent(const std::string& type) const;
	void LoadHierarchy(pugi::xml_node node, const std::string& parentGroupId);
	void LoadHierarchyIntoComponent(const pugi::xml_node& node, Diagram::Component* parent);
	void SaveHierarchy(pugi::xml_node node, const std::string& groupId) const;

	std::vector<std::unique_ptr<Diagram::Component>> componentList;
	std::unique_ptr<Diagram::CameraComponent> cameraData;
	std::unique_ptr<Diagram::GridComponent> gridData;
	std::unique_ptr<Diagram::MinimapComponent> minimapData;
};
