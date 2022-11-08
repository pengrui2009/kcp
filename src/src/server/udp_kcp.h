#ifndef UDP_KCP_H
#define UDP_KCP_H

#include "ikcp.h"
#include "asio_udp.h"
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
constexpr size_t MIN_CONV_VALUE = 1000;

class KcpServer;
class Packet
{
public:
    Packet();
    ~Packet();
protected:
    
private:

};

// TODO new reuse conv , but current not support
// random number
class Random
{
public:
	Random(int offset, int size) : offset_(offset)
    {
		this->size_ = 0;
		seeds_.resize(size);
	}

	int random() {
		int x, i;
		if (seeds_.size() == 0) 
        {
            return 0;
        }

		if (size_ == 0) 
        { 
			for (i = 0; i < (int)seeds_.size(); i++) 
            {
				seeds_[i] = offset_ + i;
			}
			size_ = (int)seeds_.size();
		}
		i = rand() % size_;
		x = seeds_[i];
		seeds_[i] = seeds_[--size_];
		return x;
	}

protected:
	int offset_;
    int size_;
	std::vector<int> seeds_;
};

class Connection {
public:
    Connection(const kcp_conv_t conv, asio_endpoint_t endpoint, int mtu_size, KcpServer *server);

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

    static int kcp_output(const char *buf, int len, struct IKCPCB *kcp, void *user);

    int kcp_send(const uint8_t *data_ptr, int data_len);

    int kcp_receive(uint8_t *data_ptr, size_t data_len);

    void kcp_update(uint32_t millsec_time);
protected:

private:
    std::atomic_bool stop_;

    kcp_conv_t conv_;
    
    asio_endpoint_t endpoint_;
    // packet mtu size
    int mtu_size_;
    
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

    std::unordered_map<kcp_conv_t, std::shared_ptr<Connection>> connections_;
    
    // packet mtu size
    int mtu_size_;
    // max clients
    int max_clients_size_;

    boost::asio::deadline_timer kcp_timer_;

    std::shared_ptr<Random> random_conv_ptr_;

    std::function<event_callback_func> event_callback_;


    int handle_disconnect_frame(const asio_endpoint_t &dest, kcp_conv_t conv);

    int handle_connect_frame(const asio_endpoint_t &dest, kcp_conv_t &conv);

    int handle_udp_packet(asio_endpoint_t dest, uint8_t *data_ptr, size_t data_len);
    
    int handle_kcp_packet(asio_endpoint_t dest, uint8_t *data_ptr, size_t data_len);

    void handle_kcp_time();

    int do_kcp_timer_handle();

    int do_receive_handle(asio_endpoint_t dest, std::size_t bytes_transferred);

    int get_packet_conv(const uint8_t *data_ptr, size_t data_len, kcp_conv_t &conv);

    kcp_conv_t get_new_conv(void) const;
};

#endif /* UDP_KCP_H */