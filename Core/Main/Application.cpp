#include "Application.hpp"
#include <stdexcept>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <filesystem>
#include "../Utils/Path.hpp"

namespace fs = std::filesystem;

Application::Application() {
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
        RenderFrame();
    }
}

void Application::InitializeImGui() const {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    DarkStyle();
    SetupFont();
    
    ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer.GetSDLRenderer());
    ImGui_ImplSDLRenderer2_Init(m_renderer.GetSDLRenderer());

    spdlog::info("ImGui initialized");
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
            int w, h;
            SDL_GetWindowSize(m_window, &w, &h);
            const glm::vec2 screenSize{static_cast<float>(w), static_cast<float>(h)};
            EventHandler::HandleEvent(event, m_diagramData.GetCamera(), m_diagramData.GetComponents(), screenSize);
        }
    }
}


void Application::RenderFrame() noexcept {
    RenderUI();
    
    m_renderer.Clear();
    m_renderer.DrawGrid(m_diagramData.GetCamera(), m_diagramData.GetGrid());
    m_renderer.DrawComponents(m_diagramData.GetComponents(), m_diagramData.GetCamera());

    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_renderer.GetSDLRenderer());

    m_renderer.Present();
}

void Application::RenderUI() noexcept {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 3.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 3.0f));
    
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::BeginMenu("Load")) {
                for (const auto& file : m_workspaceFiles) {
                    if (ImGui::MenuItem(file.c_str())) {
                        m_diagramData.Load((Utils::GetWorkspacePath() / file).string());
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Save")) {
                m_diagramData.Save((Utils::GetWorkspacePath() / "Default.xml").string());
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
            ImGui::MenuItem("Component Tree", nullptr, &m_showComponentTree);
            ImGui::MenuItem("Component Editor", nullptr, &m_showComponentEditor);
            ImGui::MenuItem("Demo", nullptr, &m_showDemo);
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
    
    ImGui::PopStyleVar(2);
    
    if (m_showProperties) {
        RenderPropertiesPanel();
    }
    
    if (m_showComponentTree) {
        Diagram::Component::RenderComponentTree(m_diagramData.GetComponents());
    }
    
    if (m_showComponentEditor) {
        Diagram::Component::RenderComponentEditor();
    }
    
    if (m_showDemo) {
        ImGui::ShowDemoWindow(&m_showDemo);
    }
}

void Application::RenderPropertiesPanel() noexcept {
    ImGui::Begin("Properties", &m_showProperties);
    
    auto& components = m_diagramData.GetComponents();
    auto& camera = m_diagramData.GetCamera();
    
    size_t blockCount = std::count_if(components.begin(), components.end(),
        [](const auto& comp) { return dynamic_cast<Diagram::Block*>(comp.get()) != nullptr; });
    
    ImGui::Text("Camera: (%.1f, %.1f) Zoom: %.2f", camera.data.position.x, camera.data.position.y, camera.data.zoom);
    ImGui::Text("Blocks: %zu", blockCount);
    
    if (ImGui::Button("Add Block")) {
        auto newBlock = std::make_unique<Diagram::Block>();
        newBlock->data.position = {100.0f + static_cast<float>(blockCount + 1) * 150.0f, 100.0f};
        newBlock->data.label = "Block " + std::to_string(blockCount + 1);
        components.push_back(std::move(newBlock));
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

void Application::DarkStyle() noexcept {
    auto& style = ImGui::GetStyle();
    auto& colors = style.Colors;
    
    colors[ImGuiCol_Text]                = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]        = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]            = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg]             = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
    colors[ImGuiCol_PopupBg]             = ImVec4(0.12f, 0.12f, 0.14f, 0.98f);
    colors[ImGuiCol_Border]              = ImVec4(0.25f, 0.25f, 0.26f, 1.00f);
    colors[ImGuiCol_BorderShadow]        = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]             = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]      = ImVec4(0.19f, 0.19f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBgActive]       = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_TitleBg]             = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgActive]       = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]    = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
    colors[ImGuiCol_MenuBarBg]           = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]         = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]       = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
    colors[ImGuiCol_CheckMark]           = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
    colors[ImGuiCol_SliderGrab]          = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]    = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]              = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_ButtonHovered]       = ImVec4(0.17f, 0.17f, 0.19f, 1.00f);
    colors[ImGuiCol_ButtonActive]        = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
    colors[ImGuiCol_Header]              = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_HeaderHovered]       = ImVec4(0.17f, 0.17f, 0.19f, 1.00f);
    colors[ImGuiCol_HeaderActive]        = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
    colors[ImGuiCol_Separator]           = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]    = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    colors[ImGuiCol_SeparatorActive]     = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
    colors[ImGuiCol_ResizeGrip]          = ImVec4(0.00f, 0.47f, 0.84f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]   = ImVec4(0.00f, 0.47f, 0.84f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]    = ImVec4(0.00f, 0.47f, 0.84f, 0.95f);
    colors[ImGuiCol_Tab]                 = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
    colors[ImGuiCol_TabHovered]          = ImVec4(0.17f, 0.17f, 0.19f, 1.00f);
    colors[ImGuiCol_TabActive]           = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_TabUnfocused]        = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]  = ImVec4(0.11f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_PlotLines]           = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]    = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]       = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]       = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]   = ImVec4(0.25f, 0.25f, 0.26f, 1.00f);
    colors[ImGuiCol_TableBorderLight]    = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TableRowBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]       = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_TextSelectedBg]      = ImVec4(0.00f, 0.47f, 0.84f, 0.35f);
    colors[ImGuiCol_DragDropTarget]      = ImVec4(1.00f, 0.80f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]        = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]   = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]    = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);

    // Compact VS Code Style Settings
    style.WindowPadding        = ImVec2(5.0f, 5.0f);
    style.FramePadding         = ImVec2(6.0f, 3.0f);
    style.CellPadding          = ImVec2(3.0f, 2.0f);
    style.ItemSpacing          = ImVec2(5.0f, 3.0f);
    style.ItemInnerSpacing     = ImVec2(3.0f, 2.0f);
    style.TouchExtraPadding    = ImVec2(0.0f, 0.0f);


    style.IndentSpacing        = 12.0f;
    style.ScrollbarSize        = 12.0f;
    style.GrabMinSize          = 8.0f;

    // Borders
    style.WindowBorderSize     = 1.0f;
    style.ChildBorderSize      = 1.0f;
    style.PopupBorderSize      = 1.0f;
    style.FrameBorderSize      = 0.0f;
    style.TabBorderSize        = 0.0f;

    // Rounding
    style.WindowRounding       = 2.0f;
    style.ChildRounding        = 4.0f;
    style.FrameRounding        = 2.0f;
    style.PopupRounding        = 4.0f;
    style.ScrollbarRounding    = 9.0f;
    style.GrabRounding         = 2.0f;
    style.LogSliderDeadzone    = 4.0f;
    style.TabRounding          = 4.0f;

    // Alignment
    style.WindowTitleAlign     = ImVec2(0.00f, 0.50f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign      = ImVec2(0.50f, 0.50f);
    style.SelectableTextAlign  = ImVec2(0.00f, 0.00f);

    // Safe Area Padding
    style.DisplaySafeAreaPadding = ImVec2(3.0f, 3.0f);
}

void Application::SetupFont() noexcept {
    ImGuiIO& io = ImGui::GetIO();
    
    const char* fontPaths[] = {
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Windows/Fonts/arial.ttf",
        "/Windows/Fonts/segoeui.ttf"
    };
    
    for (const char* fontPath : fontPaths) {
        if (std::filesystem::exists(fontPath)) {
            ImFontConfig config;
            config.OversampleH = 3;
            config.OversampleV = 2;
            config.PixelSnapH = true;
            io.Fonts->AddFontFromFileTTF(fontPath, 12.0f, &config);
            return;
        }
    }
    
    ImFontConfig config;
    config.SizePixels = 14.0f;
    config.OversampleH = 3;
    config.OversampleV = 2;
    config.PixelSnapH = true;
    io.Fonts->AddFontDefault(&config);
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
