#ifndef UDP_KCP_H
#define UDP_KCP_H

#include "ikcp.h"
#include "asio_udp.h"
#include "connection.h"
#include "timer.h"
#include "typedef.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <cstdint>
#include <functional>
#include <unordered_map>

/*
 kcp_conv <= 100(0x64), for cmd use
 kcp_conv > 100, for kcp conv
 */

// typedef IUINT32 kcp_conv_t;

constexpr size_t DEFAULT_MTU_SIZE = 1400;
constexpr size_t DEFAULT_MAX_CLIENT_SIZE = 1400;
constexpr int MIN_CONV_VALUE = 1000;

class KcpServer;
class Packet
{
public:
    Packet();
    ~Packet();
protected:
    
private:

};

class KcpServer : public ASIOUdp
{
public:
    KcpServer(std::string ip, uint16_t port);

    ~KcpServer();
    
    int initialize();
    
    int run();

    int set_mtu(int mtu_size);

    void set_event_callback(const std::function<event_callback_func>& func);

    int send_udp_packet(const asio_endpoint_t &dest, const uint8_t *data_ptr, size_t data_len);

    int send_udp_packet(struct IKCPCB *kcp_ptr, const uint8_t *data_ptr, size_t data_len);

    int send_kcp_packet(const kcp_conv_t conv, const uint8_t *data_ptr, size_t data_len);

    static int output(const char *buf, int len, struct IKCPCB *kcp, void *user);
protected:
    int update(uint32_t timestamp);

    void OnSendDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred) override;
    
    void OnReceiveDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred) override;
private:
    std::atomic_bool run_;

    std::mutex connections_mutex_;
    // signature key
    std::string sign_key_;

    std::unordered_map<kcp_conv_t, std::shared_ptr<Connection>> connections_;
    
    // packet mtu size
    int mtu_size_;
    // max clients
    size_t max_clients_size_;

    boost::asio::deadline_timer kcp_timer_;

    std::shared_ptr<RandomConv> random_conv_ptr_;

    std::function<event_callback_func> event_callback_;


    int handle_disconnect_frame(const asio_endpoint_t &dest, kcp_conv_t conv);

    int handle_connect_frame(const asio_endpoint_t &dest, kcp_conv_t &conv);

    int handle_udp_packet(asio_endpoint_t dest, uint8_t *data_ptr, size_t data_len);
    
    int handle_kcp_packet(asio_endpoint_t dest, uint8_t *data_ptr, size_t data_len);

    void handle_kcp_time();

    int do_kcp_timer_handle();

    int do_receive_handle(asio_endpoint_t dest, std::size_t bytes_transferred);

    int get_packet_conv(const uint8_t *data_ptr, size_t data_len, kcp_conv_t &conv);

    int get_new_conv(kcp_conv_t &conv) const;
};

#endif /* UDP_KCP_H */