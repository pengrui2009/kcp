#include "client.h"
#include "udp_kcp.h"
#include "logger.h"

#include <memory>

Client::Client(std::string ip, uint16_t port)
{
    client_ptr_ = std::make_shared<KcpClient>(ip, port);
}

Client::~Client()
{
    client_ptr_.reset();
}

int Client::initialize(std::function<connect_event_callback_t> connect_callback, 
    std::function<disconnect_event_callback_t> disconnect_callback,
    std::function<receive_event_callback_t> receive_callback)
{
    int ret = 0;

    ret = client_ptr_->initialize();
    if (ret)
    {
        LOG_ERROR("client initialize ret:%d", ret);
        return -1;
    }

    connect_callback_ = connect_callback;
    disconnect_callback_ = disconnect_callback;
    recvdata_callback_ = receive_callback;

    client_ptr_->set_event_callback(std::bind(&Client::handle_event_callback, this, 
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

    return 0;
}

int Client::connect(const std::string &server_ip, const uint16_t server_port)
{
    int ret = 0;

    ret = client_ptr_->connect(server_ip, server_port);
    if (ret)
    {
        LOG_ERROR("client connect ret:%d", ret);
        return -1;
    }
    
    return 0;
}

int Client::run()
{
    int ret = 0;

    ret = client_ptr_->run();
    if (ret)
    {
        LOG_ERROR("server initialize ret:%d", ret);
        return -1;
    }

    return 0;
}

int Client::disconnect()
{
    int ret = 0;

    ret = client_ptr_->disconnect();
    if (ret)
    {
        LOG_ERROR("server disconnect ret:%d", ret);
        return -1;
    }

    return 0;
}

int Client::deinitialize()
{
    return 0;
}

int Client::send(const uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;

    ret = client_ptr_->send_kcp_packet(data_ptr, data_len);
    if (ret)
    {
        LOG_ERROR("send_kcp_packet failed ret:%d", ret);
        return -1;
    }
    return 0;
}

void Client::handle_event_callback(ukcp_info_t ukcp, EventType event_type, 
    uint8_t *data_ptr, size_t data_len)
{
    switch (event_type)
    {
    case EVENT_TYPE_CONNECT:
        LOG_ERROR("EVENT_TYPE_CONNECT");
        break;
    case EVENT_TYPE_DISCONNECT:
        LOG_ERROR("EVENT_TYPE_DISCONNECT");
        break;
    case EVENT_TYPE_RECEIVE:
        // LOG_ERROR("EVENT_TYPE_RECEIVE data_len:%d", data_len);
        recvdata_callback_(ukcp, event_type, data_ptr, data_len);
        break;
    default:
        break;    
    }
}
