#include "reliable_asio_udp.h"
#include <cstdint>
#include <memory>
#include <thread>

std::array<uint8_t, 1024> data_buffer;

void thread_handle_callback(void *data_ptr)
{
    int ret = 0;
    ReliableASIOUDP *raudp_ptr = (ReliableASIOUDP *)(data_ptr);

    // raudp_ptr->spin();
    while(1)
    {
        ret = raudp_ptr->receive(data_buffer.data(), data_buffer.size());
        if (ret > 0)
        {
            std::cout << "recv ret:" << ret << std::endl;
        }
        sleep(1);
    }
    
}

int main()
{
    int ret = 0;
    std::string ip = "127.0.0.1";
    
    std::unique_ptr<ReliableASIOUDP> rap_ptr = std::make_unique<ReliableASIOUDP>(0x11, ip, 36001);
    std::unique_ptr<std::thread> task_ptr = std::make_unique<std::thread>(thread_handle_callback, rap_ptr.get());
    
    rap_ptr->initialize();

    rap_ptr->start();

    while(1)
    {
        rap_ptr->run();
        usleep(10000);
        rap_ptr->spin_once();   
    }
    
    return 0;
}