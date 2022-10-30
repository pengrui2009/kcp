#include "udp_kcp.h"
#include "logger.h"

int main()
{
    int ret = 0;
    char log_filepath[] = "client.log";
    std::string client_ip = "127.0.0.1";
    std::string server_ip = "127.0.0.1";
    uint16_t client_port = 35001;
    uint16_t server_port = 36001;


    LOG_INIT(log_filepath);

    std::unique_ptr<KcpClient> client_ptr = 
        std::make_unique<KcpClient>(client_ip, client_port);

    LOG_INFO("client initialize");
    client_ptr->initialize();
    LOG_INFO("client connect to [%s:%d]", server_ip.c_str(), server_port);
    ret = client_ptr->connect(server_ip, server_port);
    if (ret)
    {
        LOG_INFO("client connect failed");
        return -1;
    }
    LOG_INFO("client runing");
    client_ptr->run();

    while(1)
    {
        sleep(2);
        uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
        ret = client_ptr->send_kcp_packet(buffer, sizeof(buffer));
        LOG_INFO("send_kcp_packet ret:%d", ret);
        
    }
    
    return 0;
}