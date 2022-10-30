#include "udp_kcp.h"
#include "logger.h"

int main()
{
    char log_filepath[] = "server.log";
    std::string server_ip = "127.0.0.1";
    uint16_t server_port = 36001;

    LOG_INIT(log_filepath);

    std::unique_ptr<KcpServer> server_ptr = 
        std::make_unique<KcpServer>(server_ip, server_port);

    LOG_INFO("server initialize");
    server_ptr->initialize();

    LOG_INFO("server runing");
    server_ptr->run();
    
    return 0;
}