cmake_path(GET CMAKE_CURRENT_LIST_DIR FILENAME DIR_NAME) # 1. The directory name IS the component name.

add_library(${DIR_NAME} STATIC)                           # 2. The component IS a CMake target...
add_library(${PROJECT_NAME}::${DIR_NAME} ALIAS ${DIR_NAME}) #    ...consumed through a namespaced alias.

target_sources(${DIR_NAME}
        PRIVATE
        ${DIR_NAME}.cpp                                   # 3. Same-named translation unit (private).

        PUBLIC
        FILE_SET ${DIR_NAME} TYPE HEADERS                 # 4. Same-named public header, exposed as
        FILES ${DIR_NAME}/${DIR_NAME}.hpp                 #    #include <name/name.hpp>.
)

# Demonstrating per-component C++ standard:
# text.cpp uses C++20 std::ranges::transform, but its public header only requires C++17.
# Setting cxx_std_20 as PRIVATE keeps the requirement internal; consumers remain on baseline C++17.
target_compile_features(${DIR_NAME} PRIVATE cxx_std_20)

# This component has no dependencies, so it links nothing.
# Without a link, it cannot include anything from another component.

if(BUILD_TESTING)
    add_subdirectory(${DIR_NAME}TEST)                     # 5. The test lives with the component.
endif()
