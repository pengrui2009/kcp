#ifndef UDP_KCP_H
#define UDP_KCP_H

#include "ikcp.h"

#include "timer.h"

#include "asio_udp.h"
#include <atomic>
#include <cstdint>

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
    int send_udp_package(const char *data_ptr, int data_len);
    
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

    static int output(const char *buf, int len, struct IKCPCB *kcp, void *user);

};

#endif /* UDP_KCP_H */