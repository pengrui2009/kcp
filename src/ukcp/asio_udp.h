#ifndef ASIO_UDP_H
#define ASIO_UDP_H

#include <cstdint>
#include <iostream>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <memory>

constexpr size_t KEthPacketMaxLength = 4096;

enum TxMode {
    NORMAL_MODE = 0,
    ASIO_MODE = 1,
};

class ASIOUdp {
public:
    ASIOUdp(TxMode mode = ASIO_MODE, uint16_t port = 36001);
    ASIOUdp(TxMode mode = ASIO_MODE, std::string &ip, uint16_t port = 36001);
    ~ASIOUdp();
    int initialize();
    int start();
    int run();

    int send(uint8_t *data_ptr, size_t data_len, const std::string &ip, const uint16_t port);

    void get_ipaddress();
    void set_ipaddress();

    void set_host();
    void get_host();

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
    boost::asio::ip::udp::endpoint endpoint_ptr_;

    std::array<uint8_t, KEthPacketMaxLength> databuffer_{};

    virtual void OnSendDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred);
    virtual void OnReceiveDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred);
private:

};

#endif /* ASIO_UDP_H */