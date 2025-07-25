#include "Application.hpp"
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

Application::Application() {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    spdlog::info("Initializing application...");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        throw std::runtime_error("SDL_Init error: " + std::string(SDL_GetError()));
    
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

    // Create window without OpenGL
    m_window = SDL_CreateWindow("Negentropy - Diagram Editor",
                                SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), 
                                SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), 1280, 720,
                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!m_window) {
        SDL_Quit();
        throw std::runtime_error("SDL_CreateWindow error: " + std::string(SDL_GetError()));
    }

    // Create SDL2 renderer
    m_sdlRenderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_sdlRenderer) {
        spdlog::warn("Failed to create accelerated renderer, trying software...");
        m_sdlRenderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_SOFTWARE);
        if (!m_sdlRenderer) {
            SDL_DestroyWindow(m_window);
            SDL_Quit();
            throw std::runtime_error("Failed to create SDL renderer: " + std::string(SDL_GetError()));
        }
    }
    
    spdlog::info("Created SDL2 renderer successfully");

    if (!m_renderer.Initialize(m_sdlRenderer)) {
        SDL_DestroyRenderer(m_sdlRenderer);
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        throw std::runtime_error("Failed to initialize renderer");
    }

    InitializeImGui();
    
    spdlog::info("Application initialized successfully");
    SDL_ShowWindow(m_window);
}

Application::~Application() {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    spdlog::info("Shutting down application...");
    
    m_running = false;
    
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }
    
    m_renderer.Cleanup();
    
    if (m_sdlRenderer) {
        SDL_DestroyRenderer(m_sdlRenderer);
        m_sdlRenderer = nullptr;
    }
    
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    SDL_Quit();
    spdlog::info("Application shutdown complete");
}

void Application::Run() {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    while (m_running) {
#ifdef TRACY_ENABLE
        FrameMark;
#endif
        ProcessEvents();
        Update();
        RenderFrame();
    }
}

void Application::InitializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    ImGui::StyleColorsDark();
    
    ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_sdlRenderer);
    ImGui_ImplSDLRenderer2_Init(m_sdlRenderer);
    
    spdlog::info("ImGui initialized with SDL2 renderer");
}

void Application::ProcessEvents() {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        
        if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            m_running = false;
        }
        
        if (!ImGui::GetIO().WantCaptureMouse) {
            m_eventHandler.ProcessEvents(m_diagramData.GetCamera(), m_diagramData.GetBlocks(), m_running);
        }
    }
}

void Application::Update() {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
}

void Application::RenderFrame() {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    
    RenderUI();
    
    m_renderer.Clear();
    m_renderer.DrawGrid(m_diagramData.GetCamera());
    m_renderer.DrawBlocks(m_diagramData.GetBlocks(), m_diagramData.GetCamera());
    
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_sdlRenderer);
    
    SDL_RenderPresent(m_sdlRenderer);
}

void Application::RenderUI() {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) {}
            if (ImGui::MenuItem("Open", "Ctrl+O")) {}
            if (ImGui::MenuItem("Save", "Ctrl+S")) {}
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

void Application::RenderPropertiesPanel() {
    ImGui::Begin("Properties", &m_showProperties);
    
    auto& blocks = m_diagramData.GetBlocks();
    auto& camera = m_diagramData.GetCamera();
    
    ImGui::Text("Camera Position: (%.1f, %.1f)", camera.position.x, camera.position.y);
    ImGui::Text("Blocks Count: %zu", blocks.size());
    
    if (ImGui::Button("Add Block")) {
        blocks.emplace_back();
        auto& newBlock = blocks.back();
        newBlock.position = {100.0f + blocks.size() * 150.0f, 100.0f};
        newBlock.label = "Block " + std::to_string(blocks.size());
    }
    
    ImGui::Separator();
    
    for (size_t i = 0; i < blocks.size(); ++i) {
        auto& block = blocks[i];
        ImGui::PushID(static_cast<int>(i));
        
        if (ImGui::CollapsingHeader(("Block " + std::to_string(i + 1)).c_str())) {
            char labelBuffer[256];
            strncpy(labelBuffer, block.label.c_str(), sizeof(labelBuffer) - 1);
            labelBuffer[sizeof(labelBuffer) - 1] = '\0';
            if (ImGui::InputText("Label", labelBuffer, sizeof(labelBuffer))) {
                block.label = labelBuffer;
            }
            ImGui::DragFloat2("Position", &block.position.x, 1.0f);
            ImGui::DragFloat2("Size", &block.size.x, 1.0f, 10.0f, 500.0f);
            ImGui::ColorEdit4("Color", &block.color.x);
            
            const char* typeNames[] = {"Start", "Process", "Decision", "End"};
            int currentType = static_cast<int>(block.type);
            if (ImGui::Combo("Type", &currentType, typeNames, 4)) {
                block.type = static_cast<Diagram::BlockType>(currentType);
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

