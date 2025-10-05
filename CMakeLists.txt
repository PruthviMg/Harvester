cmake_minimum_required(VERSION 3.15)
project(Harvestor VERSION 1.0 LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------
# Find SFML
# ------------------------
find_package(SFML 2.5 REQUIRED COMPONENTS graphics window system)

# ------------------------
# Collect sources
# ------------------------
file(GLOB_RECURSE SOURCE_FILES
    src/src/*.cpp
)

file(GLOB_RECURSE HEADER_FILES
    src/inc/*.hpp
    src/inc/*.h
)

# ------------------------
# Executable
# ------------------------
add_executable(${PROJECT_NAME}
    ${SOURCE_FILES}
    ${HEADER_FILES}   # optional, helps IDEs
)

# ------------------------
# Include directories
# ------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    src/inc
)

# ------------------------
# Link libraries
# ------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    curl
    
)

# ------------------------
# Copy resources
# ------------------------
file(COPY input/ DESTINATION ${CMAKE_BINARY_DIR}/input/)
file(COPY layouts/ DESTINATION ${CMAKE_BINARY_DIR}/layouts/)
file(COPY resources/ DESTINATION ${CMAKE_BINARY_DIR}/resources)
file(COPY report/land.csv DESTINATION ${CMAKE_BINARY_DIR}/input/)
file(COPY report/water.csv DESTINATION ${CMAKE_BINARY_DIR}/input/)

