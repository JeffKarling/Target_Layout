# How It Works

## 1. Lineage & Philosophy

John Lakos defined the *physical component* (a header and its implementation) as the atomic unit of physical design in *Large-Scale C++ Software Design*. Layouts such as the Pitchfork Layout (PFL) build on that vocabulary.

Target Layout is a personal extension of that line of thinking, not an official one: it takes the physical component and makes it **literally** a CMake target, with the filesystem, the build graph and the namespace all agreeing on one name.

## 2. The rule

Every component is described by a single name. For a component named `text`:

| What | Name |
| :--- | :--- |
| Directory | `srcTargets/text/` |
| CMake target | `text` |
| Alias consumers use | `Target_Layout::text` |
| Translation unit | `text.cpp` |
| Public header | `text/text.hpp`, included as `<text/text.hpp>` |
| C++ namespace | `text::` |
| Test directory, target and source | `textTEST/`, `textTEST`, `textTEST.cpp` |

Because every path is derivable from the name, there is nothing to look up: you do not search for where `text` lives, you already know.

## 3. Anatomy of a component

```cmake
cmake_path(GET CMAKE_CURRENT_LIST_DIR FILENAME DIR_NAME)
```
The component's name is taken from its **directory**. This is the only place the name is "defined", and it is defined by the filesystem.

```cmake
add_library(${DIR_NAME} STATIC)
add_library(${PROJECT_NAME}::${DIR_NAME} ALIAS ${DIR_NAME})
```
The component **is** a target. The namespaced alias means a typo in a consumer's `target_link_libraries` is a configure-time error instead of a silent link to a library named `text`.

```cmake
target_sources(${DIR_NAME}
        PRIVATE ${DIR_NAME}.cpp
        PUBLIC  FILE_SET ${DIR_NAME} TYPE HEADERS
                FILES ${DIR_NAME}/${DIR_NAME}.hpp)
```
The translation unit is private. The header is declared as a `FILE_SET` (CMake 3.23+), which is why this project requires 3.23. The set's base directory is the component directory, so consumers write `#include <text/text.hpp>`: the inner `text/` folder is the include prefix.

```cmake
if(BUILD_TESTING)
    add_subdirectory(${DIR_NAME}TEST)
endif()
```
The test is registered by the component itself.

### Header-only components (`INTERFACE`)

If a component consists entirely of templates or inline declarations without a `.cpp` file, use CMake's `INTERFACE` library type. The structure remains completely identical:

```cmake
cmake_path(GET CMAKE_CURRENT_LIST_DIR FILENAME DIR_NAME)

add_library(${DIR_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${DIR_NAME} ALIAS ${DIR_NAME})

target_sources(${DIR_NAME}
        INTERFACE FILE_SET ${DIR_NAME} TYPE HEADERS
                  FILES ${DIR_NAME}/${DIR_NAME}.hpp)

if(BUILD_TESTING)
    add_subdirectory(${DIR_NAME}TEST)
endif()
```

Even though the library is header-only, its dedicated unit test (`textTEST`) lives alongside it, ensuring that template instantiations compile cleanly and pass automated tests.

## 4. Auto-registration

`srcTargets/CMakeLists.txt` adds every subdirectory:

```cmake
file(GLOB COMPONENT_DIRS LIST_DIRECTORIES true CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(DIR ${COMPONENT_DIRS})
    if(IS_DIRECTORY ${DIR})
        add_subdirectory(${DIR})
    endif()
endforeach()
```

Adding a component never edits this file, the root `CMakeLists.txt`, or any other component. `CONFIGURE_DEPENDS` makes the build re-run configure when a directory appears.

Order does not matter: CMake resolves `target_link_libraries` to targets that are defined later.

## 5. The link graph is the architecture

`report` is the one component with dependencies, and shows how visibilities are declared:

```cmake
target_link_libraries(${DIR_NAME}
        PUBLIC  ${PROJECT_NAME}::geometry # report.hpp exposes geometry::Rect
        PRIVATE ${PROJECT_NAME}::text)    # only report.cpp uses text
```

- `geometry` is **PUBLIC** because `report.hpp` includes `<geometry/geometry.hpp>`. Anyone using `report` needs `geometry`'s headers.
- `text` is **PRIVATE**: an implementation detail. Anything that links `report` does *not* get `text`'s headers.
- If a component is header-only (`INTERFACE`), any dependencies exposed to consumers are declared with **`INTERFACE`**:
  ```cmake
  target_link_libraries(${DIR_NAME} INTERFACE ${PROJECT_NAME}::geometry)
  ```

This is enforced by the compiler. If `app` (which links only `report`) tries to use an unlinked component:

```cpp
#include <text/text.hpp>
```
```text
app.cpp:1:10: fatal error: text/text.hpp: No such file or directory
```

A component that links nothing (like `text` and `geometry`) cannot include anything from another component, so layering violations are compile errors instead of code-review comments.

Because a component's public surface is strictly limited to `<name>/<name>.hpp` (`PUBLIC FILE_SET`) and its `PUBLIC` link dependencies, everything else inside `srcTargets/<name>/`—the translation unit, internal helper files, and `PRIVATE` dependencies—is hidden behind a hard compilation wall. This makes **component deepening** (keeping public interfaces narrow while hiding implementation complexity) tangible at the filesystem and build level, and lets you inspect the strategic architecture of the codebase by reading `CMakeLists.txt` and `<name>/<name>.hpp` without wading through every line of `.cpp` code.

In larger projects, CMake's built-in Graphviz (`.dot`) generation turns this link graph directly into a queryable dependency database with minimal setup. For AI agents, reading the plain-text `.dot` file is a high-leverage, token-saving shortcut to grasp the full component graph without crawling every directory; for humans, rendering it into an image provides an immediate visual mental map of component layers and `PUBLIC` vs. `PRIVATE` boundaries during design and review.

## 6. Tests live with the component

`textTEST/` follows the same rule one level down: its directory name is its target name, and it links the component it is named after:

```cmake
cmake_path(GET CMAKE_CURRENT_LIST_DIR FILENAME DIR_NAME)    # textTEST
string(REGEX REPLACE "TEST$" "" COMPONENT_NAME ${DIR_NAME}) # text

add_executable(${DIR_NAME} ${DIR_NAME}.cpp)
target_link_libraries(${DIR_NAME} PRIVATE ${PROJECT_NAME}::${COMPONENT_NAME})
add_test(NAME ${DIR_NAME} COMMAND ${DIR_NAME})
```

The tests here are plain executables that return a non-zero exit code on failure, so the project has no test-framework dependency. Use GoogleTest or Catch2 in a real project: the layout does not care.

## 7. Executables are components too

`app/` follows the same rule (`add_executable(${DIR_NAME} ${DIR_NAME}.cpp)`) and links `${PROJECT_NAME}::report`. It has no public header because nothing links against it.

## 8. Adding a component

Because every component is self-contained and structurally identical, creating a new component involves zero central configuration. 

You do not need to touch the root `CMakeLists.txt`, parent folders, or any central registry. Instead, you can use any existing component directory as a complete, pre-wired scaffolding template:

- **Self-contained scaffolding:** An existing component provides a working `CMakeLists.txt`, matching public header directory (`<name>/<name>.hpp`), and a dedicated test target (`<name>TEST`).
- **Automatic discovery:** The top-level `srcTargets/CMakeLists.txt` automatically detects new directories via `file(GLOB ... CONFIGURE_DEPENDS)`. The new target, its alias `${PROJECT_NAME}::<name>`, and its test appear on the next build without editing parent scripts.
- **Zero central friction:** As explained in the FAQ (*"Isn't one CMakeLists.txt per component a lot?"*), because each component's build file is minimal and deduces its own target name from the filesystem, duplicating or scaffolding components introduces no administrative overhead across multiple trees.

## 9. What this does and does not enforce

Being precise matters more than sounding impressive.

### Enforced by the compiler & build system

- **No undeclared dependencies:** A component cannot use another component unless it explicitly links it via `target_link_libraries`.
- **Transitive exposure control:**
  - **`PRIVATE`**: Dependencies remain strictly internal to the `.cpp` implementation.
  - **`PUBLIC`**: Dependencies propagate headers to consumers.
  - **`INTERFACE`**: Dependencies propagate requirements from header-only components without compiling a binary.

### Conventions (not policed by the build system)

- **The naming rule itself:** Nothing stops someone adding `srcTargets/foo/` with an unrelated target name. The 1-to-1 convention is what keeps the codebase predictable and navigable; shared templates and code reviews keep it consistent.
