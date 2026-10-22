cmake_path(GET CMAKE_CURRENT_LIST_DIR FILENAME DIR_NAME)

add_library(${DIR_NAME} STATIC)
add_library(${PROJECT_NAME}::${DIR_NAME} ALIAS ${DIR_NAME})

target_sources(${DIR_NAME}
        PRIVATE
        ${DIR_NAME}.cpp

        PUBLIC
        FILE_SET ${DIR_NAME} TYPE HEADERS
        FILES ${DIR_NAME}/${DIR_NAME}.hpp
)

# The dependency graph IS the architecture, and the link visibility is the encapsulation:
target_link_libraries(${DIR_NAME}
        PUBLIC  ${PROJECT_NAME}::geometry # report.hpp exposes geometry::Rect, so consumers need it too.
        PRIVATE ${PROJECT_NAME}::text     # only report.cpp uses text: consumers of report cannot include it.
)

if(BUILD_TESTING)
    add_subdirectory(${DIR_NAME}TEST)
endif()
