cmake_minimum_required(VERSION 3.12)
project(LIS LANGUAGES CXX)

# -------------------------------
# Set C++ standard
# -------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -------------------------------
# Include directories
# -------------------------------
include_directories(${PROJECT_SOURCE_DIR}/include)

# -------------------------------
# Collect source files
# -------------------------------
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

# -------------------------------
# Create executable
# -------------------------------
add_executable(LIS ${SOURCES})

# -------------------------------
# Enable OpenMP if found
# -------------------------------
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    target_link_libraries(LIS PRIVATE OpenMP::OpenMP_CXX)
endif()