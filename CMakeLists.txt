cmake_minimum_required(VERSION 3.23) # FILE_SET (3.23) is the minimum the layout needs

project(Target_Layout
        VERSION 1.0.0
        DESCRIPTION "The component is the CMake target"
        HOMEPAGE_URL "https://github.com/JeffKarling/Target_Layout"
        LANGUAGES CXX
)

# Project-wide defaults. Applied globally so component CMakeLists stay about the layout only.
set(CMAKE_CXX_STANDARD 17) # Baseline standard across the project
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(
        "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic>"
        "$<$<CXX_COMPILER_ID:MSVC>:/W4>"
)

include(CTest) # defines BUILD_TESTING and calls enable_testing()

# Every directory under srcTargets/ is one component, which is one CMake target.
add_subdirectory(srcTargets)
