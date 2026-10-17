# FAQ

## Where are `include/` and `src/`?

There are none, on purpose. Those directories group files by *kind*. Target Layout groups them by *component*: everything about `text` (header, source, test, CMake) is one folder.

| Task | Conventional `include/` + `src/` + `tests/` | Target Layout |
| :--- | :--- | :--- |
| Add a component | create a header, a source, a test in 3 trees; edit CMake lists | copy a folder, rename |
| Rename a component | touch 3 trees plus CMake | rename the folder and its same-named files |
| Delete a component | hunt down files in 3 trees plus CMake | delete the folder (and links to it) |
| "Where does `text` live?" | search | `srcTargets/text/` |

The cost: the public header is one level deeper (`srcTargets/text/text/text.hpp`). That inner `text/` is what gives consumers the `<text/text.hpp>` include path.

## Isn't one `CMakeLists.txt` per component a lot?

Each is about 10 lines, and the lines are identical apart from the dependencies. They are deliberately *explicit* rather than hidden behind custom build scripting, so you can read exactly what a component is at a glance.

If you want to deduplicate shared options across components, you can do so however you prefer—such as `include()`ing a shared options template or linking a common interface target. The layout does not depend on that choice.

## Does this layout mean I have to make my objects or targets smaller?

No. At its core, Target Layout is not about component size—it is about **co-location and 1-to-1 identity**:

In Pitchfork (PFL), a logical component (like `geometry`) is typically fractured across three distant folder hierarchies: `include/myproject/geometry/`, `src/geometry/`, and `tests/geometry/`. A CMake target in PFL is an abstraction stitched together across those separate trees.

Target Layout brings all of those files into one place. Whatever boundary you draw around a component—whether large or small—that folder becomes your CMake target: its directory, target name, public header, namespace, and test all live together as a single, self-contained unit.

What Target Layout changes is that it **makes component size a free choice**. In a 3-tree layout, splitting a growing component into smaller components means creating files across three directory trees and editing central build lists, which nudges teams toward larger, coarser components. By removing that friction, Target Layout fits the modern trend toward **smaller, focused components**—especially in high-velocity, AI-agent-driven development where smaller components mean:

- **Zero-maintenance target registration:** Because every subdirectory under `srcTargets/` is a target, CMake discovers and registers new components automatically—no editing parent `CMakeLists.txt` files across multiple trees.
- **Frictionless boundary enforcement:** Because defining a dedicated target requires no multi-tree setup, you can keep compilation boundaries strict: `target_link_libraries` **is** your architectural map, and the compiler prevents undeclared header inclusions.
- **Confined blast radius for humans and AI agents:** Smaller components keep diffs, context windows, and human code reviews bounded to a single folder, while modifying an implementation file only recompiles that component and its dedicated test binary.
- **Zero-friction navigation:** All headers, sources, tests, and build instructions for a component live side-by-side in one directory.

## Why does this layout matter for AI coding agents?

AI coding agents are trained and benchmarked primarily on **tactical programming**—making the immediate issue or test pass as quickly as possible. Left unchecked in a conventional `include/` + `src/` + `tests/` repository, tactical behavior gradually degrades architecture: agents take the path of least resistance by `#include`-ing whatever header is convenient from the global `include/` tree and piling new code into existing targets rather than editing three directory trees and central `CMakeLists.txt` files.

Target Layout aligns the physical layout with **strategic programming**:

- **Tactical shortcuts fail at compile time:** Because there is no global `include/` directory, an agent cannot reach across component boundaries unless the dependency is explicitly declared in `target_link_libraries`. Layering violations and circular dependencies are rejected immediately by the compiler and CMake.
- **Strategic refactoring is the path of least resistance:** When you instruct an agent to refactor, split a growing component, or extract a new component, `${DIR_NAME}` delegated naming and directory auto-registration mean it can create or rename a self-contained folder under `srcTargets/` without touching central build files.
- **Clear targets for component deepening and test coverage:** When an agent is tasked with architectural improvements—such as *component deepening* (narrowing `<name>/<name>.hpp` and demoting `PUBLIC` dependencies to `PRIVATE`) or expanding unit tests in `<name>TEST/`—100% of the component's contract, implementation, build rules, and tests live together in one directory.
- **Token-saving graph database at scale (`.dot`):** Because every component is a CMake target, CMake's straightforward Graphviz `.dot` generation exports the entire component dependency graph and visibility rules (`PUBLIC` vs. `PRIVATE`) as a single compact text database. At scale, an AI agent can read this `.dot` file in one step to plan strategic changes or refactorings—saving significant context window tokens compared to crawling individual directories—while humans get a rendered visual mental map for code review.

## Why `file(GLOB)`? Isn't globbing discouraged?

Globbing *source files* in a target is discouraged because the build cannot see new files. Here the glob only discovers **component directories**, with `CONFIGURE_DEPENDS` so adding one re-runs configure. The files inside each component are listed explicitly in `target_sources`.

## What does it actually enforce?

Compile-time boundary enforcement:

- **No undeclared access:** A component cannot `#include` headers from another component unless it is explicitly declared in `target_link_libraries`.
- **Transitive exposure control:**
  - **`PRIVATE`**: Implementation dependencies remain internal to the `.cpp` and are never exposed to consumers.
  - **`PUBLIC`**: Interface dependencies required by public headers are propagated to consumers automatically.
  - **`INTERFACE`**: Header-only components propagate their dependencies and include paths to consumers without generating compiled binaries.
- **Structural consistency:** The build system does not police C++ syntax or filenames—consistency is maintained by the self-contained component convention and shared templates.

## Does it scale to a large project?

Target Layout itself adds no special build overhead: whether a project has 5 large components or 200 smaller ones, each directory is simply a standard CMake target.

However, because Target Layout makes creating components effortless, teams naturally take advantage of **smaller components** for better modularity, reviewability, and development velocity. When a project is designed around many smaller component targets, the build system should be configured with standard C++ scaling practices in mind:

- **Incremental builds (the 99% developer loop):** Scales exceptionally well. Modifying a component only recompiles that single translation unit and only re-links its dedicated test binary, avoiding unnecessary project-wide recompilation.
- **Clean build overhead:** Compiling many individual target archives has slightly more process invocation and header parsing overhead on a cold clean build than bundling everything into a single library. Techniques like Precompiled Headers (PCH) or C++20 Modules mitigate this if clean builds become a bottleneck.
- **Binary size & template deduplication:** Separate static archive boundaries can duplicate template instantiations across targets. Enabling Link-Time Optimization (LTO / IPO) in release builds allows the linker to perform global inlining and discard duplicate symbols.
- **Linker performance:** Linking many static libraries into final binaries shifts work to the linker. For very large codebases, modern linkers (such as `lld` or `mold`) keep link times fast.
- **Architectural flexibility:** Because every component is an isolated CMake target, changing library types (e.g. switching to `OBJECT` libraries) or toggling unity builds can be done centrally without restructuring files or directories.

## Does it handle install/export?

Yes, and it is significantly cleaner than traditional CMake configurations.

In conventional layouts, installation requires maintaining separate include directories and awkward generator expressions (`$<BUILD_INTERFACE>` vs `$<INSTALL_INTERFACE>`). 

Because Target Layout uses CMake 3.23+ `FILE_SET ... TYPE HEADERS`, CMake natively knows the public include hierarchy. Installing and exporting both the binaries and their headers collapses into a single standard call:

```cmake
install(TARGETS ${DIR_NAME}
        EXPORT ${PROJECT_NAME}Targets
        FILE_SET ${DIR_NAME})
```

CMake automatically handles the header destination directory without needing generator expression workarounds.
