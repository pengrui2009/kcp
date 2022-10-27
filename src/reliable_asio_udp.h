#ifndef RELIABLE_ASIO_UDP_H
#define RELIABLE_ASIO_UDP_H

#include "ikcp.h"
#include "timer.h"

#include <cstdint>
#include <iostream>
#include <atomic>
#include <thread>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/bind/bind.hpp>
#include <memory>
// #include "asio_udp.h"
// #include <memory>

constexpr size_t KEthPacketMaxLength = 4096;

enum TxMode {
    NORMAL_TX = 0,
    ASIO_TX = 1,
};

typedef boost::function<void(const boost::system::error_code&, const int&)> asio_func_callback;

class ReliableASIOUDP {
public:
    ReliableASIOUDP(IUINT32 conv, std::string &ip, IUINT16 port = 36001);
    ~ReliableASIOUDP();

    int initialize();
    int connect(std::string &server_ip, IUINT16 server_port);
    int start();
    int run();
    void spin();
    void spin_once();
    static int output(const char *buf, int len, struct IKCPCB *kcp, void *user);

    int send_udp_package(const char *data_ptr, int data_len);

    int send(uint8_t *data_ptr, int data_len);
    int receive(uint8_t *data_ptr, int data_size);
protected:
    // virtual 
private:
    IUINT32 conv_;
    // system status manager kcp
    std::shared_ptr<ikcpcb> syskcp_ptr_;
    std::shared_ptr<ikcpcb> datakcp_ptr_;
    // server ip and port
    std::string ip_;
    IUINT16 port_;

    // false: not run true:runing
    std::atomic_bool run_;
    // client connect to server status
    std::atomic_bool connect_status_;
    std::atomic_bool ready_;
    
    
    // normal or asio send data
    TxMode txmode_;

    std::shared_ptr<boost::asio::ip::udp::socket> socket_ptr_;
    std::shared_ptr<boost::asio::io_service> service_ptr_;
    boost::asio::ip::udp::endpoint endpoint_;

    std::array<uint8_t, KEthPacketMaxLength> tempBuffer_{};
    // ikcp update timer
    std::shared_ptr<Timer> timer_ptr_;
    // udp rx status handle
    std::shared_ptr<std::thread> receive_task_;

    void OnTimerCallback();
    void OnReceiveKcpCallback();
    void OnSendDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred);
    void OnReceiveDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred);


    asio_func_callback *recv_callback_func_ptr_;
    asio_func_callback *send_callback_func_ptr_;
};

#endif /* RELIABLE_ASIO_UDP_H */