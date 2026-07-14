cmake_minimum_required(VERSION 3.20)

project(OptionLab
    VERSION 0.1.0
    DESCRIPTION "Parallel American/Bermudan options pricer with vol surface calibration"
    LANGUAGES CXX)

# ---- Language / build defaults ---------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Put binaries in build/bin, libraries in build/lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---- Dependencies ----------------------------------------------------------
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS json)
find_package(OpenSSL REQUIRED)

# QuantLib (Homebrew keg-only install has no CMake config, so locate manually)
find_path(QUANTLIB_INCLUDE_DIR
    NAMES ql/quantlib.hpp
    HINTS /opt/homebrew/opt/quantlib/include
          /usr/local/opt/quantlib/include
          /usr/local/include)
find_library(QUANTLIB_LIBRARY
    NAMES QuantLib
    HINTS /opt/homebrew/opt/quantlib/lib
          /usr/local/opt/quantlib/lib
          /usr/local/lib)

# ---- Core library ----------------------------------------------------------
# All engine sources live under src/; glob keeps this simple while scaffolding.
file(GLOB_RECURSE OPTIONLAB_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM OPTIONLAB_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)

add_library(optionlab ${OPTIONLAB_SOURCES})
target_include_directories(optionlab PUBLIC ${CMAKE_SOURCE_DIR}/include/optionlab)
target_link_libraries(optionlab PUBLIC Threads::Threads)
target_link_libraries(optionlab PUBLIC Boost::boost Boost::json)
target_link_libraries(optionlab PRIVATE OpenSSL::SSL OpenSSL::Crypto)

if(QUANTLIB_INCLUDE_DIR AND QUANTLIB_LIBRARY)
    target_include_directories(optionlab PUBLIC ${QUANTLIB_INCLUDE_DIR})
    target_link_libraries(optionlab PUBLIC ${QUANTLIB_LIBRARY})
    message(STATUS "QuantLib found: ${QUANTLIB_LIBRARY}")
else()
    message(WARNING "QuantLib not found — building without it for now")
endif()

# ---- Main executable -------------------------------------------------------
add_executable(optionlab_app ${CMAKE_SOURCE_DIR}/src/main.cpp)
target_link_libraries(optionlab_app PRIVATE optionlab)
set_target_properties(optionlab_app PROPERTIES OUTPUT_NAME optionlab)
