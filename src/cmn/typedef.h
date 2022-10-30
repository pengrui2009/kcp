#ifndef TYPEDEF_H
#define TYPEDEF_H

#include <stdint.h>

#include <string>
#include <boost/asio.hpp>

typedef unsigned int kcp_conv_t;
typedef boost::asio::ip::udp::endpoint asio_endpoint_t;

// typedef uint32_t kcp_conv_t;
// typedef int (*kcp_output_func)(const char *buf, int len, struct IKCPCB *kcp, void *user);
typedef int (kcp_output_func)(const char *buf, int len, struct IKCPCB *kcp, void *user);

enum EventType
{
    EVENT_TYPE_CONNECT,
    EVENT_TYPE_DISCONNECT,
    EVENT_TYPE_RECEIVE,
    EVENT_TYPE_END
};

struct ukcp_info_t {
    kcp_conv_t conv;
    asio_endpoint_t endpoint;
};

typedef void (event_callback_func)(ukcp_info_t , EventType , uint8_t *data_ptr, size_t data_len);

#endif /* TYPEDEF_H */