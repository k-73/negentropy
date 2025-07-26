#include "Application.hpp"
#include "Renderer.hpp"
#include "EventHandler.hpp"
#include "DiagramData.hpp"
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <filesystem>
#include <vector>
#include <string>
#include "../Utils/Path.hpp"

namespace fs = std::filesystem;

Application::Application() : m_eventHandler(std::make_unique<EventHandler>()), m_diagramData(std::make_unique<DiagramData>()) {
    InitSDL();
    CreateWindow();

    if (!m_renderer.Initialize(m_window)) {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        throw std::runtime_error("Failed to initialize renderer");
    }

    InitializeImGui();
    
    spdlog::info("Application initialized successfully");

    RefreshWorkspaceFiles();
    SDL_Delay(16);
    SDL_ShowWindow(m_window);
}

Application::~Application() {
    spdlog::info("Shutting down application...");
    
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    SDL_Quit();
    spdlog::info("Application shutdown complete");
}

void Application::Run() {
    while (m_running) {
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ProcessEvents();
        Update();
        RenderFrame();
    }
}

void Application::InitializeImGui() const {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    
    ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer.GetSDLRenderer());
    ImGui_ImplSDLRenderer2_Init(m_renderer.GetSDLRenderer());

    spdlog::info("ImGui initialized with SDL2 renderer");
}

void Application::ProcessEvents() noexcept {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        
        if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            m_running = false;
            return;
        }
        
        bool shouldProcessEvent = true;
        if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEWHEEL) {
            shouldProcessEvent = !ImGui::GetIO().WantCaptureMouse;
        } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP || event.type == SDL_TEXTINPUT) {
            shouldProcessEvent = !ImGui::GetIO().WantCaptureKeyboard;
        }

        if (shouldProcessEvent) {
            EventHandler::HandleEvent(event, m_diagramData->GetCamera(), m_diagramData->GetBlocks());
        }
    }
}

void Application::Update() noexcept {
}

void Application::RenderFrame() noexcept {
    RenderUI();
    
    m_renderer.Clear();
    m_renderer.DrawGrid(m_diagramData->GetCamera());
    m_renderer.DrawBlocks(m_diagramData->GetBlocks(), m_diagramData->GetCamera());

    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer.GetSDLRenderer());

    m_renderer.Present();
}

void Application::RenderUI() noexcept {

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::BeginMenu("Load")) {
                for (const auto& file : m_workspaceFiles) {
                    if (ImGui::MenuItem(file.c_str())) {
                        m_diagramData->Load((Utils::GetWorkspacePath() / file).string());
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Save")) {
                m_diagramData->Save((Utils::GetWorkspacePath() / "Default.xml").string());
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                m_running = false;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Properties", nullptr, &m_showProperties);
            ImGui::MenuItem("Demo", nullptr, &m_showDemo);
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
    
    if (m_showProperties) {
        RenderPropertiesPanel();
    }
    
    if (m_showDemo) {
        ImGui::ShowDemoWindow(&m_showDemo);
    }
}

void Application::RenderPropertiesPanel() noexcept {
    ImGui::Begin("Properties", &m_showProperties);
    
    auto& blocks = m_diagramData->GetBlocks();
    auto& camera = m_diagramData->GetCamera();
    
    ImGui::Text("Camera Position: (%.1f, %.1f)", camera.data.position.x, camera.data.position.y);
    ImGui::Text("Blocks Count: %zu", blocks.size());
    
    if (ImGui::Button("Add Block")) {
        blocks.emplace_back();
        auto& newBlock = blocks.back();
        newBlock.data.position = {100.0f + static_cast<float>(blocks.size()) * 150.0f, 100.0f};
        newBlock.data.label = "Block " + std::to_string(blocks.size());
    }
    
    ImGui::Separator();
    
    for (size_t i = 0; i < blocks.size(); ++i) {
        auto& block = blocks[i];
        ImGui::PushID(static_cast<int>(i));
        
        if (ImGui::CollapsingHeader(("Block " + std::to_string(i + 1)).c_str())) {
            char labelBuffer[256];
            strncpy(labelBuffer, block.data.label.c_str(), sizeof(labelBuffer) - 1);
            labelBuffer[sizeof(labelBuffer) - 1] = '\0';
            if (ImGui::InputText("Label", labelBuffer, sizeof(labelBuffer))) {
                block.data.label = labelBuffer;
            }
            ImGui::DragFloat2("Position", &block.data.position.x, 1.0f);
            ImGui::DragFloat2("Size", &block.data.size.x, 1.0f, 10.0f, 500.0f);
            ImGui::ColorEdit4("Color", &block.data.backgroundColor.x);
            
            const char* typeNames[] = {"Start", "Process", "Decision", "End"};
            int currentType = static_cast<int>(block.data.type);
            if (ImGui::Combo("Type", &currentType, typeNames, 4)) {
                block.data.type = static_cast<Diagram::Block::Type>(currentType);
            }
            
            if (ImGui::Button("Delete")) {
                blocks.erase(blocks.begin() + i);
                ImGui::PopID();
                break;
            }
        }
        
        ImGui::PopID();
    }
    
    ImGui::End();
}

void Application::RefreshWorkspaceFiles() {
    m_workspaceFiles.clear();
    const auto path = Utils::GetWorkspacePath();
    for (const auto & entry : fs::directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".xml") {
            m_workspaceFiles.push_back(entry.path().filename().string());
        }
    }
}

void Application::InitSDL() {
    spdlog::info("Initializing SDL...");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        throw std::runtime_error("SDL_Init error: " + std::string(SDL_GetError()));

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
}

void Application::CreateWindow() {
    spdlog::info("Creating window...");
    int displayIndex = 0;
    int mouseX, mouseY;
    SDL_GetGlobalMouseState(&mouseX, &mouseY);
    int numDisplays = SDL_GetNumVideoDisplays();

    for (int i = 0; i < numDisplays; i++) {
        SDL_Rect displayBounds;
        if (SDL_GetDisplayBounds(i, &displayBounds) == 0 &&
            mouseX >= displayBounds.x && mouseX < displayBounds.x + displayBounds.w &&
            mouseY >= displayBounds.y && mouseY < displayBounds.y + displayBounds.h) {
            displayIndex = i;
            break;
        }
    }

    m_window = SDL_CreateWindow("Negentropy - Diagram Editor",
                                SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex),
                                SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), 1280, 720,
                                SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!m_window) {
        SDL_Quit();
        throw std::runtime_error("SDL_CreateWindow error: " + std::string(SDL_GetError()));
    }
}
