#include "udp_kcp.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    int ret  = 0;
    char log_filepath[] = "server.log";
    auto server_ptr = std::make_unique<UKcp>("", 36001);
    
    LOG_INIT(log_filepath);

    LOG_INFO("ukcp server initialize...");
    ret = server_ptr->initialize();
    if (ret)
    {
        LOG_ERROR("server initialize failed, ret:%d", ret);
        return -1;
    }

    while(1)
    {
        sleep(1);
    }

    return 0;
}