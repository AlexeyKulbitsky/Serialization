cmake_minimum_required(VERSION 3.4.1)

project (Serialization)

set(SERIALIZATION_PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/source)
set(SERIALIZATION_PROJECT_NAME Serialization)

if(WIN32)
  SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11")
elseif(APPLE)
  SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11")
endif()

set(SERIALIZATION_LIB_INCLUDE
  source/Property.h
  source/ObjectFactory.h
  source/ObjectDesc.h
  source/Serializer.h
  source/TypeInfo.h
  source/TypeTraits.h
  source/TempContainer.h
)

set(SERIALIZATION_LIB_SRC
  source/Property.cpp
  source/ObjectFactory.cpp
  source/Serializer.cpp
)

set(SERIALIZATION_LIB_SOURCE_FILES
  ${SERIALIZATION_LIB_INCLUDE}    
  ${SERIALIZATION_LIB_SRC}
)

include_directories(source)

# Group files according to files organization
foreach(FILE ${SERIALIZATION_LIB_SOURCE_FILES}) 
    # Get the directory of the source file
    get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

    # Remove common directory prefix to make the group
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")

    # Make sure we are using windows slashes
    string(REPLACE "/" "\\" GROUP "${GROUP}")

    source_group("${GROUP}" FILES "${FILE}")
endforeach()

add_library(${SERIALIZATION_PROJECT_NAME} ${SERIALIZATION_LIB_SOURCE_FILES})

#-----------------

set(SERIALIZATION_PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/source)
set(EXAMPLE_PROJECT_NAME Example)

set(EXAMPLE_APP_SRC
  source/main.cpp
)

set(EXAMPLE_APP_SOURCE_FILES
  ${EXAMPLE_APP_SRC}
)

add_executable(${EXAMPLE_PROJECT_NAME} ${EXAMPLE_APP_SOURCE_FILES})
target_link_libraries(${EXAMPLE_PROJECT_NAME} ${SERIALIZATION_PROJECT_NAME})

set_target_properties(${APPLICATION_PROJECT_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
)
