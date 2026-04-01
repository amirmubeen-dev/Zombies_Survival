cmake_minimum_required(VERSION 3.16)
project(ZombieSurvival3D CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Build type ----
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---- Source files ----
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/**/*.cpp")
# Ensure we get main.cpp if glob misses it
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/src/main.cpp")
list(REMOVE_DUPLICATES SOURCES)

# ---- Executable ----
add_executable(${PROJECT_NAME} ${SOURCES})

# ---- Include directories ----
target_include_directories(${PROJECT_NAME} PRIVATE
    src/
    vendor/
)

# ---- Find packages ----
find_package(OpenGL REQUIRED)

# Try pkg-config first (MSYS2 typically has this)
find_package(PkgConfig QUIET)

if(PkgConfig_FOUND)
    pkg_check_modules(GLFW glfw3)
    pkg_check_modules(GLEW glew)
    pkg_check_modules(GLM glm)
endif()

# Fallback: manual search for MSYS2 MinGW64 paths
if(NOT GLFW_FOUND)
    find_library(GLFW_LIBRARIES NAMES glfw3 glfw
        PATHS "C:/msys64/mingw64/lib" "/mingw64/lib")
    find_path(GLFW_INCLUDE_DIRS GLFW/glfw3.h
        PATHS "C:/msys64/mingw64/include" "/mingw64/include")
    set(GLFW_FOUND TRUE)
endif()

if(NOT GLEW_FOUND)
    find_library(GLEW_LIBRARIES NAMES glew32 GLEW
        PATHS "C:/msys64/mingw64/lib" "/mingw64/lib")
    find_path(GLEW_INCLUDE_DIRS GL/glew.h
        PATHS "C:/msys64/mingw64/include" "/mingw64/include")
    set(GLEW_FOUND TRUE)
endif()

if(NOT GLM_FOUND)
    find_path(GLM_INCLUDE_DIRS glm/glm.hpp
        PATHS "C:/msys64/mingw64/include" "/mingw64/include")
endif()

# ---- OpenMP ----
find_package(OpenMP REQUIRED)

# ---- Optional: OpenAL ----
find_library(OPENAL_LIB NAMES OpenAL32 openal
    PATHS "C:/msys64/mingw64/lib" "/mingw64/lib")
if(OPENAL_LIB)
    find_path(OPENAL_INCLUDE AL/al.h
        PATHS "C:/msys64/mingw64/include" "/mingw64/include")
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_OPENAL)
    target_include_directories(${PROJECT_NAME} PRIVATE ${OPENAL_INCLUDE})
    message(STATUS "[OpenAL] Found: ${OPENAL_LIB}")
else()
    message(STATUS "[OpenAL] Not found — audio disabled")
endif()

# ---- Include & link ----
target_include_directories(${PROJECT_NAME} PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    ${OPENGL_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${GLEW_LIBRARIES}
    OpenMP::OpenMP_CXX
)

if(OPENAL_LIB)
    target_link_libraries(${PROJECT_NAME} ${OPENAL_LIB})
endif()

# Windows: also link gdi32, user32 for GLFW
if(WIN32)
    target_link_libraries(${PROJECT_NAME} gdi32 user32)
endif()

# ---- Compiler flags ----
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:-Wall -Wextra -g>
    $<$<CONFIG:Release>:-O2>
)
if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    # Suppress console window in release
    # -mwindows causes linker issue with some stdcout, keep it off for safety
    # target_link_options(${PROJECT_NAME} PRIVATE -mwindows)
endif()

# OpenMP linker flag
if(WIN32)
    target_link_options(${PROJECT_NAME} PRIVATE -fopenmp)
endif()

# ---- Copy assets to build dir ----
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
        COMMENT "Copying assets..."
    )
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "GLFW:  ${GLFW_LIBRARIES}")
message(STATUS "GLEW:  ${GLEW_LIBRARIES}")
message(STATUS "OpenGL: ${OPENGL_LIBRARIES}")
