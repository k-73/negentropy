#pragma once

#include <SDL.h>
#include <cxxabi.h>

#include <algorithm>
#include <glm/vec2.hpp>
#include <memory>
#include <pugixml.hpp>
#include <string>
#include <typeinfo>
#include <vector>

struct ImVec2;

namespace Diagram
{
	struct Camera;

	class EventHandler
	{
	public:
		virtual ~EventHandler() = default;
		virtual bool HandleEvent(const SDL_Event& event, const Camera& camera, const glm::vec2& screenSize) = 0;
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

		virtual void Render(SDL_Renderer* renderer, const Camera& camera, const glm::vec2& screenSize) const {}
		virtual void Update(float deltaTime) {}

		virtual std::string GetDisplayName() const { return id.empty() ? GetTypeName() : id; }
		virtual std::string GetTypeName() const { return "Component"; }

		bool IsSelected() const { return isSelected; }
		void Select() { isSelected = true; }
		void Deselect() { isSelected = false; }
		bool IsVisible() const { return isVisible; }
		void Hide() { isVisible = false; }
		void Show() { isVisible = true; }

		Component* GetParent() { return parent; }

		void AddChild(std::unique_ptr<Component> child) {
			if(!child || child.get() == this || IsAncestor(child.get())) {
				return;
			}

			child->parent = this;
			child->DirtyCache();

			RecalculateBounds(child.get());

			children.push_back(std::move(child));
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

		// TODO: Consider moving settings to a separate class
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

	protected:
		virtual void RenderOfficial(SDL_Renderer* renderer, const Camera& camera, const glm::vec2& screenSize) const {
			if(!isVisible) return;

			Render(renderer, camera, screenSize);
			for(const auto& child: children) {
				child->RenderOfficial(renderer, camera, screenSize);
			}
		}

		virtual void UpdateOfficial(float deltaTime) {
			Update(deltaTime);
			for(const auto& child: children) {
				child->Update(deltaTime);
			}
		}

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

	inline bool DispatchEvent(Component* root, const SDL_Event& event, const Camera& camera, const glm::vec2& screenSize) {
		if(event.type != SDL_MOUSEBUTTONDOWN && event.type != SDL_MOUSEMOTION && event.type != SDL_MOUSEBUTTONUP) {
			return false;
		}

		glm::vec2 worldPoint;
		if(event.type == SDL_MOUSEMOTION) {
			// worldPoint = ScreenToWorld({event.motion.x, event.motion.y}, camera);
		} else {
			// worldPoint = ScreenToWorld({event.button.x, event.button.y}, camera);
		}
		// For the sake of this example, let's assume worldPoint is calculated here.

		Component* target = root->FindComponentAt(worldPoint);

		for(Component* current = target; current != nullptr; current = current->GetParent()) {
			EventHandler* handler = dynamic_cast<EventHandler*>(current);
			if(handler) {
				if(handler->HandleEvent(event, camera, screenSize)) {
					return true;
				}
			}
		}
		return false;
	}
}

// namespace Diagram
// {
// 	struct Camera;
// 	class Block;

// 	class ComponentBase
// 	{
// 	public:
// 		std::string groupId;
// 		std::string id;

// 		virtual ~ComponentBase() = default;

// 		// Core interface
// 		virtual bool HandleEvent(const SDL_Event& event, const Camera& camera, glm::vec2 screenSize) noexcept = 0;
// 		virtual void Render(SDL_Renderer* renderer, const Camera& camera, glm::vec2 screenSize) const noexcept = 0;
// 		virtual void XmlSerialize(pugi::xml_node& node) const = 0;
// 		virtual void XmlDeserialize(const pugi::xml_node& node) = 0;
// 		virtual std::string GetDisplayName() const noexcept = 0;
// 		virtual std::string GetTypeName() const noexcept = 0;

// 		// Selection management
// 		static ComponentBase* GetSelected() noexcept { return selected; }
// 		static void Select(ComponentBase* component) noexcept { selected = component; }
// 		static void ClearSelection() noexcept { selected = nullptr; }

// 	private:
// 		inline static ComponentBase* selected;
// 	};

// 	template<typename T>
// 	std::string demangle() {
// 		int status;
// 		std::unique_ptr<char, void (*)(void*)> demangled {
// 			abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status), std::free};

// 		if(!demangled) return "unknown";

// 		std::string name {demangled.get()};
// 		if(auto pos = name.find_last_of("::"); pos != std::string::npos)
// 			name = name.substr(pos + 1);
// 		return name;
// 	}

// 	template<typename Derived>
// 	class Component : public ComponentBase
// 	{
// 	public:
// 		std::string GetTypeName() const noexcept override { return demangle<Derived>(); }
// 		static std::string GetStaticTypeName() noexcept { return demangle<Derived>(); }
// 	};

// }