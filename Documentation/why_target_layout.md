# Why Target Layout

Traditional C++ layouts like the Pitchfork Layout (PFL) organize files by **artifact type** (`include/`, `src/`, `tests/`). Even when a PFL project is broken into multiple libraries—either by stitching targets across those three trees or by nesting full `libs/<name>/include/`, `libs/<name>/src/`, and `libs/<name>/tests/` hierarchies for every component—the physical component remains fractured across directories.

Target Layout organizes files by **physical component**, and makes each component a **discrete CMake target** in a single folder. Combining co-location with a strict 1-to-1 naming symmetry (`Directory == Target == Translation Unit == Header == Namespace == Test`) unlocks architectural and workflow advantages without multi-tree overhead.

---

## 1. Compiler-Enforced Architecture & Cycle-Free Design

When multiple components share a top-level `include/` tree, any `.cpp` file can accidentally `#include` headers from unrelated components. Even in multi-library setups, creating a new isolated library in a 3-tree layout has enough directory and build-file friction that teams often lump multiple logical components into coarser targets.

Because Target Layout makes defining a target as cheap as creating a folder, every component gets its own compilation boundary:

- **No undeclared dependencies:** A component receives no include paths to another component unless it explicitly links it. Including an unlinked header is an immediate compiler error rather than a code-review comment.
- **Guaranteed acyclic dependency graph (DAG):** In John Lakos's *Large-Scale C++ Software Design*, keeping physical components levelized (free of circular dependencies) requires constant manual vigilance. When every component is a static target, CMake rejects circular dependencies automatically at configure and link time.
- **Strict transitive visibility:** Implementation-only dependencies stay strictly internal to the component, while interface dependencies propagate only when exposed in the public header.

---

## 2. Co-Location & Context Locality

In a type-segregated tree (`include/`, `src/`, `tests/`), reading or refactoring a single component means jumping across three separate directory hierarchies and stitching context together mentally.

| Metric | `include/` + `src/` + `tests/` | Target Layout |
| :--- | :--- | :--- |
| **Files to inspect a component** | Scattered across 3 directory trees | Co-located in `srcTargets/<name>/` |
| **Component overhead** | `libs/<name>/{include,src,tests}` per library | One flat folder per component |
| **Path resolution** | Search the repository | Computable from `<name>` alone |
| **Blast radius of a PR** | Diffs span 3+ directory trees | Confined to a single directory |
| **Test proximity** | Distant `tests/` mirror tree | Side-by-side in `<name>TEST/` |

This co-location benefits both **human engineers** and **AI coding assistants**:

- **Zero-search navigation:** Knowing the component name `report` immediately tells you where its folder, public header, implementation, and unit test live.
- **Self-contained context:** Opening `srcTargets/report/` in an IDE, code review, or AI agent context window provides 100% of the public contract, private implementation, dependencies, and unit tests in one place.
- **Test synchronization:** When a component's unit test lives right next to its header and implementation, interface changes and test updates happen together in the same directory.

---

## 3. Delegated Naming (`${DIR_NAME}`) & Conflict-Free Team Scaling

In conventional CMake projects, adding, renaming, or removing a component requires editing central build files across `src/` and `tests/`—a frequent source of trivial Git merge conflicts when multiple branches add components in parallel.

Target Layout eliminates central registry maintenance by **delegating the component's identity to its folder name (`${DIR_NAME}`)** and discovering component directories automatically:

- **The folder name is the single source of truth:** A component's build file never hardcodes its own name. It reads `${DIR_NAME}` from the folder it lives in and derives the target name, link alias, source filenames, and test target from that single variable.
- **Copy-paste scaffolding:** To create a new component, copy any existing component folder and rename the folder and its files. No parent build files are edited.
- **Zero-conflict parallel branches:** Two developers adding different components on separate Git branches touch zero shared files.
- **Single-action renames:** Naming components early in a project is hard. Because the build file is driven entirely by `${DIR_NAME}`, renaming a component's folder and files updates its target identity automatically without rewriting build scripts.

---

## 4. Incremental C++ Version Upgrades (Not All-or-Nothing)

In many C++ projects, upgrading to a newer C++ standard feels risky because it is treated as an all-or-nothing switch across the entire repository. In reality, major C++ standard library implementations (`libstdc++`, `libc++`, and MSVC STL) go to great lengths to maintain ABI compatibility across modern language versions (C++17, C++20, C++23, C++26) within the same toolchain, allowing translation units compiled with different `-std=` levels to link together safely.

Because every component in Target Layout is an independent CMake target with explicit `PRIVATE` and `PUBLIC` visibility, adopting new C++ features is low-risk and incremental:

- **Private standard upgrades (`PRIVATE`):** A component can adopt a newer C++ standard inside its `.cpp` implementation while keeping its public header compatible with an older baseline—requiring zero changes to consumers. This repository exemplifies this directly: `srcTargets/text/text.cpp` adopts C++20 `std::ranges::transform` via `target_compile_features(${DIR_NAME} PRIVATE cxx_std_20)` while its public header and consumers (`report`, `app`) build cleanly under the C++17 project baseline.
- **Scoped public standard upgrades (`PUBLIC`):** If a component's public header adopts newer C++ features, that standard requirement propagates only to the downstream components that actually link it, leaving unrelated subsystems untouched.
- **Targeted compiler flags and hardware intrinsics:** If a single performance-critical component needs SIMD flags, fast-math, or specialized warning profiles while legacy components stay on baseline flags, those settings apply cleanly to that component boundary alone.

---

## 5. Strategic by Default: Built for Modern & AI-Driven Velocity

At its core, Target Layout is not about component size—a component can be as large or as small as your design calls for. What Target Layout changes is **the friction balance between tactical and strategic programming**.

In *A Philosophy of Software Design*, John Ousterhout contrasts **tactical programming** (getting the immediate task or bugfix working as fast as possible) with **strategic programming** (investing in clean boundaries, refactoring, and long-term design so the codebase does not decay into a "ball of mud"). AI coding agents—trained and benchmarked primarily on tactical issue resolution—naturally gravitate toward tactical shortcuts unless guided and constrained by the project's structure.

In traditional 3-tree layouts (`include/`, `src/`, `tests/`), **tactical shortcuts are easy and strategic refactoring is hard**: any `.cpp` file can grab headers from a shared `include/` tree, whereas splitting or extracting a component carries the administrative tax of creating files across three directory trees and editing central build lists. Because of that friction, both humans and AI agents let components grow coarser and more coupled than they should.

Target Layout **inverts that incentive structure**, making strategic architecture the path of least resistance:

- **Compiler guardrails against tactical shortcuts:** An agent or developer cannot silently `#include` an unlinked header just to get a test passing (`fatal error: No such file or directory`). Coupling two components requires an explicit change to `target_link_libraries` in `CMakeLists.txt`, turning hidden architectural drift into an obvious, single-line diff—and circular dependencies into an immediate CMake error.
- **Zero-friction strategic refactoring:** When a component grows too large, extracting a new component is as simple as copying a folder and renaming its same-named files. Auto-registration and `${DIR_NAME}` mean splitting a component is just as easy as piling more code into an existing one.
- **First-class support for "component deepening" (`PUBLIC` vs. `PRIVATE`):** Great architecture favors *deep components*—narrow, simple public interfaces hiding rich internal implementations. In Target Layout, a component's public surface is strictly isolated to `<name>/<name>.hpp` (`PUBLIC FILE_SET`) and its `PUBLIC` link dependencies, while `<name>.cpp`, private helper files, and `PRIVATE` dependencies stay completely encapsulated. When directing an AI agent to deepen components or reduce coupling, the boundary between public contract and private implementation is explicit in a single directory.
- **Strategic oversight without reading every line of code:** As AI feature velocity increases, human review becomes the bottleneck. Because the directory structure and the ~10-line `CMakeLists.txt` *are* the architecture, you can audit the strategic impact of a pull request at a glance—by checking `<name>/<name>.hpp` and `target_link_libraries`—without having to police every line of `.cpp` implementation for hidden coupling.
- **Built-in graph database for AI context and human mental maps:** Because every physical component is a 1-to-1 CMake target, generating a Graphviz `.dot` dependency database in CMake is straightforward and produces an exact map of the component architecture. As a project scales to many smaller components, this plain-text `.dot` database serves as a major token- and context-saving shortcut for AI agents—letting an agent read the entire dependency DAG and `PUBLIC`/`PRIVATE` boundaries in a single step rather than scanning dozens of directories—while rendering into a visual mental map for human architectural review and refactoring.
- **Minimal rebuild and verification blast radius:** Pointing an agent at `srcTargets/<name>/` gives it 100% of the relevant context (contract, implementation, dependencies, and `<name>TEST/`) without repository-wide noise. Modifying an implementation file recompiles only that component and re-links only its dedicated test executable and downstream consumers, letting humans and agents verify changes, expand test coverage, or run static analysis (`clang-tidy`, sanitizers) in seconds.

---

## 6. Code-Generation Components Are a Natural Fit

In layouts that divide the repository into `include/`, `src/`, and `tests/`, code generation (such as Protobuf, FlatBuffers, reflection/DSL scripts, or generated test suites) is always awkward: a `.proto` schema or a test-generation dataset is neither a C++ header nor a `.cpp` source, so teams invent separate `schemas/` trees and custom output-directory plumbing. Worse, if generated headers land in a shared build include path, downstream targets can accidentally `#include` them without declaring a build-order dependency—causing flaky parallel build races.

Target Layout's rules match modern CMake code-generation best practices 1-to-1:

- **A natural home for inputs (*"commit inputs, not outputs"*):** Because a directory in `srcTargets/` represents a *component* rather than a file extension, the schema, DSL input, or template lives directly inside `srcTargets/<name>/` in version control, while generated `.hpp` and `.cpp` outputs stay strictly in the build tree.
- **Knowable, deterministic outputs via `${DIR_NAME}`:** Build-time code generators require the build system to know the exact output filenames and paths before the generator runs. Target Layout's 1-to-1 naming rule (`${DIR_NAME}`) makes the generated header and translation unit paths completely predictable from the folder name alone.
- **Seamless `FILE_SET` consumption & race-free builds:** Because every component in Target Layout already publishes its interface through a target-scoped `FILE_SET`, attaching generated headers from the component's build directory to its `FILE_SET` makes a codegen component indistinguishable from a hand-written one. Consumers link `${PROJECT_NAME}::<name>` and `#include <<name>/<name>.hpp>` as usual, and CMake guarantees the generator runs before any consumer compiles—with full incremental rebuilds, IDE indexing, and no special "prepare" targets to remember.
- **Self-contained test code generation (`<name>TEST/`):** Unit testing frequently relies on code generation—such as generating parameterized test cases from JSON/YAML/DSL inputs, synthesizing mock implementations, or building fuzz/property test harnesses. Because `<name>TEST/` is already its own directory and CMake target living right inside the component, test-generation inputs and build rules stay co-located inside `<name>TEST/` (guarded by `BUILD_TESTING`), never polluting the production component target or non-test builds.
- **Isolated compiler flags for generated code:** Generated C++ sources often require relaxed compiler warnings or specific library dependencies (like `libprotobuf`). Giving the generated code its own component or test target keeps those warning suppressions and dependencies strictly isolated from hand-written code.

---

## 7. Clean Scaling to Production Codebases

While this repository demonstrates the layout in its minimal form, the same 1-to-1 structure scales naturally to large production projects:

- **Centralized target configuration via `${DIR_NAME}`:** Because every component names its target `${DIR_NAME}`, a project can share warning profiles, sanitizer setups, static analysis hooks, and Link-Time Optimization settings through a single shared configuration file included by each component—without inventing custom CMake wrapper macros.
- **Private helper files:** A component is not limited to a single source file. Internal helper `.cpp` and `.hpp` files can live alongside the main translation unit inside the component folder without being added to the public header set, keeping internal details encapsulated.
- **Frictionless packaging and C++20 Modules readiness:** Because public headers are formally attached to each component target, installing and exporting packages works without path-rewriting workarounds, and components can be migrated from traditional headers to C++20 Modules one component at a time without reorganizing the filesystem.

> **Designing your build setup for smaller components:** Because Target Layout makes creating components frictionless, projects naturally embrace smaller, focused components. As a project grows to many small library targets, cold clean builds and linking do slightly more work in exchange for much faster incremental builds. Any project built around smaller components shares these characteristics, and a modern CMake setup manages them cleanly with standard settings (LTO/IPO, modern linkers like `lld`/`mold`, Precompiled Headers, or C++20 Modules)—see **[FAQ: Does it scale to a large project?](faq.md#does-it-scale-to-a-large-project)**.
