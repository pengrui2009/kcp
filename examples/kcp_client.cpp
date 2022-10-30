#include "ikcp.h"

#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <sys/time.h>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <thread>

constexpr size_t KEthPacketMaxLength = 4096;
typedef boost::asio::ip::udp::endpoint asio_endpoint_t;

typedef void(receive_callback_func)(const asio_endpoint_t&, uint8_t *, size_t data_len);

class asioudp {
public:
    enum SendMethod {
        NORMAL_SEND = 0,
        ASIO_SEND = 1,
    };

    asioudp(uint16_t port = 36001);
    ~asioudp();
    void run();

    void set_receive_callback_func(const std::function<receive_callback_func> &func);
    int send(SendMethod method, const uint8_t *data_ptr, size_t data_len, const std::string &ip, const uint16_t port);

protected:
    uint16_t port_;
    std::atomic_bool run_;
    std::atomic_bool ready_;

    std::shared_ptr<boost::asio::ip::udp::socket> socket_ptr_;
    std::shared_ptr<boost::asio::io_service> service_ptr_;
    asio_endpoint_t endpoint_;

    std::function<receive_callback_func> receive_callback_;

    std::array<uint8_t, KEthPacketMaxLength> tempBuffer_{};

    void OnSendDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred);
    void OnReceiveDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred);

};

asioudp::asioudp(uint16_t port) :port_(port)
{
    service_ptr_ = std::make_shared<boost::asio::io_service>();
    socket_ptr_ = std::make_shared<boost::asio::ip::udp::socket>(
        *service_ptr_.get(), boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port_));
    socket_ptr_->async_receive_from(
        boost::asio::buffer(tempBuffer_, KEthPacketMaxLength), endpoint_,
        boost::bind(
            &asioudp::OnReceiveDataCallback, this, &boost::asio::placeholders::error,
            &boost::asio::placeholders::bytes_transferred));
    
    run_.store(true, std::memory_order_release);
}

asioudp::~asioudp()
{
    service_ptr_->reset();
    socket_ptr_.reset();
    service_ptr_.reset();
}

void asioudp::run()
{
    ready_.store(true, std::memory_order_release);
    service_ptr_->run();
}

int asioudp::send(SendMethod method, const uint8_t *data_ptr, size_t data_len, const std::string &ip, const uint16_t port)
{
    int ret = 0;

    boost::asio::ip::udp::endpoint remote_endpoint(
        boost::asio::ip::address_v4::from_string(ip), port);

    switch (method)
    {
    case NORMAL_SEND:
        {
            size_t result = -1;
            result = socket_ptr_->send_to(boost::asio::buffer(data_ptr, data_len), remote_endpoint);
            if (result != data_len)
            {
                ret = -1;
            }
        }
        break;
    case ASIO_SEND:
        {
            socket_ptr_->async_send_to(boost::asio::buffer(data_ptr, data_len), remote_endpoint,
                boost::bind(
                &asioudp::OnSendDataCallback, this, &boost::asio::placeholders::error,
                &boost::asio::placeholders::bytes_transferred));
        }
        break;
    default:
        ret = -1;
        break;
    }
    
    return ret;
}

void asioudp::set_receive_callback_func(const std::function<receive_callback_func> &func)
{
    this->receive_callback_ = func;
}

void asioudp::OnSendDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred)
{
    std::cout << "OnSendDataCallback data_len:" << bytes_transferred << std::endl;
}

void asioudp::OnReceiveDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred) 
{
    if (!run_.load()) {
        return;
    }

    if (ec.value() != 0) {
        throw boost::system::system_error(ec);
    }
    if (ready_.load()) {
        std::cout << "OnReceiveDataCallback bytes_transferred:" << bytes_transferred << std::endl;
        
        receive_callback_(endpoint_, tempBuffer_.data(), bytes_transferred);
    }
    tempBuffer_.fill(0U);
    if (socket_ptr_ != nullptr) {
    socket_ptr_->async_receive_from(
        boost::asio::buffer(tempBuffer_, KEthPacketMaxLength), endpoint_,
        boost::bind(
            &asioudp::OnReceiveDataCallback, this, &boost::asio::placeholders::error,
            &boost::asio::placeholders::bytes_transferred));
    }

}


/* get system time */
static inline void itimeofday(long *sec, long *usec)
{
	#if defined(__unix)
	struct timeval time;
	gettimeofday(&time, NULL);
	if (sec) *sec = time.tv_sec;
	if (usec) *usec = time.tv_usec;
	#else
	static long mode = 0, addsec = 0;
	BOOL retval;
	static IINT64 freq = 1;
	IINT64 qpc;
	if (mode == 0) {
		retval = QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		freq = (freq == 0)? 1 : freq;
		retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
		addsec = (long)time(NULL);
		addsec = addsec - (long)((qpc / freq) & 0x7fffffff);
		mode = 1;
	}
	retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
	retval = retval * 2;
	if (sec) *sec = (long)(qpc / freq) + addsec;
	if (usec) *usec = (long)((qpc % freq) * 1000000 / freq);
	#endif
}

/* get clock in millisecond 64 */
static inline IINT64 iclock64(void)
{
	long s, u;
	IINT64 value;
	itimeofday(&s, &u);
	value = ((IINT64)s) * 1000 + (u / 1000);
	return value;
}

static inline IUINT32 iclock()
{
	return (IUINT32)(iclock64() & 0xfffffffful);
}

class KcpClient {
public:
    KcpClient()
    {
        udp_ptr_ = std::make_shared<asioudp>(36001);

        kcp_ptr_.reset(ikcp_create(0x6688, (void *)this));

        udp_ptr_->set_receive_callback_func(std::bind(&KcpClient::handle_receive_callback, this, std::placeholders::_1, 
            std::placeholders::_2, std::placeholders::_3));

        kcp_ptr_->output = &KcpClient::output;
    }

    ~KcpClient()
    {
        
    }

    int initialize()
    {
        int ret  =0;

        ikcp_wndsize(kcp_ptr_.get(), 128, 128);
        
        // 默认模式
		// ikcp_nodelay(kcp_ptr_.get(), 0, 10, 0, 0);

        // 普通模式，关闭流控等
		// ikcp_nodelay(kcp_ptr_.get(), 0, 10, 0, 1);

        // 启动快速模式
		// 第二个参数 nodelay-启用以后若干常规加速将启动
		// 第三个参数 interval为内部处理时钟，默认设置为 10ms
		// 第四个参数 resend为快速重传指标，设置为2
		// 第五个参数 为是否禁用常规流控，这里禁止
		ikcp_nodelay(kcp_ptr_.get(), 2, 10, 2, 1);
		kcp_ptr_->rx_minrto = 10;
		kcp_ptr_->fastresend = 1;

        std::thread([this]()
        {
            int ret = 0;
            while(1)
            {
                
                IUINT32 current = iclock();

                ikcp_update(this->kcp_ptr_.get(), current);
                // std::cout << "current:" << current << std::endl;
                usleep(5000);

                ret = ikcp_recv(kcp_ptr_.get(), reinterpret_cast<char *>(buffer_.data()), buffer_size_);

            }
            
        }).detach();

        return 0;
    }

    int run()
    {
        int ret = 0;
        udp_ptr_->run();
        
        return 0;
    }

    int send_udp_packet(const uint8_t *data_ptr, size_t data_len)
    {
        int ret = 0;
        std::string server_ip = "127.0.0.1";
        uint16_t server_port = 38001;

        ret = udp_ptr_->send(asioudp::NORMAL_SEND, data_ptr, data_len, server_ip, server_port);
        if (ret)
        {
            return -1;
        }

        return 0;
    }

    int send_kcp_packet(const char *buffer_ptr, int buffer_len)
    {
        int ret = 0;

        ret = ikcp_send(kcp_ptr_.get(), buffer_ptr, buffer_len);
        if (ret)
        {
            std::cout << "ikcp_send ret:" << ret << std::endl;
            return -1;
        }

        // ikcp_flush(kcp_ptr_.get());

        return 0;
    }

    void handle_receive_callback(const asio_endpoint_t &dest, uint8_t *data_ptr, uint16_t data_len)
    {
        std::cout << "handle_receive_callback data_len:" << data_len << std::endl;
    }

    static int output(const char *data_ptr, int data_len, struct IKCPCB *kcp, void *user_ptr)
    {
        std::cout << iclock() << " output success." << std::endl;
        return ((KcpClient *)user_ptr)->send_udp_packet(reinterpret_cast<const uint8_t *>(data_ptr), data_len);
    }

private:
    std::shared_ptr<asioudp> udp_ptr_;

    std::shared_ptr<ikcpcb> kcp_ptr_;
    
    std::array<uint8_t, 1024> buffer_;
    size_t buffer_size_{1024};
};

int main()
{
    int ret = 0;
    std::shared_ptr<KcpClient> client_ptr = std::make_shared<KcpClient>();

    ret = client_ptr->initialize();
    if (ret)
    {
        std::cout << "initialize failed." << std::endl;
        return -1;
    }

    std::thread([client_ptr](){
        while(1)
        {
            static uint8_t msg_count = 0;
            char buffer[] = {0x00, 0x01, 0x02, 0x03};
            buffer[0] = msg_count++;
            client_ptr->send_kcp_packet(buffer, sizeof(buffer));
            std::cout << "send_kcp_packet success." << std::endl;
            // client_ptr->send_udp_packet(reinterpret_cast<const uint8_t *>(buffer), sizeof(buffer));
            sleep(100);
        }
    }).detach();

    client_ptr->run();

    return 0;
}