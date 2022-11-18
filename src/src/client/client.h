#ifndef CLIENT_H
#define CLIENT_H

#include "udp_kcp.h"
#include "typedef.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <functional>

typedef void(connect_event_callback_t)(ukcp_info_t, EventType);
typedef void(disconnect_event_callback_t)(ukcp_info_t, EventType);
typedef void(receive_event_callback_t)(ukcp_info_t, EventType, uint8_t *, size_t);

class Client {
public:
    Client(std::string ip, uint16_t port);
    ~Client();

    int initialize(std::function<connect_event_callback_t> ,
        std::function<disconnect_event_callback_t> ,
        std::function<receive_event_callback_t> );

    int connect(const std::string &server_ip, const uint16_t server_port);
    
    int disconnect();
    
    int deinitialize();

    int send(const uint8_t *data_ptr, size_t data_len);

    int run();

protected:
    void handle_event_callback(ukcp_info_t, EventType, uint8_t *data_ptr, size_t data_len);

private:
    std::shared_ptr<KcpClient> client_ptr_;

    std::function<connect_event_callback_t> connect_callback_;
    std::function<disconnect_event_callback_t> disconnect_callback_;
    std::function<receive_event_callback_t> recvdata_callback_;
};

#endif /* CLIENT_H */
