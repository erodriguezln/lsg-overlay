# SDL3 CMake Boilerplate

Scaffold for SDL3 projects with Dear ImGui and automatic dependency resolution via CMake FetchContent.

## Requirements

- CMake ≥ 3.30
- C++20 capable compiler (MSVC)
- Internet connection on first build (SDL3 and ImGui downloaded automatically; other libraries can be enabled in CMakeLists.txt)

## Build

### Using CMake Presets (Recommended)

To configure and build the project using CMake Presets (MSVC + Visual Studio 2026):

1. **List the available presets**:
   ```bash
   cmake --list-presets
   ```

2. **Configure the project**:
   ```bash
   # Configure Debug build
   cmake --preset windows-msvc-debug

   # Configure Release build
   cmake --preset windows-msvc-release
   ```

3. **Build the project**:
   ```bash
   # Build Debug
   cmake --build --preset windows-msvc-debug

   # Build Release
   cmake --build --preset windows-msvc-release
   ```

The compiled binary and its assets will be copied to `build/msvc-debug/` or `build/msvc-release/`.

### Manual Build

Alternatively, you can build manually using the default compiler and generator:

```bash
cmake -B build
cmake --build build
```

The binary is placed in `build/`.