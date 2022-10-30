#include "server.h"
#include "logger.h"

#include <iostream>

int main()
{
    char log_filepath[] = "server.log";
    std::string server_ip = "127.0.0.1";
    uint16_t server_port = 36001;

    LOG_INIT(log_filepath);

    std::unique_ptr<Server> server_ptr = 
        std::make_unique<Server>(server_ip, server_port);

    LOG_INFO("server initialize");
    server_ptr->initialize();

    LOG_INFO("server runing");
    server_ptr->run();


  
    return 0;
}