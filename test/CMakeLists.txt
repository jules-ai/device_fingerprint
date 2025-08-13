# CMakeLists.txt for tests

# Define the test executable
add_executable(fingerprint_test test.cpp)

# Link against the fingerprint library
target_link_libraries(fingerprint_test PRIVATE fingerprint::fingerprint)

# Add the test to CTest
add_test(NAME fingerprint_unit_test
    COMMAND fingerprint_test
)
