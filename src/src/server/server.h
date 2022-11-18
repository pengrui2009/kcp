#ifndef SERVER_H
#define SERVER_H

#include "udp_kcp.h"
#include "typedef.h"
#include <memory>

typedef void(connect_event_callback_t)(ukcp_info_t, EventType);
typedef void(disconnect_event_callback_t)(ukcp_info_t, EventType);
typedef void(receive_event_callback_t)(ukcp_info_t, EventType, uint8_t *, size_t);

class Server {
public:
    Server(std::string ip, uint16_t port);
    ~Server();

    int initialize(std::function<connect_event_callback_t> ,
        std::function<disconnect_event_callback_t> ,
        std::function<receive_event_callback_t> );

    int deinitialize();

    int send(const kcp_conv_t conv, const uint8_t *data_ptr, size_t data_len);

    int run();
protected:
    void handle_event_callback(ukcp_info_t, EventType, uint8_t *data_ptr, size_t data_len);
private:
    std::shared_ptr<KcpServer> server_ptr_;

    std::function<connect_event_callback_t> connect_callback_;
    std::function<disconnect_event_callback_t> disconnect_callback_;
    std::function<receive_event_callback_t> recvdata_callback_;
};

#endif /* SERVER_H */