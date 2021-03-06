########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8.9)
project(PothosPython)
enable_language(CXX)

find_package(Pothos CONFIG REQUIRED)
list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake)
include(PothosPythonUtil)

########################################################################
## Find python
########################################################################
find_package(PythonLibs)

message(STATUS "PYTHONLIBS_FOUND: ${PYTHONLIBS_FOUND} - ${PYTHONLIBS_VERSION_STRING}")
message(STATUS "PYTHON_INCLUDE_DIRS: ${PYTHON_INCLUDE_DIRS}")
message(STATUS "PYTHON_LIBRARIES: ${PYTHON_LIBRARIES}")

# enable python debug mode when the debug library is specified
if (PYTHON_DEBUG_LIBRARY)
    add_definitions(-DPy_DEBUG)
    set(PYTHON_DEBUG_POSTFIX _d)
else()
    unset(PYTHON_DEBUG_POSTFIX)
endif()

#on windows, we require a pythonxx_d.lib in debug mode
#require that the PYTHON_DEBUG_LIBRARY flag is set
#or the build assumes that the debug library DNE
if(WIN32 AND NOT PYTHON_DEBUG_LIBRARY AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    return()
endif()

if(NOT PYTHONLIBS_FOUND)
    return()
endif()

########################################################################
## Python<->Pothos proxy wrapper
########################################################################
include_directories(${PYTHON_INCLUDE_DIRS})

set(SOURCES
   PythonProxy.cpp
   PythonHandle.cpp
   PythonConvert.cpp
   TestPython.cpp
   TestPythonBlock.cpp
   PythonBlock.cpp
   ProxyHelpers.cpp
)

POTHOS_MODULE_UTIL(
    TARGET PythonSupport
    SOURCES ${SOURCES}
    LIBRARIES ${PYTHON_LIBRARIES}
    DESTINATION proxy/environment
)

########################################################################
# Install cmake helper modules
########################################################################
install(
    FILES
        cmake/PothosPythonUtil.cmake
        cmake/PothosPythonUtil.hpp
    DESTINATION ${POTHOS_CMAKE_DIRECTORY}
)

########################################################################
# Enter the subdirectory configuration
########################################################################
add_subdirectory(Pothos)
add_subdirectory(TestBlocks)
