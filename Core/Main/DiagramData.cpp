
#include "DiagramData.hpp"

#include <iostream>
#include <pugixml.hpp>

#include "../Diagram/Components/BlockComponent.hpp"
#include "../Utils/Notification.hpp"
#include "../Utils/Path.hpp"

DiagramData::DiagramData() noexcept {
	// Initialize camera and grid components
	cameraData = std::make_unique<Diagram::CameraComponent>();
	gridData = std::make_unique<Diagram::GridComponent>(glm::vec2{1000.0f, 1000.0f});
	
	// Set IDs for the components
	cameraData->id = "Main Camera";
	gridData->id = "Background Grid";
	
	// Add them to the component list so they appear in the tree
	// Note: We store raw pointers in componentList but manage memory separately
	// This is a temporary solution until we refactor the architecture
	
	// TODO: Re-enable XML loading after component system is stable
	// Load((Utils::GetWorkspacePath() / "Default.xml").string());
}

void DiagramData::Load(const std::string& filePath) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filePath.c_str());
	if(!result) {
		std::cerr << "Error loading file: " << result.description() << std::endl;
		return;
	}

	componentList.clear();
	
	// Clear existing children from grid
	auto& gridChildren = gridData->GetChildren();
	while(!gridChildren.empty()) {
		gridData->RemoveChild(gridChildren[0].get());
	}
	
	auto diagram = doc.child("Diagram");
	if(!diagram) return;

	if(auto cameraNode = diagram.child("Camera")) {
		cameraData->XmlDeserialize(cameraNode);
	}

	if(auto gridNode = diagram.child("Grid")) {
		gridData->XmlDeserialize(gridNode);
		LoadHierarchyIntoComponent(gridNode, gridData.get());
	}

	// Load orphaned components (for backward compatibility)
	if(auto rootNode = diagram.child("Root")) {
		LoadHierarchy(rootNode, "");
	}
	if(auto orphanedNode = diagram.child("OrphanedComponents")) {
		LoadHierarchy(orphanedNode, "");
	}
}

void DiagramData::Save(const std::string& filePath) const {
	pugi::xml_document doc;
	auto declarationNode = doc.append_child(pugi::node_declaration);
	declarationNode.append_attribute("version") = "1.0";
	declarationNode.append_attribute("encoding") = "UTF-8";

	auto diagram = doc.append_child("Diagram");

	auto cameraNode = diagram.append_child("Camera");
	cameraData->XmlSerialize(cameraNode);

	auto gridNode = diagram.append_child("Grid");
	gridData->XmlSerialize(gridNode);  // This will automatically save all children in hierarchy

	// Only save orphaned components (those not in the hierarchy)
	auto rootNode = diagram.append_child("OrphanedComponents");
	for(const auto& component: componentList) {
		if(!component->GetParent()) {  // Only save components without a parent
			auto componentNode = rootNode.append_child("Component");
			const auto& id = component->id.empty() ? "comp" + std::to_string(reinterpret_cast<uintptr_t>(component.get())) : component->id;
			componentNode.append_attribute("id").set_value(id.c_str());
			componentNode.append_attribute("type").set_value(component->GetTypeName().c_str());
			component->XmlSerialize(componentNode);
		}
	}

	if(doc.save_file(filePath.c_str())) {
		Notify::Success("Diagram saved successfully!");
	} else {
		Notify::Error("Error saving diagram!");
	}
}

std::unique_ptr<Diagram::Component> DiagramData::CreateComponent(const std::string& type) const {
	if(type == "Block" || type == "BlockComponent") {
		return std::make_unique<Diagram::BlockComponent>("New Block", glm::vec2{0.0f, 0.0f}, glm::vec2{100.0f, 50.0f});
	}
	return nullptr;
}

void DiagramData::LoadHierarchy(pugi::xml_node node, const std::string& parentGroupId) {
	for(auto child: node.children()) {
		const std::string name = child.name();
		if(name == "Component") {
			if(auto component = CreateComponent(child.attribute("type").as_string())) {
				component->id = child.attribute("id").as_string();
				component->XmlDeserialize(child);
				componentList.push_back(std::move(component));
			}
		}
	}
}

void DiagramData::LoadHierarchyIntoComponent(const pugi::xml_node& node, Diagram::Component* parent) {
	auto childrenNode = node.child("Children");
	if(childrenNode) {
		for(auto childNode : childrenNode.children()) {
			const std::string childType = childNode.name();
			if(auto child = CreateComponent(childType)) {
				child->id = childNode.attribute("id").as_string();
				child->XmlDeserialize(childNode);
				// Recursively load children of this child
				LoadHierarchyIntoComponent(childNode, child.get());
				parent->AddChild(std::move(child));
			}
		}
	}
}

void DiagramData::SaveHierarchy(pugi::xml_node node, const std::string& groupId) const {
	for(const auto& component: componentList) {
		auto componentNode = node.append_child("Component");
		const auto& id = component->id.empty() ? "comp" + std::to_string(reinterpret_cast<uintptr_t>(component.get())) : component->id;
		componentNode.append_attribute("id").set_value(id.c_str());
		componentNode.append_attribute("type").set_value(component->GetTypeName().c_str());
		component->XmlSerialize(componentNode);
	}
}

void DiagramData::AddComponent(std::unique_ptr<Diagram::Component> component) noexcept {
	if(component) {
		componentList.push_back(std::move(component));
	}
}
