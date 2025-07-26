# Negentropy

**Interactive diagram editor with camera controls and real-time block manipulation**

## Features

- **Visual Editor**: Create and edit flowchart diagrams with drag-and-drop blocks
- **Block Types**: Start, Process, Decision, and End blocks with customizable properties  
- **Camera System**: Pan and zoom with mouse controls for large diagrams
- **Real-time UI**: ImGui-based properties panel for live editing
- **File Management**: Save/load diagrams as XML workspace files
- **Cross-platform**: Built with SDL2 for Windows/Linux/macOS support

## Quick Start

```bash
# Build
mkdir build && cd build
cmake ..
make

# Run
./negentropy
```

## Controls

- **Middle mouse drag**: Pan camera view
- **Mouse wheel**: Zoom in/out  
- **Left click**: Select and drag blocks
- **Menu → File → Load**: Open saved diagrams and load them
- **Menu → File → Save**: Save the current diagram to workspace
- **Menu → File → Exit**: Exit the application
- **Menu → View → Properties**: Toggle editing panel
- **Menu → View → Demo**: Open ImGui demo window

## Dependencies

- SDL2 (graphics/input)
- ImGui (UI framework)
- pugixml (XML serialization)
- GLM (math library)
- spdlog (logging)
- magic_enum (enum reflection)
- EnTT (entity component system)

## Architecture

**Core/Main/**: Application lifecycle, rendering and event handling  
**Core/Diagram/**: Block and camera data structures  
**Core/Utils/**: Utility functions for workspace paths  
**Workspace/**: Saved diagram files (.xml format)

Built with modern C++20 and static event handling system for efficient diagram manipulation.