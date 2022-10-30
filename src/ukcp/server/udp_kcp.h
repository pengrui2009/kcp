#ifndef UDP_KCP_H
#define UDP_KCP_H

#include "ikcp.h"

#include "timer.h"

#include "asio_udp.h"
#include <atomic>
#include <mutex>
#include <cstdint>
#include <unordered_map>

/*
 kcp_conv <= 100(0x64), for cmd use
 kcp_conv > 100, for kcp conv
 */

typedef IUINT32 kcp_conv_t;
typedef int (*kcp_output_func)(const char *buf, int len, struct IKCPCB *kcp, void *user);

class Packet
{
public:
    Packet();
    ~Packet();
protected:
    
private:

};

class Connection {
public:
    Connection(const kcp_conv_t conv, asio_endpoint_t endpoint, kcp_output_func output);

    ~Connection();

    // int kcp_input(uint8_t *data_ptr, size_t data_len)
    // {
    //     return 0;
    // }

    // int kcp_output()
    // {
    //     return 0;
    // }

    // int kcp_receive(uint8_t *data_ptr, size_t data_len)
    // {
    //     return 0;
    // }

    // int kcp_send(uint8_t *data_ptr, size_t data_len)
    // {
    //     return 0;
    // }

    kcp_conv_t get_kcpconv() const
    {
        return this->conv_;
    }

    asio_endpoint_t get_endpoint() const
    {
        return this->endpoint_;
    }

    ikcpcb* get_kcpcb()
    {
        return this->kcp_ptr_;
    }

    int kcp_input(const asio_endpoint_t& dest, const uint8_t* data_ptr, size_t data_len);

    int kcp_send(const uint8_t *data_ptr, int data_len);

    int kcp_receive();

    void kcp_update(uint32_t millsec_time);
protected:

private:
    std::atomic_bool stop_;

    kcp_conv_t conv_;
    
    asio_endpoint_t endpoint_;

    ikcpcb *kcp_ptr_;
    // std::shared_ptr<ikcpcb> kcp_ptr_;

    
    // static int output(const char *buf, int len, struct IKCPCB *kcp, void *user);
};

class KcpServer : public ASIOUdp
{
public:
    KcpServer(std::string ip, uint16_t port);

    ~KcpServer();
    
    int initialize();
    
    int run();
    
    int update(uint32_t timestamp);

    static int output(const char *buf, int len, struct IKCPCB *kcp, void *user);
protected:


    void OnSendDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred) override;
    
    void OnReceiveDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred) override;
private:
    std::atomic_bool run_;

    std::mutex connections_mutex_;
    std::unordered_map<kcp_conv_t, std::shared_ptr<Connection>> connections_;
    
    boost::asio::deadline_timer kcp_timer_;

    int handle_connect_frame(const asio_endpoint_t &dest);

    int handle_udp_packet(asio_endpoint_t dest, uint8_t *data_ptr, size_t data_len);
    
    int handle_kcp_packet(asio_endpoint_t dest, uint8_t *data_ptr, size_t data_len);

    void handle_kcp_time();

    int do_kcp_timer_handle();

    int do_receive_handle(asio_endpoint_t dest, std::size_t bytes_transferred);

    int send_udp_packet(const asio_endpoint_t &dest, const uint8_t *data_ptr, size_t data_len);

    int send_udp_packet(struct IKCPCB *kcp_ptr, const uint8_t *data_ptr, size_t data_len);

    int send_kcp_packet(const kcp_conv_t conv, const uint8_t *data_ptr, size_t data_len);

    int get_packet_conv(const uint8_t *data_ptr, size_t data_len, kcp_conv_t &conv);

    kcp_conv_t get_new_conv(void) const;
};
#if 0
class UKcp : public ASIOUdp {
public:
    UKcp(std::string ip, uint16_t port);
    ~UKcp();
    int connect(std::string server_ip, uint16_t port);
    int initialize();
    int start();
    int spin();
    int service();
    int send_msg(uint8_t *data_ptr, size_t data_len, std::string &ip, uint16_t port);
    
    int recv_msg(uint8_t *data_ptr, size_t data_len, std::string &ip, uint16_t port);
protected:
    int handle_request_connection_packet();

    int send_udp_packet(struct IKCPCB *kcp_ptr, const char *data_ptr, int data_len);
    
    int HandleReceiveData(boost::asio::ip::udp::endpoint endpoint, std::size_t bytes_transferred);

    void OnTimerCallback();

    void OnSendDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred) override;
    
    void OnReceiveDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred) override;
private:
    uint8_t msg_count_{0};
    // system status manager kcp
    IUINT32 system_conv_;
    std::shared_ptr<ikcpcb> system_kcp_ptr_;
    
    // ikcp update timer    
    std::shared_ptr<Timer> monitor_timer_ptr_;

    std::atomic_bool connect_status_;

    std::unordered_map<kcp_conv_t, std::shared_ptr<Connection>> connections_;

    int get_packet_conv(uint8_t *data_ptr, size_t data_len, kcp_conv_t &conv);

    static int output(const char *buf, int len, struct IKCPCB *kcp, void *user);

};
#endif

#endif /* UDP_KCP_H */