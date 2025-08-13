# CMakeLists.txt for the demo application

# Add the executable target
add_executable(fingerprint_app main.cpp)

# Link against the fingerprint library
# The 'fingerprint' target is available from the parent directory's scope
target_link_libraries(fingerprint_app PRIVATE fingerprint::fingerprint)
