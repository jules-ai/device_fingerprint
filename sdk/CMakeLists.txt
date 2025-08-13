# CMakeLists.txt for the fingerprint SDK

# Add the library target
add_library(fingerprint STATIC src/fingerprint.cpp)
add_library(fingerprint::fingerprint ALIAS fingerprint)

# Define the public include directory.
# This makes sure that consumers of this library can find the headers.
target_include_directories(fingerprint PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Platform-specific dependencies
if(APPLE)
    find_library(IOKIT_LIBRARY NAMES IOKit REQUIRED)
    find_library(CORE_FOUNDATION_LIBRARY NAMES CoreFoundation REQUIRED)
    target_link_libraries(fingerprint PUBLIC ${IOKIT_LIBRARY} ${CORE_FOUNDATION_LIBRARY})
endif()

if(WIN32)
    target_link_libraries(fingerprint PUBLIC ole32 oleaut32 wbemuuid)
endif()

# Installation rules
install(TARGETS fingerprint
    EXPORT fingerprint-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT fingerprint-targets
    FILE fingerprint-config.cmake
    NAMESPACE fingerprint::
    DESTINATION lib/cmake/fingerprint
)
