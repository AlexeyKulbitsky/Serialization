cmake_minimum_required(VERSION 3.4.1)

project (SerializationExample)

set(SERIALIZATION_PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/source)
set(EXAMPLE_PROJECT_NAME Example)

set(EXAMPLE_APP_SRC
  source/main.cpp
)

set(EXAMPLE_APP_SOURCE_FILES
  ${EXAMPLE_APP_SRC}
)

add_executable(${EXAMPLE_PROJECT_NAME} ${EXAMPLE_APP_SOURCE_FILES})

# add serialization library
add_subdirectory(.. "${CMAKE_CURRENT_BINARY_DIR}/build/serialization")
include_directories(../source)
target_link_libraries(${EXAMPLE_PROJECT_NAME} Serialization)

# add jsoncpp
add_subdirectory(jsoncpp/src "${CMAKE_CURRENT_BINARY_DIR}/build/jsoncpp")
include_directories(jsoncpp/include)
target_link_libraries(${EXAMPLE_PROJECT_NAME} jsoncpp_lib)

set_target_properties(${EXAMPLE_PROJECT_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
)
