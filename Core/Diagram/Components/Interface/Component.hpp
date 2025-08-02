#pragma once

#include <SDL.h>
#include <cxxabi.h>

#include <algorithm>
#include <cstring>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>
//
#include <glm/vec2.hpp>
#include <pugixml.hpp>

#include "Utils/XMLSerialization.hpp"
#include "imgui.h"

struct ImVec2;

namespace Diagram
{
	struct CameraView {
		glm::vec2 worldPosition;
		float zoom;

		glm::vec2 ScreenToWorld(const glm::vec2& screenPos, const glm::vec2& screenSize) const {
			const glm::vec2 center = screenSize * 0.5f;
			return ((screenPos - center) / zoom) + worldPosition;
		}

		glm::vec2 WorldToScreen(const glm::vec2& worldPos, const glm::vec2& screenSize) const {
			const glm::vec2 center = screenSize * 0.5f;
			return ((worldPos - worldPosition) * zoom) + center;
		}
	};

	class EventHandler
	{
	public:
		virtual ~EventHandler() = default;
		virtual bool HandleEvent(const SDL_Event& event, const CameraView& view, const glm::vec2& screenSize) = 0;
	};

	class Component
	{
	public:
		std::string id;
		glm::vec2 position {0.0f, 0.0f};
		glm::vec2 size {0.0f, 0.0f};
		bool isVisible = true;
		bool isSelected = false;

	protected:
		static std::vector<std::unique_ptr<Component>> allComponents;

		Component* parent = nullptr;
		std::vector<std::unique_ptr<Component>> children;

		mutable glm::vec2 cachedWorldPosition {0.0f, 0.0f};
		mutable bool isCacheDirty = true;

	public:
		virtual ~Component() = default;

		virtual void Render(SDL_Renderer* renderer, const CameraView& view, const glm::vec2& screenSize) const {}
		virtual void Update(float deltaTime) {}

		virtual void RenderUI() {}

		// Called when a child component's size changes
		virtual void OnChildSizeChanged(Component* child) {}

		// Called when cache becomes dirty (position/size changes)
		virtual void OnCacheDirty() {}

		virtual std::string GetDisplayName() const { return id.empty() ? GetTypeName() : id; }
		virtual std::string GetTypeName() const { return "Component"; }

		bool IsSelected() const { return isSelected; }
		void Select() { isSelected = true; }
		void Deselect() { isSelected = false; }
		bool IsVisible() const { return isVisible; }
		void Hide() { isVisible = false; }
		void Show() { isVisible = true; }

		Component* GetParent() { return parent; }
		const std::vector<std::unique_ptr<Component>>& GetChildren() const { return children; }

		void AddChild(std::unique_ptr<Component> child) {
			AddChildAt(std::move(child), children.size());
		}

		void AddChildAt(std::unique_ptr<Component> child, size_t index) {
			if(!child || child.get() == this || IsAncestor(child.get())) {
				return;
			}
			child->parent = this;
			child->DirtyCache();
			RecalculateBounds(child.get());

			index = std::clamp(index, size_t(0), children.size());

			children.insert(children.begin() + index, std::move(child));
		}

		std::unique_ptr<Component> RemoveChild(Component* childToRemove) {
			auto it = std::find_if(children.begin(), children.end(),
								   [&](const std::unique_ptr<Component>& p) {
									   return p.get() == childToRemove;
								   });

			if(it != children.end()) {
				std::unique_ptr<Component> removedChild = std::move(*it);
				children.erase(it);
				removedChild->parent = nullptr;
				return removedChild;
			}
			return nullptr;
		}

		static void Move(Component* componentToMove, Component* newParent, size_t index) {
			if(!componentToMove || !newParent || componentToMove == newParent || newParent->IsAncestor(componentToMove)) {
				return;
			}

			Component* oldParent = componentToMove->GetParent();

			if(oldParent) {
				std::unique_ptr<Component> ownedComponent = oldParent->RemoveChild(componentToMove);
				if(ownedComponent) {
					newParent->AddChildAt(std::move(ownedComponent), index);
				}
			}
		}

		void ReorderChild(Component* child, size_t newIndex) {
			if(!child || child->GetParent() != this) {
				return;
			}

			auto it = std::find_if(children.begin(), children.end(),
								   [&](const std::unique_ptr<Component>& p) {
									   return p.get() == child;
								   });

			if(it == children.end()) {
				return;	 // Should not happen if parent is correct, but good practice.
			}

			std::unique_ptr<Component> ownedChild = std::move(*it);
			children.erase(it);

			if(newIndex > children.size()) {
				newIndex = children.size();
			}
			children.insert(children.begin() + newIndex, std::move(ownedChild));
		}

		void MoveChildUp(Component* child) {
			auto it = std::find_if(children.begin(), children.end(),
								   [&](const std::unique_ptr<Component>& p) {
									   return p.get() == child;
								   });
			if(it != children.end() && it != children.begin()) {
				size_t currentIndex = std::distance(children.begin(), it);
				ReorderChild(child, currentIndex - 1);
			}
		}

		void MoveChildDown(Component* child) {
			auto it = std::find_if(children.begin(), children.end(),
								   [&](const std::unique_ptr<Component>& p) {
									   return p.get() == child;
								   });
			if(it != children.end() && (it + 1) != children.end()) {
				size_t currentIndex = std::distance(children.begin(), it);
				ReorderChild(child, currentIndex + 1);
			}
		}

		void MoveChildToBack(Component* child) {
			ReorderChild(child, 0);
		}

		void MoveChildToFront(Component* child) {
			ReorderChild(child, children.size());
		}

		void SetPosition(const glm::vec2& newPosition) {
			position = newPosition;
			DirtyCache();
		}

		const glm::vec2& GetWorldPosition() const {
			if(isCacheDirty) {
				if(parent) {
					cachedWorldPosition = parent->GetWorldPosition() + position;
				} else {
					cachedWorldPosition = position;
				}
				isCacheDirty = false;
			}
			return cachedWorldPosition;
		}

		void DirtyCache() {
			isCacheDirty = true;
			OnCacheDirty(); // Call virtual method when cache becomes dirty
			for(const auto& child: children) {
				child->DirtyCache();
			}
		}

		bool HitTest(const glm::vec2& worldPoint) const {
			if(!isVisible) {
				return false;
			}
			const glm::vec2& worldPos = GetWorldPosition();
			return (worldPoint.x >= worldPos.x && worldPoint.x <= worldPos.x + size.x &&
					worldPoint.y >= worldPos.y && worldPoint.y <= worldPos.y + size.y);
		}

		Component* FindComponentAt(const glm::vec2& worldPoint) {
			if(!isVisible) {
				return nullptr;
			}

			for(auto it = children.rbegin(); it != children.rend(); ++it) {
				Component* found = (*it)->FindComponentAt(worldPoint);
				if(found) {
					return found;
				}
			}

			if(HitTest(worldPoint)) {
				return this;
			}

			return nullptr;
		}

		// TODO: Consider moving settings to a separate class in future
		virtual void XmlSerialize(pugi::xml_node& node) const {
			node.append_attribute("id") = id.c_str();

			pugi::xml_node childrenNode = node.append_child("Children");
			for(const auto& child: children) {
				pugi::xml_node childNode = childrenNode.append_child(child->GetTypeName().c_str());
				child->XmlSerialize(childNode);
			}
		}

		virtual void XmlDeserialize(const pugi::xml_node& node) {
			id = node.attribute("id").as_string();
		}

		static Component* GetSelected() {
			for(const auto& component: allComponents) {
				if(component->isSelected) {
					return component.get();
				}
			}
			return nullptr;
		}
		static void DeselectAll() {
			for(auto& component: allComponents) {
				component->isSelected = false;
			}
		}

	public:
		void RenderOfficial(SDL_Renderer* renderer, const CameraView& view, const glm::vec2& screenSize) const {
			if(!isVisible) return;

			Render(renderer, view, screenSize);
			for(const auto& child: children) {
				child->RenderOfficial(renderer, view, screenSize);
			}
		}

		void RenderUIOfficial() {
			if(!isVisible) return;

			ImGui::PushID(&id);
			RenderUI();
			ImGui::PopID();

			for(const auto& child: children) {
				child->RenderUIOfficial();
			}
		}

		void UpdateOfficial(float deltaTime) {
			Update(deltaTime);
			for(const auto& child: children) {
				child->UpdateOfficial(deltaTime);
			}
		}

	protected:
		bool IsAncestor(const Component* potentialAncestor) const {
			for(const Component* p = parent; p != nullptr; p = p->parent) {
				if(p == potentialAncestor) return true;
			}
			return false;
		}

		void RecalculateBounds(const Component* child) {
			float requiredWidth = child->position.x + child->size.x;
			float requiredHeight = child->position.y + child->size.y;

			size.x = std::max(size.x, requiredWidth);
			size.y = std::max(size.y, requiredHeight);
		}
	};

	inline std::vector<std::unique_ptr<Component>> Component::allComponents {};

	inline bool DispatchEvent(Component* root, const SDL_Event& event, const CameraView& view, const glm::vec2& screenSize) {
		static Component* capturedComponent = nullptr; // Component that has captured mouse events
		
		if(event.type != SDL_MOUSEBUTTONDOWN && event.type != SDL_MOUSEMOTION && event.type != SDL_MOUSEBUTTONUP) {
			return false;
		}

		glm::vec2 worldPoint;
		if(event.type == SDL_MOUSEMOTION) {
			worldPoint = view.ScreenToWorld({(float)event.motion.x, (float)event.motion.y}, screenSize);
		} else {
			worldPoint = view.ScreenToWorld({(float)event.button.x, (float)event.button.y}, screenSize);
		}

		// Handle mouse button up - always send to captured component first, then release capture
		if(event.type == SDL_MOUSEBUTTONUP) {
			if(capturedComponent) {
				EventHandler* handler = dynamic_cast<EventHandler*>(capturedComponent);
				if(handler) {
					bool handled = handler->HandleEvent(event, view, screenSize);
					capturedComponent = nullptr; // Release capture after mouse up
					if(handled) return true;
				}
				capturedComponent = nullptr; // Release capture even if not handled
			}
		}

		// Handle mouse motion - send to captured component if exists
		if(event.type == SDL_MOUSEMOTION && capturedComponent) {
			EventHandler* handler = dynamic_cast<EventHandler*>(capturedComponent);
			if(handler) {
				if(handler->HandleEvent(event, view, screenSize)) {
					return true;
				}
			}
		}

		// For mouse down or uncaptured motion, find component at mouse position
		Component* target = root->FindComponentAt(worldPoint);

		for(Component* current = target; current != nullptr; current = current->GetParent()) {
			EventHandler* handler = dynamic_cast<EventHandler*>(current);
			if(handler) {
				if(handler->HandleEvent(event, view, screenSize)) {
					// If this was a mouse down event, capture this component for future motion events
					if(event.type == SDL_MOUSEBUTTONDOWN) {
						capturedComponent = current;
					}
					return true;
				}
			}
		}
		return false;
	}
}