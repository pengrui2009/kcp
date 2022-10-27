#include "reliable_asio_udp.h"
#include <array>
#include <cstdint>
#include <memory>
#include <thread>

void thread_handle_callback(void *data_ptr)
{
    ReliableASIOUDP *raudp_ptr = (ReliableASIOUDP *)(data_ptr);

    raudp_ptr->spin();
}

int main()
{
    std::string ip = "127.0.0.1";
    std::string server_ip = "127.0.0.1";
    std::unique_ptr<ReliableASIOUDP> rap_ptr = std::make_unique<ReliableASIOUDP>(0x11, ip, 37001);
    std::unique_ptr<std::thread> task_ptr = std::make_unique<std::thread>(thread_handle_callback, rap_ptr.get());

    rap_ptr->initialize();

    rap_ptr->connect(server_ip, 36001);

    rap_ptr->start();

    while(1)
    {
        //std::array<uint8_t, std::size_t _Nm>
        uint8_t buffer[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

        rap_ptr->run();

        // usleep(100000);
        sleep(1);

        rap_ptr->send(buffer, sizeof(buffer));
        std::cout << "rap send data" << std::endl;

        // ikcp_waitsnd()
    }

    return 0;
}