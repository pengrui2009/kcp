#ifndef UDP_KCP_H
#define UDP_KCP_H

#include "ikcp.h"

#include "timer.h"

#include "asio_udp.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

typedef IUINT32 kcp_conv_t;

class KcpClient : public ASIOUdp {
public:
    KcpClient(std::string ip, uint16_t port);
    ~KcpClient();

    int initialize();

    int run();

    int connect(const std::string &server_ip, uint16_t server_port);

    int send_udp_packet(const uint8_t *data_ptr, size_t data_len);

    int send_kcp_packet(const uint8_t *data_ptr, size_t data_len);

    static int output(const char *buf, int len, struct IKCPCB *kcp, void *user);
protected:
    void OnSendDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred) override;
    
    void OnReceiveDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred) override;
private:
    std::atomic_bool run_;
    std::string local_ip_;
    uint16_t local_port_;

    std::string server_ip_;
    uint16_t server_port_;

    // 0:default 1:initialized 2:connected 3: kcp inited 4: connecting 5:disconnect
    std::atomic_int run_state_;
    std::mutex connect_mutex_;
    std::condition_variable connect_cond_;

    kcp_conv_t conv_;
    std::shared_ptr<ikcpcb> kcp_ptr_;

    boost::asio::deadline_timer kcp_timer_;
    
    int do_kcp_timer_handle();

    void handle_kcp_time();

    int get_packet_conv(const uint8_t *data_ptr, size_t data_len, kcp_conv_t &conv);

    // int make_
    int handle_udp_packet(asio_endpoint_t dest, uint8_t *data_ptr, size_t data_len);

    int handle_kcp_packet(asio_endpoint_t dest, uint8_t *data_ptr, size_t data_len);

    int handle_connect_response_frame(const asio_endpoint_t &dest, const uint8_t *data_ptr, size_t data_len);

    int do_receive_handle(const asio_endpoint_t &endpoint, size_t bytes_transferred);
};

// class UKcp : public ASIOUdp {
// public:
//     UKcp(std::string ip, uint16_t port);
//     ~UKcp();
//     int connect(std::string server_ip, uint16_t port);
//     int initialize();
//     int start();
//     int spin();
//     int service();
//     int send_msg(uint8_t *data_ptr, size_t data_len, std::string &ip, uint16_t port);
    
//     int recv_msg(uint8_t *data_ptr, size_t data_len, std::string &ip, uint16_t port);
// protected:
//     int handle_request_connection_packet();

//     int send_udp_package(struct IKCPCB *kcp_ptr, const char *data_ptr, int data_len);
    
//     int HandleReceiveData(boost::asio::ip::udp::endpoint endpoint, std::size_t bytes_transferred);

//     void OnTimerCallback();

//     void OnSendDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred) override;
    
//     void OnReceiveDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred) override;
// private:
//     uint8_t msg_count_{0};
//     // system status manager kcp
//     IUINT32 system_conv_;
//     std::shared_ptr<ikcpcb> system_kcp_ptr_;
    
//     // ikcp update timer    
//     std::shared_ptr<Timer> monitor_timer_ptr_;

//     std::atomic_bool connect_status_;

//     std::unordered_map<kcp_conv_t, boost::asio::ip::udp::endpoint> endpoints_;

//     int get_packet_conv(uint8_t *data_ptr, size_t data_len, kcp_conv_t &conv);

//     static int output(const char *buf, int len, struct IKCPCB *kcp, void *user);

// };

#endif /* UDP_KCP_H */