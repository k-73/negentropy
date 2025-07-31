# Negentropy

## Quick Start

```bash
# Build
mkdir cmake-build-debug && cd cmake-build-debug
cmake ..
make

# Run
./negentropy
```

## WebAssembly Build

```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest
source ./emsdk_env.sh

# Build for web
cd /path/to/negentropy
mkdir build-wasm && cd build-wasm
emcmake cmake ..
emmake make

# Run in browser
python3 -m http.server 8000
# Open http://localhost:8000/negentropy.html
```

## Controls

## Dependencies

- SDL2 (graphics/input)
- ImGui (UI framework)
- pugixml (XML serialization)
- GLM (math library)
- spdlog (logging)
- magic_enum (enum reflection)
- EnTT (entity component system)
