cmake_minimum_required(VERSION 3.15)
project(Harvestor VERSION 1.0 LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SFML (graphics, window, system)
find_package(SFML 2.5 REQUIRED COMPONENTS graphics window system)

# Collect source files (only from src, not CMake files)
file(GLOB_RECURSE SOURCE_FILES
    src/*.cpp
)

# Collect headers (optional, for IDEs)
file(GLOB_RECURSE HEADER_FILES
    src/*.hpp
    src/*.h
)

# Executable
add_executable(${PROJECT_NAME}
    ${SOURCE_FILES}
    ${HEADER_FILES}   # Optional: helps IDEs see headers
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    src/Common/inc
    src/Platform/inc
    src/Manager/inc
    src/Configurator/inc
    src/Feature/inc
)

# Link SFML
target_link_libraries(${PROJECT_NAME} PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
)

file(COPY src/images/ DESTINATION ${CMAKE_BINARY_DIR}/images)
file(COPY src/images/DejaVuSans.ttf DESTINATION ${CMAKE_BINARY_DIR}/images)

