# Target Layout

*'The component is the CMake target.'*

*'One directory = one component = one CMake target. The directory, target, translation unit, public header, namespace and test all share one name.'*

```text
srcTargets/
├── CMakeLists.txt        <-- auto-registers every subdirectory as a component
├── text/
│   ├── CMakeLists.txt
│   ├── text.cpp
│   ├── text/text.hpp     <-- #include <text/text.hpp>
│   └── textTEST/
├── geometry/
├── report/               <-- depends on text (PRIVATE) and geometry (PUBLIC)
└── app/
```

This repository exists only to demonstrate and document that layout: four tiny components, no external dependencies, about 100 lines of CMake.

```bash
cmake --preset debug && cmake --build --preset debug && ctest --preset debug
```

Requires CMake 3.23+ and a C++ compiler (C++17 baseline, C++20 for the `text` component).

**Documentation:** https://jeffkarling.github.io/Target_Layout/

Licensed under [MIT-0](LICENSE).
