[Release Tag Page](https://github.com/JeffKarling/Target_Layout/tags) | [GitHub Repository](https://github.com/JeffKarling/Target_Layout)

# Target Layout

*'The component is the CMake target.'*

Target Layout is a small, opinionated way to lay out a C++ project with CMake. It is built on a single rule:

*'One directory = one component = one CMake target. The directory, the target, the translation unit, the public header, the namespace and the test all share one name.'*

That is the whole idea. This repository exists only to demonstrate and document it: four tiny components, no external dependencies, about 100 lines of CMake.

---

## The layout

```text
srcTargets/
├── CMakeLists.txt        <-- auto-registers every subdirectory as a component
├── text/                 <-- a component, which is also the CMake target `text`
│   ├── CMakeLists.txt
│   ├── text.cpp          <-- translation unit
│   ├── text/text.hpp     <-- public header:  #include <text/text.hpp>
│   └── textTEST/         <-- its test, living with it
│       ├── CMakeLists.txt
│       └── textTEST.cpp
├── geometry/             <-- same shape
├── report/               <-- same shape, depends on text + geometry
└── app/                  <-- an executable component
```

Everything about `text` is found by knowing one word: `text`.

## A component in full

This is the entire `srcTargets/text/CMakeLists.txt` (comments trimmed):

```cmake
cmake_path(GET CMAKE_CURRENT_LIST_DIR FILENAME DIR_NAME)   # directory name IS the component name

add_library(${DIR_NAME} STATIC)                            # the component IS a CMake target
add_library(${PROJECT_NAME}::${DIR_NAME} ALIAS ${DIR_NAME})

target_sources(${DIR_NAME}
        PRIVATE ${DIR_NAME}.cpp
        PUBLIC  FILE_SET ${DIR_NAME} TYPE HEADERS
                FILES ${DIR_NAME}/${DIR_NAME}.hpp)

target_compile_features(${DIR_NAME} PRIVATE cxx_std_20)    # C++20 implementation; headers stay C++17

if(BUILD_TESTING)
    add_subdirectory(${DIR_NAME}TEST)
endif()
```

Nothing in it mentions `text` literally. Copy the folder, rename it, and it is a new component.

## Try it

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./Build/debug/srcTargets/app/app      # POSTER: 3 x 4 (area 12)
```

Requires CMake 3.23+ and a C++ compiler (C++17 baseline, C++20 for the `text` component). No other dependencies.

## Read next

- **[How it works](how_it_works.md)**: the rule, the anatomy of a component, link visibility, adding a component, and what the layout does *not* enforce.
- **[Why Target Layout](why_target_layout.md)**: compiler-enforced boundaries, `${DIR_NAME}` delegated naming, per-component C++ standards, and how the layout scales.
- **[FAQ](faq.md)**: "where is `include/` and `src/`?", "isn't one CMakeLists per component a lot?", and the other questions this layout always gets.
