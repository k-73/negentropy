#pragma once

#include <glm/vec2.hpp>
#include <SDL.h>
#include <pugixml.hpp>
#include <vector>
#include <string>
#include <memory>
#include <typeinfo>
#include <algorithm>
#include <cxxabi.h>

struct ImVec2;

namespace Diagram {
    struct Camera;
    class Block;
    
    class ComponentBase {
    public:
        
        std::string groupId;
        std::string id;

        virtual ~ComponentBase() = default;
        
        // Core interface
        virtual bool HandleEvent(const SDL_Event& event, const Camera& camera, glm::vec2 screenSize) noexcept = 0;
        virtual void Render(SDL_Renderer* renderer, const Camera& camera, glm::vec2 screenSize) const noexcept = 0;
        virtual void XmlSerialize(pugi::xml_node& node) const = 0;
        virtual void XmlDeserialize(const pugi::xml_node& node) = 0;
        virtual std::string GetDisplayName() const noexcept = 0;
        virtual std::string GetTypeName() const noexcept = 0;
        
        // Selection management
        static ComponentBase* GetSelected() noexcept { return s_selected; }
        static void Select(ComponentBase* component) noexcept { s_selected = component; }
        static void ClearSelection() noexcept { s_selected = nullptr; }
        
        
    private:
        inline static ComponentBase* s_selected;
    };
    
    template<typename T>
    std::string demangle() {
        int status;
        std::unique_ptr<char, void(*)(void*)> demangled{
            abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status), std::free};
        
        if (!demangled) return "unknown";
        
        std::string name{demangled.get()};
        if (auto pos = name.find_last_of("::"); pos != std::string::npos) 
            name = name.substr(pos + 1);
        return name;
    }
    
    template<typename Derived>
    class Component : public ComponentBase {
    public:
        std::string GetTypeName() const noexcept override { return demangle<Derived>(); }
        static std::string GetStaticTypeName() noexcept { return demangle<Derived>(); }
    };
    
}