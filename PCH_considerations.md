# Precompiled headers (PCH) considerations in Target Layout

This document outlines architectural and performance considerations when integrating precompiled headers into a Target Layout project. It serves as reference material for build optimizations and potential inclusion in an extended project template (`target_layout_extended`).

---

## 1. Overview

Because every component in Target Layout is an ordinary, first-class CMake target, CMake's native `target_precompile_headers` command (introduced in CMake 3.16) works out of the box.

However, how PCH is applied matters significantly for both build performance and dependency graph integrity.

---

## 2. Why linking to a PCH target is an anti-pattern

A natural first thought might be to define a `pch_common` component in `srcTargets/` and have other components link against it via `target_link_libraries`. In CMake and Target Layout, this introduces several problems:

- **Redundant recompilation:** If an `INTERFACE` target defines precompiled headers and other targets link to it, CMake by default compiles a fresh, separate PCH binary for every consuming target. In a project with 30 components, the PCH is compiled 30 times, degrading clean build times.
- **Architectural graph pollution:** In Target Layout, the link graph reflects the physical software architecture and domain dependencies (for example, `report` depends on `geometry`). Linking every component to `pch_common` adds dozens of artificial edges to dependency analysis and Graphviz (`.dot`) architectural exports.
- **Violation of component identity:** A Target Layout component has a strict 1-to-1 structure (`<name>.cpp`, `<name>/<name>.hpp`, and `<name>TEST/`). A PCH artifact has no public domain interface, no translation unit logic, and no unit test.

---

## 3. The recommended approach: `REUSE_FROM`

To compile the precompiled header binary **exactly once** and share it across multiple components without altering their domain link graphs, modern CMake provides the `REUSE_FROM` keyword.

### Step 1: Define the PCH target at the build level

Define a concrete static target outside `srcTargets/` (for example, in the root `CMakeLists.txt` or a dedicated `Build/` script):

```cmake
# Create a dedicated build target to compile the shared PCH once
add_library(project_pch STATIC "${CMAKE_BINARY_DIR}/pch_empty.cpp")
file(TOUCH "${CMAKE_BINARY_DIR}/pch_empty.cpp")

target_precompile_headers(project_pch
    PUBLIC
        <algorithm>
        <memory>
        <string>
        <string_view>
        <vector>)
```

### Step 2: Attach to components

#### Option A: Explicit opt-in per component
Inside individual components that benefit from heavy standard library or third-party includes:

```cmake
target_precompile_headers(${DIR_NAME} REUSE_FROM project_pch)
```

#### Option B: Automatic registration across all components
In `srcTargets/CMakeLists.txt`, automatically apply `REUSE_FROM` to all discovered component targets:

```cmake
file(GLOB COMPONENT_DIRS LIST_DIRECTORIES true CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(DIR ${COMPONENT_DIRS})
    if(IS_DIRECTORY ${DIR})
        cmake_path(GET DIR FILENAME DIR_NAME)
        add_subdirectory(${DIR})

        # Reuse precompiled headers if the target exists
        if(TARGET ${DIR_NAME})
            target_precompile_headers(${DIR_NAME} REUSE_FROM project_pch)
        endif()
    endif()
endforeach()
```

---

## 4. Overhead analysis for components that do not use PCH headers

When PCH is enabled project-wide, some components will not use any of the headers included in the PCH. The impact breaks down as follows:

### Compilation time and memory (negligible)
- **Memory mapping:** Compilers (Clang, GCC, MSVC) do not re-parse the PCH for each file. They memory-map the pre-parsed AST directly from disk, which adds roughly 5–20 ms per translation unit.
- **Process memory:** The compiler process uses an additional 50–150 MB of RAM while compiling to hold the preloaded symbol table.
- **Net speed:** For a minimal component with no dependencies, compilation time might increase marginally (for example, from 30 ms to 45 ms). For any component that uses standard containers or algorithms, the PCH saves hundreds of milliseconds.

### Binary and object code size (zero overhead)
- **No dead code generation:** C++ standard library headers consist primarily of templates, type declarations, and inline functions. If a translation unit does not instantiate or call them, the compiler emits zero machine instructions or data bytes into the object file (`.o` / `.obj`).
- **Debug symbols:** Modern debug formats (such as DWARF with default compiler optimizations) discard unused type definitions, keeping object files lean.

### Cold-build serialization bottleneck
- When using `REUSE_FROM project_pch`, every component depends on `project_pch`.
- During a completely clean build on high-core machines (16–32 cores), all compilation jobs must wait for `project_pch` to compile once (roughly 1–2 seconds) before running in parallel.
- Once that initial target is built, all workers resume in parallel at full capacity.

### Semantic overhead and dependency masking (primary risk)
- **Hidden missing includes:** Because PCH injects headers into every translation unit, a component might compile successfully even if it forgets to `#include <string>`. If that component is later isolated or moved to another project, it will fail to compile.
- **Symbol and macro pollution:** If headers defining preprocessor macros (such as legacy system headers or headers defining `min`/`max`) are placed in the PCH, they contaminate every component's translation unit.

---

## 5. Guidelines and best practices

1. **Include what you use (IWYU):** Always write explicit `#include` directives in component headers and source files, treating PCH strictly as a compiler acceleration technique rather than an implicit include provider.
2. **Strictly stable headers only:** Only include stable, macro-free standard library headers (`<vector>`, `<string>`, `<algorithm>`, `<memory>`) or frozen third-party library headers. Never include internal component headers in a shared PCH, as modifying an internal header would trigger a full rebuild of every component in the repository.
3. **Periodic clean builds without PCH:** Maintain a CI job or build preset with PCH disabled to ensure all components remain fully self-contained.
