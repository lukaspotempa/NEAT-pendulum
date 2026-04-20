cmake_minimum_required(VERSION 3.15)
project(Cart_Pendulum CXX)

set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find all source files in the project
file(GLOB_RECURSE SRC_FILES "src/*.cpp")

list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/Cart_Pendulum.cpp")
file(GLOB_RECURSE HEADER_FILES "include/*.hpp")

# Create the executable
add_executable(Cart_Pendulum ${SRC_FILES} ${HEADER_FILES})

# Specify include directories
target_include_directories(Cart_Pendulum PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/physics
    ${CMAKE_CURRENT_SOURCE_DIR}/include/neat
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/include/utils
)

# --- SFML Linking ---
# Adding the hardcoded path from your Visual Studio project
set(SFML_ROOT "F:/Projects/.NET/libraries/SFML-3.0.2")

target_include_directories(Cart_Pendulum PRIVATE "${SFML_ROOT}/include")

# For the linker, we configure the library paths
target_link_directories(Cart_Pendulum PRIVATE "${SFML_ROOT}/lib")


target_link_libraries(Cart_Pendulum PRIVATE
    debug sfml-graphics-d
    optimized sfml-graphics
    debug sfml-window-d
    optimized sfml-window
    debug sfml-system-d
    optimized sfml-system
)


add_custom_command(TARGET Cart_Pendulum POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${SFML_ROOT}/bin"
    "$<TARGET_FILE_DIR:Cart_Pendulum>"
)
