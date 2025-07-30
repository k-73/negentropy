
#include "DiagramData.hpp"

#include <iostream>
#include <pugixml.hpp>

#include "../Diagram/Block.hpp"
#include "../Utils/Notification.hpp"
#include "../Utils/Path.hpp"

DiagramData::DiagramData() noexcept {
	Load((Utils::GetWorkspacePath() / "Default.xml").string());
}

void DiagramData::Load(const std::string& filePath) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filePath.c_str());
	if(!result) {
		std::cerr << "Error loading file: " << result.description() << std::endl;
		return;
	}

	componentList.clear();
	auto diagram = doc.child("Diagram");
	if(!diagram) return;

	if(auto cameraNode = diagram.child("Camera")) {
		cameraData.XmlDeserialize(cameraNode);
	}

	if(auto gridNode = diagram.child("Grid")) {
		gridData.XmlDeserialize(gridNode);
	}

	if(auto rootNode = diagram.child("Root")) {
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
	cameraData.XmlSerialize(cameraNode);

	auto gridNode = diagram.append_child("Grid");
	gridData.XmlSerialize(gridNode);

	auto rootNode = diagram.append_child("Root");
	SaveHierarchy(rootNode, "");

	if(doc.save_file(filePath.c_str())) {
		Notify::Success("Diagram saved successfully!");
	} else {
		Notify::Error("Error saving diagram!");
	}
}

std::unique_ptr<Diagram::ComponentBase> DiagramData::CreateComponent(const std::string& type) const {
	if(type == "Block") return std::make_unique<Diagram::Block>();
	return nullptr;
}

void DiagramData::LoadHierarchy(pugi::xml_node node, const std::string& parentGroupId) {
	for(auto child: node.children()) {
		const std::string name = child.name();
		if(name == "Group") {
			const std::string id = child.attribute("id").as_string();
			groupMap[id] = parentGroupId;
			groupNameMap[id] = child.attribute("name").as_string();
			isGroupExpandedMap[id] = child.attribute("expanded").as_bool(true);
			LoadHierarchy(child, id);
		} else if(name == "Component") {
			if(auto component = CreateComponent(child.attribute("type").as_string())) {
				component->groupId = parentGroupId;
				component->id = child.attribute("id").as_string();
				component->XmlDeserialize(child);
				componentList.push_back(std::move(component));
			}
		}
	}
}

void DiagramData::SaveHierarchy(pugi::xml_node node, const std::string& groupId) const {
	for(const auto& [id, parent]: groupMap) {
		if(parent == groupId) {
			auto groupNode = node.append_child("Group");
			groupNode.append_attribute("id").set_value(id.c_str());
			groupNode.append_attribute("name").set_value(groupNameMap.contains(id) ? groupNameMap.at(id).c_str() : id.c_str());
			groupNode.append_attribute("expanded").set_value(isGroupExpandedMap.contains(id) && isGroupExpandedMap.at(id));
			SaveHierarchy(groupNode, id);
		}
	}

	for(const auto& component: componentList) {
		if(component->groupId == groupId) {
			auto componentNode = node.append_child("Component");
			const auto& id = component->id.empty() ? "comp" + std::to_string(reinterpret_cast<uintptr_t>(component.get())) : component->id;
			componentNode.append_attribute("id").set_value(id.c_str());
			componentNode.append_attribute("type").set_value(component->GetTypeName().c_str());
			component->XmlSerialize(componentNode);
		}
	}
}

void DiagramData::AddBlock(bool isUsedCursorPosition, SDL_Window* window) noexcept {
	const size_t blockCount = GetComponentsOfType<Diagram::Block>().size();
	auto newBlock = std::make_unique<Diagram::Block>();

	if(isUsedCursorPosition && window) {
		const ImVec2 mousePosition = ImGui::GetMousePos();
		int windowWidth, windowHeight;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);
		const glm::vec2 screenCenter(windowWidth * 0.5f, windowHeight * 0.5f);
		newBlock->data.position = (glm::vec2(mousePosition.x, mousePosition.y) - screenCenter) / cameraData.data.zoom + cameraData.data.position;
	} else {
		newBlock->data.position = cameraData.data.position - newBlock->data.size * 0.5f;
	}

	newBlock->data.label = "Block " + std::to_string(blockCount + 1);
	newBlock->id = "block_" + std::to_string(blockCount + 1);
	componentList.push_back(std::move(newBlock));
}
