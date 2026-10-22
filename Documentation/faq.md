# FAQ

## Where are `include/` and `src/`?

There are none, on purpose. Those directories group files by *kind*. Target Layout groups them by *component*, where everything about `text` (header, source, test, CMake) is in one folder.

| Task | Conventional `include/` + `src/` + `tests/` | Target Layout |
| --- | --- | --- |
| Add a component | create a header, a source, a test in 3 trees; edit CMake lists | copy a folder, rename |
| Rename a component | touch 3 trees plus CMake | rename the folder and its same named files |
| Delete a component | hunt down files in 3 trees plus CMake | delete the folder (and links to it) |
| "Where does `text` live?" | search | `srcTargets/text/` |

## Why is there an inner directory for public headers?

Public headers live inside a nested directory matching the component name (for example, `srcTargets/text/text/text.hpp`). This is intentional and serves two purposes.

**Explicit namespacing and collision prevention.** While a unique component header like `text.hpp` might seem distinct on its own, common component names (such as `types`, `config`, `error`, `version`, or `platform`) would collide with third party libraries, system headers, or other targets if included bare as `<types.hpp>`. The inner folder guarantees that includes are always explicitly qualified by the component name, such as `<text/text.hpp>` or `<geometry/types.hpp>`.

**Zero boilerplate CMake includes (`FILE_SET`).** Combined with CMake 3.23+ `FILE_SET TYPE HEADERS`, the base directory of the file set defaults to the component folder (`srcTargets/text`). CMake automatically exposes `<text/text.hpp>` to consuming targets without needing manual include directories or verbose generator expressions (`$<BUILD_INTERFACE>` / `$<INSTALL_INTERFACE>`).

## Isn't one `CMakeLists.txt` per component a lot?

Each is about 10 lines, and the lines are identical apart from the dependencies. They are deliberately *explicit* rather than hidden behind custom build scripting, so you can read exactly what a component is at a glance.

If you want to deduplicate shared options across components, you can do so however you prefer, such as `include()`ing a shared options template or linking a common interface target. The layout does not depend on that choice.

## Does this layout mean I have to make my objects or targets smaller?

No. Target Layout is not about component size. It is about **colocation and one to one identity** (see **[Lineage & Philosophy](index.md#1-lineage--philosophy)**). Whatever boundary you draw around a component, large or small, that folder is your CMake target, public header, namespace, and unit test.

In a three tree layout, splitting a growing component means creating files across split directorys and editing central build list, which implicitly nudges teams creating larger growing catch all umbrella libraries.

By removing that friction, Target Layout makes smaller, focused components effortless, especially in high velocity, AI agent driven development.

**Zero maintenance target registration.** Every subdirectory under `srcTargets/` is discovered automatically, with no editing of parent `CMakeLists.txt` files.

**Frictionless boundary enforcement.** The `target_link_libraries` command **is** your architectural map, and the compiler prevents undeclared header inclusions.

**Confined blast radius.** Diffs, context windows, and code reviews stay bounded to a single folder, and edits only recompile that component and its test binary.

**Zero friction navigation.** All headers, sources, tests, and build instructions for a component live side by side in one directory.

## Is this like Cargo (Rust) workspaces?

Yes. While Target Layout's component model traces back to John Lakos and Bloomberg's BDE, its workspace ergonomics took direct inspiration from modern toolchains like Rust's Cargo (see **[Lineage & Philosophy](index.md#1-lineage--philosophy)**).

Cargo proved that enforcing **rigid build contracts and convention over configuration**, rather than asking developers to manually manage arbitrary directory trees, drastically benefits architectural simplicity, boundary enforcement, and development velocity.

In many ways, **Target Layout is a Cargo style workspace** built on top of modern CMake primitives.

```text
Cargo Workspace                             Target Layout
├── Cargo.toml                              ├── CMakeLists.txt
└── crates/                                 └── srcTargets/
    ├── parser/                                 ├── parser/
    │   ├── Cargo.toml                          │   ├── CMakeLists.txt
    │   ├── src/lib.rs                          │   ├── parser.cpp
    │   │                                       │   ├── parser/parser.hpp
    │   └── tests/                              │   └── parserTEST/
    │       └── parser_test.rs                  │       ├── CMakeLists.txt
    │                                           │       └── parserTEST.cpp
    └── renderer/                               └── renderer/
        ├── Cargo.toml                              ├── CMakeLists.txt
        └── ...                                     └── ...
```

The core principles map one to one.

**Convention over configuration.** In Cargo, directory and file placement determine target names and roles. In Target Layout, `${DIR_NAME}` automatically derives the target name, namespaced alias (`${PROJECT_NAME}::${DIR_NAME}`), header include prefix (`<name>/<name>.hpp`), and dedicated test executable.

**Zero maintenance auto discovery.** Adding a new crate in Cargo or a new component in Target Layout requires only dropping a new folder into `crates/` or `srcTargets/`. CMake automatically discovers and registers the target on the next configure run, meaning you never touch central root build scripts.

**Strict compilation boundaries.** In Cargo, crate `renderer` cannot use code from `parser` without explicitly declaring it in its dependencies. In Target Layout, component `report` cannot `#include <geometry/geometry.hpp>` without declaring `target_link_libraries(report PUBLIC Target_Layout::geometry)`. Undeclared dependencies and circular references are rejected at compile time.

**Colocated verification.** In Cargo, unit tests live inside the crate directory. In Target Layout, the test executable target (`<name>TEST/`) lives side by side with the contract (`.hpp`) and implementation (`.cpp`) it verifies, ensuring interface changes and test updates happen in the exact same directory.

## Why does this layout matter for AI coding agents?

AI coding agents are trained and benchmarked primarily on **tactical programming**, focusing on making the immediate issue or test pass as quickly as possible.

Like most human developers, agents take the path of least resistance. In a conventional `include/` + `src/` + `tests/` repository, adding quick code changes into an existing target, and `#include`ing whatever header is convenient from a shared `include/` tree, is far easier than creating a new library across three directory trees and central `CMakeLists.txt` files.

Target Layout aligns the physical layout with **strategic programming**.

**Tactical shortcuts fail at compile time.** Because there is no global `include/` directory, an agent cannot reach across component boundaries unless the dependency is explicitly declared in `target_link_libraries`. Layering violations and circular dependencies are rejected immediately by the compiler and CMake.

**Strategic refactoring is the path of least resistance.** When you instruct an agent to refactor, split a growing component, or extract a new component, `${DIR_NAME}` delegated naming and directory auto registration mean it can create or rename a self contained folder under `srcTargets/` without touching central build files.

**Clear targets for component deepening and test coverage.** When an agent is tasked with architectural improvements, such as *component deepening* (narrowing `<name>/<name>.hpp` and demoting `PUBLIC` dependencies to `PRIVATE`) or expanding unit tests in `<name>TEST/`, 100% of the component's contract, implementation, build rules, and tests live together in one directory.

**Token saving graph database at scale (`.dot`).** Because every component is a CMake target, CMake's straightforward Graphviz `.dot` generation exports the entire component dependency graph and visibility rules (`PUBLIC` vs. `PRIVATE`) as a single compact text database. At scale, an AI agent can read this `.dot` file in one step to plan strategic changes or refactorings, saving significant context window tokens compared to crawling individual directories, while humans get a rendered visual mental map for code review.

## Why `file(GLOB)`? Isn't globbing discouraged?

Globbing *source files* in a target is discouraged because the build cannot see new files. Here the glob only discovers **component directories**, with `CONFIGURE_DEPENDS` so adding one reruns configure. The files inside each component are listed explicitly in `target_sources`.

## What does it actually enforce?

Compile time boundary enforcement applies across several areas.

**No undeclared access.** A component cannot `#include` headers from another component unless it is explicitly declared in `target_link_libraries`.

**Transitive exposure control.** `PRIVATE` implementation dependencies remain internal to the `.cpp` and are never exposed to consumers, `PUBLIC` interface dependencies propagate automatically to consumers, and `INTERFACE` dependencies propagate requirements from header only components without compiling a binary.

**Structural consistency.** The build system does not police C++ syntax or filenames. Consistency is maintained by the self contained component convention and shared templates.

## Does it scale to a large project?

Target Layout itself adds no special build overhead. Whether a project has 5 large components or 200 smaller ones, each directory is simply a standard CMake target.

However, because Target Layout makes **creating components effortless**, teams naturally take advantage of smaller components for better modularity, reviewability, and development velocity. When a project is designed around many smaller component targets, the build system should be configured with standard C++ scaling practices in mind.

**Incremental builds (the 99% developer loop).** Scales exceptionally well. Modifying a component only recompiles that single translation unit and only relinks its dedicated test binary, avoiding unnecessary project wide recompilation.

**Clean build overhead.** Compiling many individual target archives has slightly more process invocation and header parsing overhead on a cold clean build than bundling everything into a single library. Techniques like precompiled headers (PCH) or C++20 Modules mitigate this if clean builds become a bottleneck.

**Binary size & template deduplication.** Separate static archive boundaries can duplicate template instantiations across targets. Enabling link time optimization (LTO / IPO) in release builds allows the linker to perform global inlining and discard duplicate symbols.

**Linker performance.** Linking many static libraries into final binaries shifts work to the linker. For very large codebases, modern linkers (such as `lld` or `mold`) keep link times fast.

**Build parallelism and `OPTIMIZE_DEPENDENCIES`.** A common myth is that fine grained static targets "serialize" a build. In reality, build generators like Ninja and Make compile all `.cpp` translation units concurrently across all available cores; source compilation never waits for dependent `.a` archives. The only thing CMake sequences by default is an order only constraint between the 1ms `ar` archiving steps. In CMake 3.19+, you can prune even this archiving constraint by adding `set(CMAKE_OPTIMIZE_DEPENDENCIES ON)` to your root `CMakeLists.txt`. It is an easy, welcome micro optimization that integrates cleanly with Target Layout, but because archiver sequencing takes mere milliseconds compared to C++ compilation, it is a nice to have cleanup rather than a game changing speed improvement.

**Architectural flexibility.** Because every component is an isolated CMake target, options like toggling unity builds or applying custom compiler profiles can be **managed centrally** without restructuring files or directories.

## Why `STATIC` libraries instead of `OBJECT` libraries?

A common question when creating targets is whether internal components should be declared as CMake `OBJECT` libraries (`add_library(${DIR_NAME} OBJECT)`) to avoid archiver (`ar`) invocations, or whether `BUILD_SHARED_LIBS=ON` could accidentally turn them into shared libraries.

Target Layout uses explicit **`STATIC`** libraries (`add_library(${DIR_NAME} STATIC)`) for three concrete reasons.

### 1. `OBJECT` libraries break transitive `PRIVATE` dependencies in CMake

In a modular component architecture, implementation dependencies are declared `PRIVATE`. For example, `report` uses `text` internally, but its public header only exposes `geometry`.

Consider the case where `text` and `report` are `OBJECT` libraries.

```cmake
add_library(text OBJECT)
add_library(report OBJECT)
target_link_libraries(report PRIVATE text)

add_executable(app app.cpp)
target_link_libraries(app PRIVATE report)
```

When CMake links `app`, it adds `report.o` to the link command, but **it drops `text.o`** because `text` was a `PRIVATE` dependency of `report`. The build fails at link time with undefined references.

```text
undefined reference to `text::to_upper(std::string)'
```

CMake's official documentation states that an object library may be linked to another object library, but the objects of the second library are not automatically added to targets linking to the first.

To work around this limitation with `OBJECT` libraries, you would have to make all dependencies `PUBLIC` (destroying encapsulation and leaking headers across the entire project) or manually link transitive dependencies into every leaf executable. 

**`STATIC` libraries propagate link closures seamlessly.** CMake automatically passes `libtext.a` to the final link line of `app` without exposing `text`'s header search paths.

### 2. Archiver overhead (`ar`) is negligible compared to compilation

Packaging an object file into a static archive (`ar rcs lib<name>.a <name>.o`) is an uncompressed archive operation that takes 1 to 2 milliseconds on modern storage and operating system disk caches.

In contrast, **95 to 99% of C++ build time** is spent in compiler front ends (`clang++`/`g++`) parsing headers, expanding templates, building ASTs, and generating machine code. In the day to day developer loop (incremental builds), fine grained static targets ensure you only recompile the single modified translation unit and rearchive it in a couple of milliseconds, rather than recompiling monolithic projects.

### 3. Immunity to `BUILD_SHARED_LIBS`

Because components explicitly specify `STATIC`, setting `-DBUILD_SHARED_LIBS=ON` in CMake has zero effect on them.

```cmake
add_library(${DIR_NAME} STATIC)
```

CMake only converts untyped `add_library(<name>)` calls into shared libraries, preventing unintended shared library and dynamic linker proliferation.

## Why `FILE_SET TYPE HEADERS` instead of `target_include_directories`?

In standard modern CMake tutorials and community forums, `target_include_directories` is common advice because it was the flagship mechanism of CMake 3.0 to 3.22.

Target Layout requires CMake 3.23+ and replaces `target_include_directories` with **`target_sources(... FILE_SET TYPE HEADERS ...)`** due to several long standing limitations.

**It passes directories instead of files.** The `target_include_directories` command only passes directory strings (`-I` flags) to the compiler. CMake has no knowledge of which individual header files belong to the target.

**IDE blindness & duplicate bookkeeping.** Because CMake only tracks directories, IDEs (Visual Studio, Xcode, CLion) often do not show public headers under the target's project view unless you manually maintain duplicate file lists in `add_library()` or `target_sources()`.

**Boilerplate install & export rules.** Making a library both buildable in tree and exportable for downstream consumers requires verbose generator expressions (`$<BUILD_INTERFACE>` vs. `$<INSTALL_INTERFACE>`) and separate `install(DIRECTORY ...)` or `install(FILES ...)` commands.

Declaring headers with `FILE_SET TYPE HEADERS` provides three major benefits.

**Automatic include resolution.** CMake automatically treats the base directory of the file set (the component directory) as the target's include path, making `target_include_directories` redundant.

**First class header ownership.** CMake formally tracks `<name>/<name>.hpp` as part of the target, ensuring IDEs display public headers directly alongside the component.

**Unified installation.** Exporting both the compiled binary and its public headers collapses into a single standard call (`install(TARGETS ${DIR_NAME} FILE_SET ${DIR_NAME})`) with zero generator expressions.

## Does it handle install/export?

Yes, and it is significantly cleaner than traditional CMake configurations.

In conventional layouts, installation requires maintaining separate include directories and awkward generator expressions (`$<BUILD_INTERFACE>` vs `$<INSTALL_INTERFACE>`). 

Because Target Layout uses CMake 3.23+ `FILE_SET ... TYPE HEADERS`, CMake natively knows the public include hierarchy. Installing and exporting both the binaries and their headers collapses into a single standard call.

```cmake
install(TARGETS ${DIR_NAME}
        EXPORT ${PROJECT_NAME}Targets
        FILE_SET ${DIR_NAME})
```

CMake automatically handles the header destination directory without needing generator expression workarounds.
