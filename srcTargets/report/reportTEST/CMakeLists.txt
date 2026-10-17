cmake_path(GET CMAKE_CURRENT_LIST_DIR FILENAME DIR_NAME)    # Same rule: directory name == target name (e.g. textTEST).
string(REGEX REPLACE "TEST$" "" COMPONENT_NAME ${DIR_NAME}) # The component under test (e.g. text).

add_executable(${DIR_NAME} ${DIR_NAME}.cpp)

target_link_libraries(${DIR_NAME} PRIVATE ${PROJECT_NAME}::${COMPONENT_NAME})

add_test(NAME ${DIR_NAME} COMMAND ${DIR_NAME})
