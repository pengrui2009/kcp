#include "server.h"
#include "udp_kcp.h"
#include "logger.h"

#include <memory>

Server::Server(std::string ip, uint16_t port)
{
    server_ptr_ = std::make_shared<KcpServer>(ip, port);
}

Server::~Server()
{
    server_ptr_.reset();
}

int Server::initialize(
    std::function<connect_event_callback_t> connect_callback,
    std::function<disconnect_event_callback_t> disconnect_callback,
    std::function<receive_event_callback_t> receive_callback)
{
    int ret = 0;

    ret = server_ptr_->initialize();
    if (ret)
    {
        LOG_ERROR("server initialize ret:%d", ret);
        return -1;
    }

    connect_callback_ = connect_callback;
    disconnect_callback_ = disconnect_callback;
    recvdata_callback_ = receive_callback;

    server_ptr_->set_event_callback(std::bind(&Server::handle_event_callback, this, 
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

    return 0;
}

int Server::run()
{
    int ret = 0;

    ret = server_ptr_->run();
    if (ret)
    {
        LOG_ERROR("server initialize ret:%d", ret);
        return -1;
    }

    return 0;
}

int Server::deinitialize()
{
    return 0;
}

int Server::send(const kcp_conv_t conv, const uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;

    ret = server_ptr_->send_kcp_packet(conv, data_ptr, data_len);
    if (ret)
    {
        LOG_ERROR("send_kcp_packet failed ret:%d", ret);
        return -1;
    }
    // ikcp_flush(ikcpcb *kcp)
    return 0;
}

void Server::handle_event_callback(ukcp_info_t ukcp, EventType event_type, 
    uint8_t *data_ptr, size_t data_len)
{
    switch (event_type)
    {
    case EVENT_TYPE_CONNECT:
        LOG_INFO("EVENT_TYPE_CONNECT");
        break;
    case EVENT_TYPE_DISCONNECT:
        LOG_INFO("EVENT_TYPE_DISCONNECT");
        break;
    case EVENT_TYPE_RECEIVE:
        // LOG_INFO("EVENT_TYPE_RECEIVE data_len:%d", data_len);
        recvdata_callback_(ukcp, event_type, data_ptr, data_len);
        break;
    default:
        break;    
    }
}
