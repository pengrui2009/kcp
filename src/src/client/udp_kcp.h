#ifndef UDP_KCP_H
#define UDP_KCP_H

#include "ikcp.h"
#include "asio_udp.h"
#include "timer.h"
#include "typedef.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>


constexpr size_t DEFAULT_MTU_SIZE = 1400;

class KcpClient : public ASIOUdp {
public:
    KcpClient(std::string ip, uint16_t port);
    ~KcpClient();

    int initialize();

    int run();

    int connect(const std::string &server_ip, uint16_t server_port);

    int disconnect();

    int set_mtu(int mtu_size);

    void set_event_callback(const std::function<event_callback_func>& func);


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

    // signature key
    std::string sign_key_;
    
    // 0:default 1:initialized 2:connected 3: kcp inited 4: connecting 5:disconnect
    std::atomic_int run_state_;
    std::mutex connect_mutex_;
    std::condition_variable connect_cond_;

    kcp_conv_t conv_;
    int mtu_size_;
    std::shared_ptr<ikcpcb> kcp_ptr_;
    // event callback function
    std::function<event_callback_func> event_callback_;

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


#endif /* UDP_KCP_H */