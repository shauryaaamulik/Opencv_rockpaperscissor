cmake_minimum_required(VERSION 3.10)
project(HandGestureDetection)

set(CMAKE_CXX_STANDARD 14)

# Find OpenCV package
find_package(OpenCV REQUIRED)

# Include OpenCV headers
include_directories(${OpenCV_INCLUDE_DIRS})

# Create the executable
add_executable(HandGesture main.cpp)

# Link OpenCV libraries to the executable
target_link_libraries(HandGesture ${OpenCV_LIBS})