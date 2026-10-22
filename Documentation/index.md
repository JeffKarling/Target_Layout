[Release Tag Page](https://github.com/JeffKarling/Target_Layout/tags) | [GitHub Repository](https://github.com/JeffKarling/Target_Layout)

# How It Works
Target Layout is a C++ project organization philosophy and build system configuration that applies Lakos' physical component model and the **convention over configuration** ergonomics of Rust's Cargo workspaces.
### Philosophy

In *Large Scale C++, Volume I* (2019) and *Large Scale C++ Software Design* (1996), John Lakos addressed two architectural issues that cause C++ projects to fail to scale.

* **Logical design** covers classes, functions, and patterns, governing API contracts and semantic relationships. Logical design alone cannot prevent architectural decay.

* **Physical design** covers files, directories, translation units, link dependencies, dictating compilation times, linking feasibility, reusability, cognitive load, and testability. Unmanaged physical dependencies quickly introduce a cognitive burden and impede development velocity. Physical design determines system viability and development velocity.


### Lineage
#### The component model (*Physical Decomposition*)

Lakos established that in scalable architecture, the **component is the atomic physical unit**. A component such as `foo` consists of three parts.

* The public contract in `foo.h` provides declarations necessary for clients to use the component. 
* The private implementation in `foo.cpp` holds definitions and internal state hidden from clients. 
* The dedicated test driver `fooTEST.cpp` exercises `foo` in complete isolation from client code.

And crucially, components depend on components rather than arbitrary loose files. Every header belongs to a component with no loose utility headers floating without physical ownership. And the test driver lives inside each component's directory rather than in a distant disconnected hierarchy.

The centerpiece of this architecture is the **acyclic dependency graph (DAG)**, where dependencies between components must never form cycles. If component `Foo` depends on component `Bar`, `Bar` must never directly or transitively depend on `Foo`. This hierarchy ensures that components can be understood, built, and tested independently from the bottom up.

#### The Pitchfork Layout / PFL (*filesystem cataloging*)

Proposed in 2018 to give C++ a common repository taxonomy, the Pitchfork Layout (PFL) standardized top level folder names (`include/`, `src/`, `tests/`, `data/`, `libs/`). However, PFL is essentially a **filesystem cataloging guideline**, not an architectural or build system contract. It allows multiple permutations (*separate* vs. *merged* headers, nested `libs/`) and groups files by *artifact extension* rather than compilation boundaries. In PFL boundaries are defined by folder names alone with some compiler enforced boundaries. And because it is a convention and not a build system contract, it can be ignored in favor of something else, and often is.

#### Rust workspace (*convention over configuration*)
Rust's Cargo succefully demonstrated that the **filesystem layout is the build configuration** implementation. A workspace consisting of components in self contained directories describing the package's dependencies in a TOML file gave rise to architectural simplicity, compile-time boundary enforcement that significantly improved development velocity.

**Target Layout** implements above into a CMake build system. By using the Lakos component (`.hpp` + `.cpp` + test) making it **literally** a self contained CMake target in a single directory where the filesystem, the build graph, the C++ namespace, and the test all agree on **one name**.

## 2. The rule

Every component is described by a single name. Consider a component named `text`.

| Property | Value |
| --- | --- |
| Directory | `srcTargets/text/` |
| CMake target | `text` |
| Alias consumers use | `Target_Layout::text` |
| Translation unit | `text.cpp` |
| Public header | `text/text.hpp`, included as `<text/text.hpp>` |
| C++ namespace | `text::` |
| Test directory, target, and source | `textTEST/`, `textTEST`, `textTEST.cpp` |

Because every path is derivable from the name, there is nothing to look up. You do not search for where `text` lives, you already know.

## 3. Anatomy of a component

```cmake
cmake_path(GET CMAKE_CURRENT_LIST_DIR FILENAME DIR_NAME)
```
The component's name is taken from its **directory**. This is the only place the name is "defined", and it is defined by the filesystem.

```cmake
add_library(${DIR_NAME} STATIC)
add_library(${PROJECT_NAME}::${DIR_NAME} ALIAS ${DIR_NAME})
```
The component **is** a target. The namespaced alias means a typo in a consumer's `target_link_libraries` is a configure time error instead of a silent link to a library named `text`.

```cmake
target_sources(${DIR_NAME}
        PRIVATE ${DIR_NAME}.cpp
        PUBLIC  FILE_SET ${DIR_NAME} TYPE HEADERS
                FILES ${DIR_NAME}/${DIR_NAME}.hpp)
```
The translation unit is private. The header is declared as a `FILE_SET` (CMake 3.23+), which is why this project requires 3.23. The set's base directory is the component directory, so consumers write `#include <text/text.hpp>`, where the inner `text/` folder serves as the include prefix.

```cmake
if(BUILD_TESTING)
    add_subdirectory(${DIR_NAME}TEST)
endif()
```
The test is registered by the component itself.

### Header only components (`INTERFACE`)

If a component consists entirely of templates or inline declarations without a `.cpp` file, use CMake's `INTERFACE` library type. The structure remains completely identical.

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

Even though the library is header only, its dedicated unit test (`textTEST`) lives alongside it, ensuring that template instantiations compile cleanly and pass automated tests.

## 4. Auto registration

The `srcTargets/CMakeLists.txt` file adds every subdirectory.

```cmake
file(GLOB COMPONENT_DIRS LIST_DIRECTORIES true CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(DIR ${COMPONENT_DIRS})
    if(IS_DIRECTORY ${DIR})
        add_subdirectory(${DIR})
    endif()
endforeach()
```

Adding a component never edits this file, the root `CMakeLists.txt`, or any other component. `CONFIGURE_DEPENDS` makes the build rerun configure when a directory appears.

Order does not matter because CMake resolves `target_link_libraries` to targets that are defined later.

## 5. The link graph is the architecture

The `report` target is the one component with dependencies and illustrates how visibilities are declared.

```cmake
target_link_libraries(${DIR_NAME}
        PUBLIC  ${PROJECT_NAME}::geometry # report.hpp exposes geometry::Rect
        PRIVATE ${PROJECT_NAME}::text)    # only report.cpp uses text
```

Here `geometry` is **PUBLIC** because `report.hpp` includes `<geometry/geometry.hpp>`, meaning anyone using `report` needs `geometry` headers. In contrast, `text` is **PRIVATE** as an internal implementation detail, so anything that links `report` does not receive `text` headers. If a component is header only (`INTERFACE`), any dependencies exposed to consumers are declared with **`INTERFACE`**.

```cmake
target_link_libraries(${DIR_NAME} INTERFACE ${PROJECT_NAME}::geometry)
```

This is enforced by the compiler. When `app` links only `report` and tries to use an unlinked component, compilation fails.

```cpp
#include <text/text.hpp>
```
```text
app.cpp:1:10: fatal error: text/text.hpp: No such file or directory
```

A component that links nothing (like `text` and `geometry`) cannot include anything from another component, so layering violations are compile errors instead of code review comments.

Because a component's public surface is strictly limited to `<name>/<name>.hpp` (`PUBLIC FILE_SET`) and its `PUBLIC` link dependencies, everything else inside `srcTargets/<name>/`, including the translation unit, internal helper files, and `PRIVATE` dependencies, is hidden behind a hard compilation wall. Therefore, **component deepening** (keeping public interfaces narrow while hiding implementation complexity) is tangible at the filesystem and build level. Also, it enables easy inspection of the strategic architecture of the codebase by reading `CMakeLists.txt` and `<name>/<name>.hpp` without wading through every line of `.cpp` code.

## 6. Tests reside with their component

The `textTEST/` directory follows the same rule one level down. Its directory name is its target name, and it links the component it is named after.

```cmake
cmake_path(GET CMAKE_CURRENT_LIST_DIR FILENAME DIR_NAME)    # textTEST
string(REGEX REPLACE "TEST$" "" COMPONENT_NAME ${DIR_NAME}) # text

add_executable(${DIR_NAME} ${DIR_NAME}.cpp)
target_link_libraries(${DIR_NAME} PRIVATE ${PROJECT_NAME}::${COMPONENT_NAME})
add_test(NAME ${DIR_NAME} COMMAND ${DIR_NAME})
```

The tests here are plain executables that return a non zero exit code on failure, so the project has no test framework dependency. Use GoogleTest or Catch2 in a real project, the layout does not care.

## 7. Executables are components

`app/` follows the same rule (`add_executable(${DIR_NAME} ${DIR_NAME}.cpp)`) and links `${PROJECT_NAME}::report`. It has no public header because nothing links against it.

## 8. Adding a component

Because every component is self contained and structurally identical, creating a new component involves zero central configuration. 

You do not need to touch the root `CMakeLists.txt`, parent folders, or any central registry. Instead, you can use any existing component directory as a complete, pre wired scaffolding template.

* **Self contained scaffolding.** An existing component provides a working `CMakeLists.txt`, matching public header directory (`<name>/<name>.hpp`), and a dedicated test target (`<name>TEST`).

* **Automatic discovery.** The top level `srcTargets/CMakeLists.txt` automatically detects new directories via `file(GLOB ... CONFIGURE_DEPENDS)`. The new target, its alias `${PROJECT_NAME}::<name>`, and its test appear on the next build without editing parent scripts.

* **Zero central friction.** As explained in the [FAQ (*"Isn't one CMakeLists.txt per component a lot?"*)](faq.md#isnt-one-cmakeliststxt-per-component-a-lot), because each component's build file is minimal and deduces its own target name from the filesystem, duplicating or scaffolding components introduces no administrative overhead across multiple trees.

## 9. What this does and does not enforce

Enforced by the compiler and build system:

* **No undeclared dependencies.** A component cannot use another component unless it explicitly links it via `target_link_libraries`.

* **Transitive exposure control.** `PRIVATE` dependencies remain strictly internal to the `.cpp` implementation, `PUBLIC` dependencies propagate headers to consumers, and `INTERFACE` dependencies propagate requirements from header only components without compiling a binary.

Conventions not enforced by the build system:

* **The naming rule itself.** Nothing stops someone adding `srcTargets/foo/` with an unrelated target name. The one to one convention is what keeps the codebase predictable and navigable, while shared templates and code reviews keep it consistent.
