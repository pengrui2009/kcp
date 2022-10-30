#ifndef SERVER_H
#define SERVER_H

#include "udp_kcp.h"
#include "typedef.h"
#include <memory>

class Server {
public:
    Server(std::string ip, uint16_t port);
    ~Server();

    int initialize();

    int deinitialize();

    int send(const kcp_conv_t conv, const uint8_t *data_ptr, size_t data_len);

    int run();
protected:
    void handle_event_callback(ukcp_info_t, EventType, uint8_t *data_ptr, size_t data_len);
private:
    std::shared_ptr<KcpServer> server_ptr_;
};

#endif /* SERVER_H */