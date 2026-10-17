cmake_path(GET CMAKE_CURRENT_LIST_DIR FILENAME DIR_NAME)

# An executable is a component too: same rule, directory name == target name.
# It has no public header because nothing links against it.
add_executable(${DIR_NAME} ${DIR_NAME}.cpp)

target_link_libraries(${DIR_NAME} PRIVATE ${PROJECT_NAME}::report)
