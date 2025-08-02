#pragma once

#include "Interface/Component.hpp"
#include "CameraComponent.hpp"
#include "BlockComponent.hpp"
#include "../../Main/EventHandler.hpp"

namespace Diagram {

    class MinimapComponent : public Component, public EventHandler {
    public:
        struct Data {
            glm::vec4 backgroundColor{0.1f, 0.1f, 0.1f, 0.9f};
            glm::vec4 viewRectColor{0.0f, 0.5f, 1.0f, 0.3f};
            glm::vec4 borderColor{0.7f, 0.7f, 0.7f, 1.0f};
            glm::vec4 componentColor{0.8f, 0.4f, 0.1f, 0.7f};
            glm::vec4 selectedColor{1.0f, 0.2f, 0.2f, 0.9f};  // Red for selected
            glm::vec4 gridColor{0.3f, 0.3f, 0.3f, 0.6f};
            float scale = 0.1f;
            float zoomFactor = 3.0f;  // How much area around camera to show
            float cameraSensitivity = 0.5f;  // How fast camera moves when clicking
            bool visible = true;
            bool showGrid = true;
            bool showComponents = true;
        } data;

    private:
        bool isDraggingViewport = false;
        glm::vec2 dragStartOffset{0.0f, 0.0f};

    public:
        MinimapComponent() {
            // Position on right side of screen (will be adjusted in Render based on screen size)
            this->position = {10.0f, 10.0f};  // Will be set to right side in Render
            this->size = {200.0f, 150.0f};
            this->id = "Minimap";
        }

        void Render(SDL_Renderer* renderer, const CameraView& view, const glm::vec2& screenSize) const override {
            if (!data.visible || !isVisible) {
                return;
            }

            // Position minimap on right side of screen
            const float minimapX = screenSize.x - size.x - 10.0f;  // 10px margin from right edge
            const float minimapY = 10.0f;  // 10px margin from top
            
            // Draw minimap background
            const SDL_FRect backgroundRect = { minimapX, minimapY, size.x, size.y };
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(data.backgroundColor.r * 255), 
                (Uint8)(data.backgroundColor.g * 255), 
                (Uint8)(data.backgroundColor.b * 255), 
                (Uint8)(data.backgroundColor.a * 255)
            );
            SDL_RenderFillRectF(renderer, &backgroundRect);

            // Get diagram data to render content - now passed from outside
            // Note: gridComponent should be passed from the caller
            // For now, we'll skip content rendering and focus on the background
            
            // Safety bounds are handled in the calling code now
            // The minimap background and border are always rendered

            // Draw nice border around minimap
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(data.borderColor.r * 255), 
                (Uint8)(data.borderColor.g * 255), 
                (Uint8)(data.borderColor.b * 255), 
                (Uint8)(data.borderColor.a * 255)
            );
            // Draw thick border (2 pixels)
            for(int i = 0; i < 2; i++) {
                SDL_FRect borderRect = { minimapX - i, minimapY - i, size.x + 2*i, size.y + 2*i };
                SDL_RenderDrawRectF(renderer, &borderRect);
            }
        }

        // Enhanced render method that takes grid component as parameter
        void RenderWithContent(SDL_Renderer* renderer, const CameraView& view, const glm::vec2& screenSize, const Component& gridComponent) const {
            if (!data.visible || !isVisible) {
                return;
            }

            // Use ImGui overlay instead of SDL to ensure proper layering with text components
            // This ensures minimap renders in the UI layer, above all world components
            ImDrawList* drawList = ImGui::GetBackgroundDrawList();

            // Position minimap on right side of screen
            const float minimapX = screenSize.x - size.x - 10.0f;  // 10px margin from right edge
            const float minimapY = 10.0f;  // 10px margin from top
            
            // Draw minimap background using ImGui
            const ImVec2 backgroundMin(minimapX, minimapY);
            const ImVec2 backgroundMax(minimapX + size.x, minimapY + size.y);
            const ImU32 backgroundColor = IM_COL32(
                (int)(data.backgroundColor.r * 255), 
                (int)(data.backgroundColor.g * 255), 
                (int)(data.backgroundColor.b * 255), 
                (int)(data.backgroundColor.a * 255)
            );
            drawList->AddRectFilled(backgroundMin, backgroundMax, backgroundColor);

            // Calculate minimap view bounds - camera is always centered
            // Add safety bounds to prevent crashes on extreme zoom levels
            const float safeZoom = std::max(0.001f, std::min(1000.0f, view.zoom));  // Clamp zoom to reasonable range
            const glm::vec2 minimapWorldSize = size * data.zoomFactor / safeZoom;
            const glm::vec2 minimapWorldCenter = view.worldPosition;  // Center on camera
            const glm::vec2 minimapWorldMin = minimapWorldCenter - minimapWorldSize * 0.5f;
            const glm::vec2 minimapWorldMax = minimapWorldCenter + minimapWorldSize * 0.5f;

            // Safety check: ensure world bounds are reasonable
            const float worldWidth = minimapWorldMax.x - minimapWorldMin.x;
            const float worldHeight = minimapWorldMax.y - minimapWorldMin.y;
            if(worldWidth > 0.1f && worldHeight > 0.1f && worldWidth < 100000.0f && worldHeight < 100000.0f) {
                // Render grid if enabled (simple grid pattern with ImGui)
                if(data.showGrid) {
                    const ImU32 gridColor = IM_COL32(80, 80, 80, 255);
                    const int gridLines = 10;
                    for(int i = 0; i <= gridLines; ++i) {
                        float x = minimapX + (i * size.x / gridLines);
                        float y = minimapY + (i * size.y / gridLines);
                        drawList->AddLine(ImVec2(x, minimapY), ImVec2(x, minimapY + size.y), gridColor, 1.0f);
                        drawList->AddLine(ImVec2(minimapX, y), ImVec2(minimapX + size.x, y), gridColor, 1.0f);
                    }
                }

                // Render components if enabled (simple component representation)
                if(data.showComponents) {
                    RenderMinimapComponentsWithImGui(drawList, {minimapX, minimapY}, minimapWorldMin, minimapWorldMax, gridComponent);
                }

                // Draw current viewport indicator (simple rectangle)
                const ImU32 viewportColor = IM_COL32(255, 255, 0, 128);
                const glm::vec2 viewportWorldSize = screenSize / view.zoom;
                const glm::vec2 viewportWorldMin = view.worldPosition - viewportWorldSize * 0.5f;
                const glm::vec2 viewportWorldMax = view.worldPosition + viewportWorldSize * 0.5f;
                
                const float viewportMinX = minimapX + ((viewportWorldMin.x - minimapWorldMin.x) / worldWidth) * size.x;
                const float viewportMinY = minimapY + ((viewportWorldMin.y - minimapWorldMin.y) / worldHeight) * size.y;
                const float viewportMaxX = minimapX + ((viewportWorldMax.x - minimapWorldMin.x) / worldWidth) * size.x;
                const float viewportMaxY = minimapY + ((viewportWorldMax.y - minimapWorldMin.y) / worldHeight) * size.y;
                
                drawList->AddRect(ImVec2(viewportMinX, viewportMinY), ImVec2(viewportMaxX, viewportMaxY), viewportColor, 0.0f, 0, 2.0f);
            }

            // Draw nice border around minimap using ImGui
            const ImU32 borderColor = IM_COL32(
                (int)(data.borderColor.r * 255), 
                (int)(data.borderColor.g * 255), 
                (int)(data.borderColor.b * 255), 
                (int)(data.borderColor.a * 255)
            );
            // Draw thick border (2 pixels)
            for(int i = 0; i < 2; i++) {
                const ImVec2 borderMin(minimapX - i, minimapY - i);
                const ImVec2 borderMax(minimapX + size.x + i, minimapY + size.y + i);
                drawList->AddRect(borderMin, borderMax, borderColor, 0.0f, 0, 2.0f);
            }
        }

        bool HandleEvent(const SDL_Event& event, const CameraView& view, const glm::vec2& screenSize) override {
            // Event handling will be moved to Application.cpp to avoid circular dependencies
            // For now, return false so Application can handle minimap events
            return false;
        }

        void RenderUI() override {
            ImGui::Text("Minimap Component");
            ImGui::Separator();
            
            ImGui::Checkbox("Visible", &data.visible);
            ImGui::Checkbox("Show Grid", &data.showGrid);
            ImGui::Checkbox("Show Components", &data.showComponents);
            
            ImGui::DragFloat("Zoom Factor", &data.zoomFactor, 0.1f, 0.5f, 10.0f);
            if(ImGui::IsItemHovered()) {
                ImGui::SetTooltip("How much area around camera to show in minimap");
            }
            
            ImGui::DragFloat("Camera Sensitivity", &data.cameraSensitivity, 0.01f, 0.1f, 2.0f);
            if(ImGui::IsItemHovered()) {
                ImGui::SetTooltip("How fast camera moves when clicking on minimap");
            }
            
            ImGui::DragFloat2("Size", &size.x, 1.0f, 50.0f, 500.0f);
            
            ImGui::ColorEdit4("Background", &data.backgroundColor.x);
            ImGui::ColorEdit4("Viewport Color", &data.viewRectColor.x);
            ImGui::ColorEdit4("Component Color", &data.componentColor.x);
            ImGui::ColorEdit4("Selected Color", &data.selectedColor.x);
            ImGui::ColorEdit4("Border Color", &data.borderColor.x);
            ImGui::ColorEdit4("Grid Color", &data.gridColor.x);
            
            if(ImGui::Button("Reset Minimap")) {
                data.zoomFactor = 3.0f;
                data.cameraSensitivity = 0.5f;
                size = {200.0f, 150.0f};
                data.visible = true;
                data.showGrid = true;
                data.showComponents = true;
            }
        }
        std::string GetTypeName() const override { return "MinimapComponent"; }

        void XmlSerialize(pugi::xml_node& node) const override {
            XML::auto_serialize(data, node);
            // Also serialize size since it's not in data struct
            node.append_attribute("width") = size.x;
            node.append_attribute("height") = size.y;
        }

        void XmlDeserialize(const pugi::xml_node& node) override {
            XML::auto_deserialize(data, node);
            // Also deserialize size
            size.x = node.attribute("width").as_float(200.0f);
            size.y = node.attribute("height").as_float(150.0f);
        }

    private:
        void RenderMinimapGrid(SDL_Renderer* renderer, const glm::vec2& minimapPos, const glm::vec2& worldMin, const glm::vec2& worldMax) const {
            // Safety check for extreme values
            const float worldWidth = worldMax.x - worldMin.x;
            const float worldHeight = worldMax.y - worldMin.y;
            if(worldWidth <= 0.0f || worldHeight <= 0.0f || worldWidth > 100000.0f || worldHeight > 100000.0f) {
                return;  // Skip rendering if world bounds are invalid or too extreme
            }

            // Create a fake grid pattern for better visibility
            const float gridSpacing = 50.0f;  // Fixed grid spacing in world units
            
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(data.gridColor.r * 255),
                (Uint8)(data.gridColor.g * 255),
                (Uint8)(data.gridColor.b * 255),
                (Uint8)(data.gridColor.a * 255));

            // Calculate grid lines in world space with safety limits
            const int maxGridLines = 1000;  // Prevent too many grid lines
            const int firstGridX = std::max(-maxGridLines, static_cast<int>(std::floor(worldMin.x / gridSpacing)));
            const int lastGridX = std::min(maxGridLines, static_cast<int>(std::ceil(worldMax.x / gridSpacing)));
            const int firstGridY = std::max(-maxGridLines, static_cast<int>(std::floor(worldMin.y / gridSpacing)));
            const int lastGridY = std::min(maxGridLines, static_cast<int>(std::ceil(worldMax.y / gridSpacing)));

            // Skip if we would draw too many lines
            if((lastGridX - firstGridX) > maxGridLines || (lastGridY - firstGridY) > maxGridLines) {
                return;
            }

            // Draw vertical lines
            for(int i = firstGridX; i <= lastGridX; ++i) {
                const float worldX = static_cast<float>(i) * gridSpacing;
                const float minimapX = minimapPos.x + ((worldX - worldMin.x) / (worldMax.x - worldMin.x)) * size.x;
                if(minimapX >= minimapPos.x && minimapX <= minimapPos.x + size.x) {
                    SDL_RenderDrawLineF(renderer, minimapX, minimapPos.y, minimapX, minimapPos.y + size.y);
                }
            }

            // Draw horizontal lines
            for(int i = firstGridY; i <= lastGridY; ++i) {
                const float worldY = static_cast<float>(i) * gridSpacing;
                const float minimapY = minimapPos.y + ((worldY - worldMin.y) / (worldMax.y - worldMin.y)) * size.y;
                if(minimapY >= minimapPos.y && minimapY <= minimapPos.y + size.y) {
                    SDL_RenderDrawLineF(renderer, minimapPos.x, minimapY, minimapPos.x + size.x, minimapY);
                }
            }
        }

        void RenderMinimapComponents(SDL_Renderer* renderer, const glm::vec2& minimapPos, const glm::vec2& worldMin, const glm::vec2& worldMax, const Component& rootComponent) const {
            RenderComponentInMinimap(renderer, minimapPos, worldMin, worldMax, &rootComponent);
        }

        void RenderComponentInMinimap(SDL_Renderer* renderer, const glm::vec2& minimapPos, const glm::vec2& worldMin, const glm::vec2& worldMax, const Component* component) const {
            if(!component || !component->IsVisible()) return;

            // Safety check for world bounds
            const float worldWidth = worldMax.x - worldMin.x;
            const float worldHeight = worldMax.y - worldMin.y;
            if(worldWidth <= 0.0f || worldHeight <= 0.0f) return;

            // Render BlockComponents as small rectangles
            if(auto* blockComp = dynamic_cast<const BlockComponent*>(component)) {
                const glm::vec2& worldPos = blockComp->GetWorldPosition();
                const glm::vec2& worldSize = blockComp->size;

                // Check if component is within minimap view
                if(worldPos.x + worldSize.x < worldMin.x || worldPos.x > worldMax.x ||
                   worldPos.y + worldSize.y < worldMin.y || worldPos.y > worldMax.y) {
                    return;  // Component is outside minimap view
                }

                // Convert world position to minimap position with safety checks
                const float minimapX = minimapPos.x + ((worldPos.x - worldMin.x) / worldWidth) * size.x;
                const float minimapY = minimapPos.y + ((worldPos.y - worldMin.y) / worldHeight) * size.y;
                const float minimapW = (worldSize.x / worldWidth) * size.x;
                const float minimapH = (worldSize.y / worldHeight) * size.y;

                // Ensure reasonable rectangle size and position
                if(minimapX < -1000.0f || minimapX > 10000.0f || minimapY < -1000.0f || minimapY > 10000.0f) {
                    return;  // Skip if position is too extreme
                }

                // Use color based on selection state - RED for selected, natural color for others
                if(blockComp->IsSelected()) {
                    SDL_SetRenderDrawColor(renderer, 
                        (Uint8)(data.selectedColor.r * 255),
                        (Uint8)(data.selectedColor.g * 255),
                        (Uint8)(data.selectedColor.b * 255),
                        (Uint8)(data.selectedColor.a * 255));
                } else {
                    // Use the component's actual background color
                    const auto& bgColor = blockComp->data.backgroundColor;
                    SDL_SetRenderDrawColor(renderer,
                        (Uint8)(bgColor.r * 255),
                        (Uint8)(bgColor.g * 255),
                        (Uint8)(bgColor.b * 255),
                        (Uint8)(bgColor.a * 255));
                }

                const SDL_FRect rect = {minimapX, minimapY, std::max(2.0f, minimapW), std::max(2.0f, minimapH)};
                SDL_RenderFillRectF(renderer, &rect);
            }

            // Recursively render children
            for(const auto& child : component->GetChildren()) {
                RenderComponentInMinimap(renderer, minimapPos, worldMin, worldMax, child.get());
            }
        }

        void RenderViewportIndicator(SDL_Renderer* renderer, const glm::vec2& minimapPos, const glm::vec2& worldMin, const glm::vec2& worldMax, const CameraView& view, const glm::vec2& screenSize) const {
            // Safety check for world bounds
            const float worldWidth = worldMax.x - worldMin.x;
            const float worldHeight = worldMax.y - worldMin.y;
            if(worldWidth <= 0.0f || worldHeight <= 0.0f) return;

            // Calculate current viewport in world space
            const glm::vec2 viewportWorldMin = view.ScreenToWorld({0.0f, 0.0f}, screenSize);
            const glm::vec2 viewportWorldMax = view.ScreenToWorld(screenSize, screenSize);

            // Convert to minimap coordinates with safety checks
            const float minimapViewMinX = minimapPos.x + ((viewportWorldMin.x - worldMin.x) / worldWidth) * size.x;
            const float minimapViewMinY = minimapPos.y + ((viewportWorldMin.y - worldMin.y) / worldHeight) * size.y;
            const float minimapViewMaxX = minimapPos.x + ((viewportWorldMax.x - worldMin.x) / worldWidth) * size.x;
            const float minimapViewMaxY = minimapPos.y + ((viewportWorldMax.y - worldMin.y) / worldHeight) * size.y;

            // Skip if coordinates are too extreme
            if(std::abs(minimapViewMinX) > 10000.0f || std::abs(minimapViewMinY) > 10000.0f ||
               std::abs(minimapViewMaxX) > 10000.0f || std::abs(minimapViewMaxY) > 10000.0f) {
                return;
            }

            // Clamp to minimap bounds
            const float clampedMinX = std::max(minimapPos.x, minimapViewMinX);
            const float clampedMinY = std::max(minimapPos.y, minimapViewMinY);
            const float clampedMaxX = std::min(minimapPos.x + size.x, minimapViewMaxX);
            const float clampedMaxY = std::min(minimapPos.y + size.y, minimapViewMaxY);

            // Draw viewport indicator
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(data.viewRectColor.r * 255),
                (Uint8)(data.viewRectColor.g * 255),
                (Uint8)(data.viewRectColor.b * 255),
                (Uint8)(data.viewRectColor.a * 255));

            const SDL_FRect viewportRect = {clampedMinX, clampedMinY, clampedMaxX - clampedMinX, clampedMaxY - clampedMinY};
            SDL_RenderFillRectF(renderer, &viewportRect);

            // Draw viewport border
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(data.viewRectColor.r * 255),
                (Uint8)(data.viewRectColor.g * 255),
                (Uint8)(data.viewRectColor.b * 255),
                255);
            SDL_RenderDrawRectF(renderer, &viewportRect);
        }
        
        // ImGui-based component rendering for proper layering
        void RenderMinimapComponentsWithImGui(ImDrawList* drawList, const glm::vec2& minimapPos, const glm::vec2& worldMin, const glm::vec2& worldMax, const Component& rootComponent) const {
            RenderComponentInMinimapImGui(drawList, minimapPos, worldMin, worldMax, &rootComponent);
        }
        
        void RenderComponentInMinimapImGui(ImDrawList* drawList, const glm::vec2& minimapPos, const glm::vec2& worldMin, const glm::vec2& worldMax, const Component* component) const {
            if(!component || !component->IsVisible()) return;

            // Safety check for world bounds
            const float worldWidth = worldMax.x - worldMin.x;
            const float worldHeight = worldMax.y - worldMin.y;
            if(worldWidth <= 0.0f || worldHeight <= 0.0f) return;

            // Render BlockComponents as small rectangles
            if(auto* blockComp = dynamic_cast<const BlockComponent*>(component)) {
                const glm::vec2& worldPos = blockComp->GetWorldPosition();
                const glm::vec2& worldSize = blockComp->size;

                // Check if component is within minimap view
                if(worldPos.x + worldSize.x < worldMin.x || worldPos.x > worldMax.x ||
                   worldPos.y + worldSize.y < worldMin.y || worldPos.y > worldMax.y) {
                    return;  // Component is outside minimap view
                }

                // Convert world position to minimap position with safety checks
                const float minimapX = minimapPos.x + ((worldPos.x - worldMin.x) / worldWidth) * size.x;
                const float minimapY = minimapPos.y + ((worldPos.y - worldMin.y) / worldHeight) * size.y;
                const float minimapW = (worldSize.x / worldWidth) * size.x;
                const float minimapH = (worldSize.y / worldHeight) * size.y;

                // Ensure reasonable rectangle size and position
                if(minimapX < -1000.0f || minimapX > 10000.0f || minimapY < -1000.0f || minimapY > 10000.0f) {
                    return;  // Skip if position is too extreme
                }

                // Use color based on selection state - RED for selected, natural color for others
                ImU32 rectColor;
                if(blockComp->IsSelected()) {
                    rectColor = IM_COL32(
                        (int)(data.selectedColor.r * 255),
                        (int)(data.selectedColor.g * 255),
                        (int)(data.selectedColor.b * 255),
                        (int)(data.selectedColor.a * 255));
                } else {
                    // Use the component's actual background color
                    const auto& bgColor = blockComp->data.backgroundColor;
                    rectColor = IM_COL32(
                        (int)(bgColor.r * 255),
                        (int)(bgColor.g * 255),
                        (int)(bgColor.b * 255),
                        (int)(bgColor.a * 255));
                }

                // Draw the block rectangle with ImGui
                const ImVec2 rectMin(minimapX, minimapY);
                const ImVec2 rectMax(minimapX + minimapW, minimapY + minimapH);
                drawList->AddRectFilled(rectMin, rectMax, rectColor);
            }

            // Recursively render children
            for(const auto& child : component->GetChildren()) {
                RenderComponentInMinimapImGui(drawList, minimapPos, worldMin, worldMax, child.get());
            }
        }

    };

} // namespace Diagram
