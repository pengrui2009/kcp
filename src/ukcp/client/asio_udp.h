#ifndef ASIO_UDP_H
#define ASIO_UDP_H

#include <cstdint>
#include <iostream>
#include <thread>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <memory>

// TODO
constexpr size_t KEthPacketMaxLength = 4096;

typedef boost::asio::ip::udp::endpoint asio_endpoint_t;

enum TxMode {
    NORMAL_MODE = 0,
    ASIO_MODE = 1,
};

class ASIOUdp {
public:
    ASIOUdp(TxMode mode = NORMAL_MODE, uint16_t port = 36001);
    ASIOUdp(TxMode mode, std::string &ip, uint16_t port);
    ~ASIOUdp();
    int initialize();
    int start();
    int run();

    
    void get_ipaddress(const asio_endpoint_t &endpoint, std::string &ip, uint16_t &port);
    void set_ipaddress(asio_endpoint_t &endpoint, const std::string &ip, const uint16_t &port);

    // void set_host();
    // void get_host();

protected:
    std::string local_ip_;
    uint16_t local_port_;

    std::string server_ip_;
    uint16_t server_port_;

    std::atomic_bool run_;
    // normal or asio send data
    TxMode txmode_;

    std::shared_ptr<boost::asio::ip::udp::socket> socket_ptr_;
    std::shared_ptr<boost::asio::io_service> service_ptr_;
    asio_endpoint_t endpoint_;

    std::array<uint8_t, KEthPacketMaxLength> databuffer_{};

    //std::shared_ptr<std::thread> run_task_;
    void do_async_receive_once();
    int send(const uint8_t *data_ptr, size_t data_len, const std::string &ip, const uint16_t port);
    int send(const uint8_t *data_ptr, size_t data_len, const asio_endpoint_t &dest);

    virtual void OnSendDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred);
    virtual void OnReceiveDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred);
private:

};

#endif /* ASIO_UDP_H */