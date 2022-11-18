CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

project(kcp LANGUAGES C)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++14")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99")


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

enable_language(CXX)

include_directories(
    ${CMAKE_SOURCE_DIR}
    src
    src/cmn
    src/protocol
    src/src
    src/src/server
)

# build reliable asio udp library
# add_library(reliable_asio_udp STATIC
#     src/timer.cpp
#     src/timestamp.cpp
#     src/reliable_asio_udp.cpp
# )

add_library(ukcp_client STATIC
    src/cmn/logger.cpp    
    src/cmn/timer.cpp
    src/cmn/timestamp.cpp
    src/protocol/base_frame.cpp
    src/protocol/pair_frame.cpp
    src/protocol/request_connect_frame.cpp
    src/protocol/request_disconnect_frame.cpp
    src/protocol/response_frame.cpp
    src/protocol/heartbeat_frame.cpp
    src/protocol/msg_frame.cpp
    src/src/client/asio_udp.cpp
    src/src/client/udp_kcp.cpp
    src/src/client/client.cpp
)

target_link_libraries(ukcp_client    
    kcp
    ssl
    crypto
    pthread    
    boost_system
    # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
)

add_library(ukcp_server STATIC
    src/cmn/logger.cpp    
    src/cmn/timer.cpp
    src/cmn/timestamp.cpp
    src/protocol/base_frame.cpp
    src/protocol/pair_frame.cpp
    src/protocol/request_connect_frame.cpp
    src/protocol/request_disconnect_frame.cpp
    src/protocol/response_frame.cpp
    src/protocol/heartbeat_frame.cpp
    src/protocol/msg_frame.cpp
    src/src/server/asio_udp.cpp
    src/src/server/connection.cpp
    src/src/server/udp_kcp.cpp
    src/src/server/server.cpp
)

# target_link_libraries(ukcp_client    
#     kcp
#     ssl
#     crypto
#     pthread    
#     boost_system
#     # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
# )


# build reliable asio udp
# add_executable(server_test
#     src/server.cpp
# )

# target_link_libraries(server_test
#     reliable_asio_udp
#     kcp
#     pthread    
#     boost_system
#     # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
# )

# add_executable(client_test
#     src/client.cpp
# )

# target_link_libraries(client_test
#     reliable_asio_udp
#     kcp
#     pthread    
#     boost_system
#     # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
# )

# build reliable asio udp
# add_executable(server_test
#     src/examples/server_test.cpp
# )

# target_link_libraries(server_test
#     ukcp
#     kcp
#     pthread    
#     boost_system
#     # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
# )

# add_executable(client_test
#     src/examples/client_test.cpp
# )

# target_link_libraries(client_test
#     ukcp
#     kcp
#     pthread    
#     boost_system
#     # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
# )


add_executable(client
    src/src/client/main.cpp
)

target_link_libraries(client
    ukcp_client
    kcp
    ssl
    crypto
    pthread    
    boost_system
    # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
)

add_executable(server
    src/src/server/main.cpp
)

target_link_libraries(server
    ukcp_server
    kcp
    ssl
    crypto
    pthread    
    boost_system
    # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
)


# test kcp client and server
add_executable(kcp_client_test
    ikcp.c
    examples/kcp_client.cpp
)

target_link_libraries(kcp_client_test
    pthread    
    boost_system
    # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
)

add_executable(kcp_server_test
    ikcp.c
    examples/kcp_server.cpp
)

target_link_libraries(kcp_server_test
    pthread    
    boost_system
    # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
)

add_executable(random_conv_test
    src/examples/randomconv_test.cpp
)

target_link_libraries(random_conv_test
    pthread
    ukcp_server
    kcp
    ssl
    crypto
    boost_system
    # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
)

# build test client
add_executable(udp_client_test
    # src/client/frame_handler.cpp
    examples/udp_client.cpp
)

target_link_libraries(udp_client_test
    kcp
    pthread
    boost_system
    # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
)

# build test server
add_executable(udp_server_test
    # src/client/frame_handler.cpp
    examples/udp_server.cpp
)

target_link_libraries(udp_server_test
    kcp
    pthread    
    boost_system
    # ${CMAKE_SOURCE_DIR}/third_party/enet/lib/libenet.a
)
