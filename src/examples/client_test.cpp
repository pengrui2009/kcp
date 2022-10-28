#include "udp_kcp.h"
#include "logger.h"

#include <memory>
#include <string>

int main(int argc, char *argv[])
{
    int ret = 0;
    char log_filepath[] = "client.log";
    std::string client_ip = "127.0.0.1";
    uint16_t client_port = 37001;

    std::string server_ip = "127.0.0.1";
    uint16_t server_port = 36001;

    std::unique_ptr<UKcp> server_ptr = std::make_unique<UKcp>(client_ip, client_port);
    
    LOG_INIT(log_filepath);

    LOG_INFO("ukcp server initialize...");
    ret  = server_ptr->initialize();
    if (ret)
    {
        LOG_ERROR("server initialize failed, ret:%d", ret);
        return -1;
    }

    ret  = server_ptr->connect(server_ip, server_port);
    if (ret)
    {
        LOG_ERROR("server connect failed, ret:%d", ret);
        return -1;
    }

    while(1)
    {
        sleep(1);
    }

    return 0;
}