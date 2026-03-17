## Build & Run

### Building the GDExtension

The project uses CMake to build the C++ GDExtension. You need Godot 4.6 headers to compile.

```bash
# Set Godot include path (adjust to your Godot installation)
export GODOT_INCLUDE_DIRS="/path/to/godot-4.6/bin/include"
export GODOT_LIBRARIES="/path/to/godot-4.6/bin/libgodot-cpp.a"

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DGODOT_INCLUDE_DIRS="$GODOT_INCLUDE_DIRS" \
  -DGODOT_LIBRARIES="$GODOT_LIBRARIES"

cmake --build build --config Release
```

### Running in Godot

1. Copy the built extension to your Godot project's `addons/` directory
2. Open the project in Godot 4.6+
3. The `FallingSandSimulation` node will be available

## Validation

Run these after implementing to get immediate feedback:

- **GDScript Tests:** Open `res://tests/TestRunner.tscn` in Godot and run the scene. Tests auto-execute on load and print `[PASS]`/`[FAIL]` to the Output panel.
- **C++ Compilation:** Verify CMake configuration: `cmake -B build -DCMAKE_BUILD_TYPE=Debug -DGODOT_INCLUDE_DIRS=/path/to/headers .`

## Operational Notes

The GDExtension requires:
- Godot 4.6 (or compatible version)
- C++17 compiler
- CMake 3.15+

### Codebase Patterns

- C++ source in `src/`
- GDScript bindings in `src/godot_extension/`
- Shaders in `shaders/`
- Tests in `tests/`
