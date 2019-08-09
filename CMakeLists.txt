cmake_minimum_required(VERSION 3.4.1)

project (Application)

# Add engine

set(APPLICATION_PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/source)
set(APPLICATION_PROJECT_NAME Application)

if(WIN32)
  SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11")
elseif(APPLE)
  SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11")
endif()

set(APPLICATION_INCLUDE
  source/Serializable.h
  source/Property.h
  source/ObjectFactory.h
  source/ObjectDesc.h
  source/Serializer.h
  source/TypeInfo.h
  source/TypeTraits.h
)

set(APPLICATION_SRC
  source/main.cpp
  source/Serializable.cpp
  source/Property.cpp
  source/ObjectFactory.cpp
  source/Serializer.cpp
)

set(APPLICATION_PROJECT_SOURCE_FILES
  ${APPLICATION_INCLUDE}    
  ${APPLICATION_SRC}
)

include_directories(source)

# Group files according to files organization
foreach(FILE ${APPLICATION_PROJECT_SOURCE_FILES}) 
    # Get the directory of the source file
    get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

    # Remove common directory prefix to make the group
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")

    # Make sure we are using windows slashes
    string(REPLACE "/" "\\" GROUP "${GROUP}")

    source_group("${GROUP}" FILES "${FILE}")
endforeach()

add_executable(${APPLICATION_PROJECT_NAME} ${APPLICATION_PROJECT_SOURCE_FILES})

set_target_properties(${APPLICATION_PROJECT_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
)
