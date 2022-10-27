CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

project(kcp LANGUAGES CXX C)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")


include(CTest)
include(GNUInstallDirs)

add_library(kcp STATIC ikcp.c)

install(FILES ikcp.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS kcp
    EXPORT kcp-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT kcp-targets
    FILE kcp-config.cmake
    NAMESPACE kcp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kcp
)

if (BUILD_TESTING)
    enable_language(CXX)
    
    add_executable(kcp_test test.cpp)
    if(MSVC AND NOT (MSVC_VERSION LESS 1900))
        target_compile_options(kcp_test PRIVATE /utf-8)
    endif()
endif ()


include_directories(
    ${CMAKE_SOURCE_DIR}
    src
)

# build reliable asio udp library
add_library(reliable_asio_udp STATIC
    src/timer.cpp
    src/timestamp.cpp
    src/reliable_asio_udp.cpp
)

# build test client
add_executable(kcp_client_test
    # src/client/frame_handler.cpp
    examples/client.cpp
)

target_link_libraries(kcp_client_test
    kcp
    pthread
    boost_system
    # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
)

# build test server
add_executable(kcp_server_test
    # src/client/frame_handler.cpp
    examples/server.cpp
)

target_link_libraries(kcp_server_test
    kcp
    pthread    
    boost_system
    # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
)

# build reliable asio udp
add_executable(server_test
    src/server.cpp
)

target_link_libraries(server_test
    reliable_asio_udp
    kcp
    pthread    
    boost_system
    # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
)

add_executable(client_test
    src/client.cpp
)

target_link_libraries(client_test
    reliable_asio_udp
    kcp
    pthread    
    boost_system
    # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
)