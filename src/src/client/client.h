#ifndef CLIENT_H
#define CLIENT_H

#include "udp_kcp.h"
#include "typedef.h"
#include <cstdint>
#include <memory>


class Client {
public:
    Client(std::string ip, uint16_t port);
    ~Client();

    int initialize();

    int connect(const std::string &server_ip, const uint16_t server_port);
    
    int disconnect();
    
    int deinitialize();

    int send(const uint8_t *data_ptr, size_t data_len);

    int run();

protected:
    void handle_event_callback(ukcp_info_t, EventType, uint8_t *data_ptr, size_t data_len);

private:
    std::shared_ptr<KcpClient> client_ptr_;
};

#endif /* CLIENT_H */
