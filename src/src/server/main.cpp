#include "server.h"

#include "msg_frame.h"
#include "timestamp.h"
#include "logger.h"

#include <cstdint>
#include <iostream>

std::unique_ptr<Server> server_ptr = nullptr;
std::string sign_key = "44090ace2d06e9a2c386c097b7b54bcbd4dc2154dc9200b4b4041e97fe1ee7c8";

void client_connect_callback(ukcp_info_t kcp, EventType type)
{

}

void client_disconnect_callback(ukcp_info_t kcp, EventType type)
{

}

void client_receive_callback(ukcp_info_t kcp, EventType type, uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;
    std::shared_ptr<MsgFrame> frame_ptr = std::make_shared<MsgFrame>(sign_key);
    uint64_t current_timestamp = Timestamp::NanoSecond();
    uint64_t msg_timestamp = 0;
    double delta = 0.0;

    ret = frame_ptr->decode(data_ptr, data_len);
    if (ret)
    {
        return;
    }
    msg_timestamp = frame_ptr->get_msgtimestamp();
    
    delta = (current_timestamp - msg_timestamp) * 1e-6;
    
    LOG_INFO("seq:%d delay:%lfms", frame_ptr->get_seqnum(), delta);

    ret = server_ptr->send(kcp.conv, data_ptr, data_len);
    if (ret)
    {
        LOG_ERROR("server send failed, ret:%d", ret);
    }
}

int main()
{
    char log_filepath[] = "server.log";
    std::string server_ip = "0.0.0.0";
    uint16_t server_port = 5220;

    LOG_INIT(log_filepath);

    server_ptr = std::make_unique<Server>(server_ip, server_port);

    LOG_INFO("server initialize");
    server_ptr->initialize(
        std::bind(&client_connect_callback, std::placeholders::_1, std::placeholders::_2),
        std::bind(&client_disconnect_callback, std::placeholders::_1, std::placeholders::_2), 
        std::bind(&client_receive_callback, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
    );

    LOG_INFO("server runing");
    server_ptr->run();


  
    return 0;
}