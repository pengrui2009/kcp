#include "client.h"
#include "msg_frame.h"
#include "timestamp.h"
#include "logger.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

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
    double delta = 0.0;
    uint64_t current_timestamp = Timestamp::NanoSecond();
    uint64_t msg_timestamp = 0;
    std::shared_ptr<MsgFrame> frame_ptr = std::make_shared<MsgFrame>(sign_key);
    
    ret = frame_ptr->decode(data_ptr, data_len);
    if (ret)
    {
        
    }
    msg_timestamp = frame_ptr->get_msgtimestamp();
    delta = (current_timestamp - msg_timestamp) * 1e-6;
    LOG_INFO("seq:%d delay:%lfms", frame_ptr->get_seqnum(), delta);
}

int main(int argc, char *argv[])
{
    int ret = 0;
    char log_filepath[] = "client.log";
    std::string client_ip = "0.0.0.0";
    // std::string server_ip = "182.92.70.200";
    std::string server_ip = "127.0.0.1";
    uint16_t client_port = 35001;
    uint16_t server_port = 5220;
    size_t send_buffer_size = 0;
    uint32_t seqnum = 0;
    std::vector<uint8_t> send_buffer;

    std::shared_ptr<MsgFrame> msgframe_ptr = std::make_shared<MsgFrame>(
        sign_key);

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << "  packet_size" << std::endl;
        return -1;
    }
    send_buffer_size = atoi(argv[1]);
    // client_ip = argv[1];

    LOG_INIT(log_filepath);

    std::unique_ptr<Client> client_ptr = 
        std::make_unique<Client>(client_ip, client_port);

    LOG_INFO("client initialize");
    LOG_INFO("test packet size:%d", send_buffer_size);
    client_ptr->initialize(
        std::bind(&client_connect_callback, std::placeholders::_1, std::placeholders::_2),
        std::bind(&client_disconnect_callback, std::placeholders::_1, std::placeholders::_2), 
        std::bind(&client_receive_callback, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    LOG_INFO("client connect to [%s:%d]", server_ip.c_str(), server_port);
    ret = client_ptr->connect(server_ip, server_port);
    if (ret)
    {
        LOG_INFO("client connect failed");
        return -1;
    }
    LOG_INFO("client runing");
    client_ptr->run();

    // uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    for (int i=0; i< send_buffer_size; i++)
    {
        send_buffer.push_back(i%255);
    }
    seqnum = 0;
    while(1)
    {
        usleep(50000);
        uint64_t timestamp = Timestamp::NanoSecond();

        ret = msgframe_ptr->encode(timestamp, 0, seqnum++, send_buffer);
        if (ret)
        {
            LOG_ERROR("msg frame encode failed.");
            continue;
        }
        std::vector<uint8_t> buffer = msgframe_ptr->get_buffer();
        ret = client_ptr->send(buffer.data(), buffer.size());
        if (ret)
        {
            LOG_ERROR("send kcp packet failed.");
        }
        // LOG_INFO("send_kcp_packet ret:%d", ret);
        
    }
    
    return 0;
}