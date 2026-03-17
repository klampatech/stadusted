## Build & Run

### Building the GDExtension

The project uses CMake to build the C++ GDExtension. You need godot-cpp 4.5+ to compile.

```bash
# Clone and build godot-cpp (required for compilation)
cd /tmp
git clone https://github.com/godotengine/godot-cpp.git
cd godot-cpp
scons -j8 platform=macos target=template_release arch=universal

# Build the stardust extension
cd /path/to/stadusted
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DGODOT_INCLUDE_DIRS="/tmp/godot-cpp/include;/tmp/godot-cpp/gdextension;/tmp/godot-cpp/gen/include" \
  -DGODOT_LIBRARIES="/tmp/godot-cpp/bin/libgodot-cpp.macos.template_release.universal.a"

cmake --build build --config Release

# Set up the addon for Godot
mkdir -p addons/godot_falling_sand
cp build/libgodot_falling_sand.dylib addons/godot_falling_sand/
cp extension.gdextension addons/godot_falling_sand/
```

### Running in Godot

1. Install Godot 4.6+ from https://godotengine.org/
2. Open the project in Godot
3. The `FallingSandSimulation` node will be available as a new node type

### Running Tests

```bash
# Run the test scene in headless Godot
/Applications/Godot.app/Contents/MacOS/Godot --headless --path . --quit
```

Note: The test runner outputs to the Godot console.

## Validation

Run these after implementing to get immediate feedback:

- **GDScript Tests:** Open `res://tests/TestRunner.tscn` in Godot and run the scene. Tests auto-execute on load and print `[PASS]`/`[FAIL]` to the Output panel.
- **C++ Compilation:** Verify CMake configuration: `cmake -B build -DCMAKE_BUILD_TYPE=Debug -DGODOT_INCLUDE_DIRS=/path/to/headers .`

## Operational Notes

The GDExtension requires:
- Godot 4.6+ (compatible version)
- C++17 compiler
- CMake 3.15+

### Codebase Patterns

- C++ source in `src/`
- GDExtension bindings in `src/godot_extension/`
- Shaders in `shaders/`
- Tests in `tests/`
- Built addon in `addons/godot_falling_sand/`
