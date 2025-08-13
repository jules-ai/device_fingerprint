# Root CMakeLists.txt for MachineFingerprint project

cmake_minimum_required(VERSION 3.10)
project(MachineFingerprint CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable testing with CTest
enable_testing()

# Add subdirectories for the SDK, demo, and tests
add_subdirectory(sdk)
add_subdirectory(demo)
add_subdirectory(test)
