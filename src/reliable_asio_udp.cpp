#include "reliable_asio_udp.h"
#include "timestamp.h"
#include <memory>
#include <thread>

/**
 * @brief 
 * 
 * @param data_ptr 
 * @param data_len 
 * @param kcp_ptr 
 * @param user_ptr 
 * @return int 
 */
int ReliableASIOUDP::output(const char *data_ptr, int data_len, struct IKCPCB *kcp_ptr, void *user_ptr)
{
    // udp send 
    ((ReliableASIOUDP*)user_ptr)->send_udp_package(data_ptr, data_len);

	return 0;
}

int ReliableASIOUDP::send_udp_package(const char *data_ptr, int data_len)
{
    int ret = 0;

    boost::asio::ip::udp::endpoint remote_endpoint(
        boost::asio::ip::address_v4::from_string(ip_), port_);

    switch (txmode_)
    {
    case NORMAL_TX:
        {
            size_t result = -1;
            result = socket_ptr_->send_to(boost::asio::buffer(data_ptr, data_len), remote_endpoint);
            if (result != data_len)
            {
                ret = -1;
            }
        }
        break;
    case ASIO_TX:
        {
            socket_ptr_->async_send_to(boost::asio::buffer(data_ptr, data_len), remote_endpoint,
                boost::bind(
                &ReliableASIOUDP::OnSendDataCallback, this, &boost::asio::placeholders::error,
                &boost::asio::placeholders::bytes_transferred));
        }
        break;
    default:
        ret = -1;
        break;
    }
    
    return ret;
}

void ReliableASIOUDP::OnTimerCallback()
{
    ikcp_update(datakcp_ptr_.get(), Timestamp::MillSecond());

    if (ikcp_waitsnd(datakcp_ptr_.get()) > 128)
    {

    }
    // send connection heartbeat message

}

void ReliableASIOUDP::OnReceiveKcpCallback()
{
    int ret = 0;

    while(1)
    {
        //ret = raudp_ptr->receive(data_buffer.data(), data_buffer.size());
        if (ret > 0)
        {
            std::cout << "recv ret:" << ret << std::endl;
        }
    }
}

void ReliableASIOUDP::OnSendDataCallback(const boost::system::error_code &ec, 
    std::size_t bytes_transferred)
{

}

void ReliableASIOUDP::OnReceiveDataCallback(const boost::system::error_code &ec, 
    std::size_t bytes_transferred)
{
    int ret = 0;

    ret = ikcp_input(datakcp_ptr_.get(), reinterpret_cast<const char *>(tempBuffer_.data()), bytes_transferred);
    if (ret)
    {

    }

    tempBuffer_.fill(0U);
    socket_ptr_->async_receive_from(
        boost::asio::buffer(tempBuffer_, KEthPacketMaxLength), endpoint_ptr_,
        boost::bind(&ReliableASIOUDP::OnReceiveDataCallback, this, &boost::asio::placeholders::error,
            &boost::asio::placeholders::bytes_transferred));
}

ReliableASIOUDP::ReliableASIOUDP(IUINT32 conv, std::string &ip, IUINT16 port) : 
    conv_(conv)
{
    connect_status_.store(false, std::memory_order_release);

    datakcp_ptr_.reset(ikcp_create(conv, (void *)this));

    datakcp_ptr_->output = &ReliableASIOUDP::output;

    timer_ptr_ = std::make_shared<Timer>("kcp_update");

    receive_task_ = std::make_shared<std::thread>(&ReliableASIOUDP::OnReceiveKcpCallback, this);

    service_ptr_ = std::make_shared<boost::asio::io_service>();
    socket_ptr_ = std::make_shared<boost::asio::ip::udp::socket>(
        *service_ptr_.get(), boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port_));

}

ReliableASIOUDP::~ReliableASIOUDP()
{    
    service_ptr_->reset();
    socket_ptr_.reset();
    service_ptr_.reset();

    ikcp_flush(datakcp_ptr_.get());

    ikcp_release(datakcp_ptr_.get());
    
    datakcp_ptr_.reset();
}

int ReliableASIOUDP::initialize()
{

    ikcp_wndsize(datakcp_ptr_.get(), 128, 128);

    socket_ptr_->async_receive_from(
    boost::asio::buffer(tempBuffer_, KEthPacketMaxLength), endpoint_ptr_,
    boost::bind(&ReliableASIOUDP::OnReceiveDataCallback, this, &boost::asio::placeholders::error,
        &boost::asio::placeholders::bytes_transferred));
    
    
    run_.store(true, std::memory_order_release);

    // default mode
    // ikcp_nodelay(datakcp_ptr_.get(), 0, 10, 0, 0);

    // normal mode
    // ikcp_nodelay(datakcp_ptr_.get(), 0, 10, 0, 1);
    
    // fast mode interval: 10ms disable, enable resend, flow control
    ikcp_nodelay(datakcp_ptr_.get(), 2, 10, 2, 1);
    datakcp_ptr_->rx_minrto = 10;
	datakcp_ptr_->fastresend = 1;

    return 0;
}

int ReliableASIOUDP::connect(std::string &server_ip, IUINT16 server_port)
{
    int retry_count = 0;

    ip_ = server_ip;
    port_ = server_port;

    // current mode:quick send ,no resend
    ikcp_nodelay(datakcp_ptr_.get(), 1, 10, 0, 0);

    // 
    while(!connect_status_.load() && (retry_count++ <= 3))
    {
        // send connect data

        usleep(100000);
    }

    if (connect_status_.load())
    {
        return 0;
    } else {
        return -1;
    }

    return 0;
}

int ReliableASIOUDP::start()
{
    timer_ptr_->start(10, std::bind(&ReliableASIOUDP::OnTimerCallback, this));

    return 0;    
}

int ReliableASIOUDP::send(uint8_t *data_ptr, int data_len)
{
    int ret = 0;
    
    ret = ikcp_send(datakcp_ptr_.get(), reinterpret_cast<char *>(data_ptr), data_len);
    
    return ret;
}

int ReliableASIOUDP::receive(uint8_t *data_ptr, int data_size)
{
    int ret = 0;

    ret = ikcp_recv(datakcp_ptr_.get(), reinterpret_cast<char *>(data_ptr), data_size);
    std::cout << "ret:" << ret << std::endl;
    if (ret <= 0)
    {
        return -1;
    }

    return ret;
}

int ReliableASIOUDP::run()
{
    // ready_.store(true, std::memory_order_release);
    
    // service_ptr_->run_one();

    // period call this
    // ikcp_update(datakcp_ptr_.get(), Timestamp::MillSecond());
    int ret = ikcp_waitsnd(datakcp_ptr_.get());
    std::cout << "ikcp_waitsnd ret:" << ret << std::endl;
}

void ReliableASIOUDP::spin()
{
    service_ptr_->run();
}

void ReliableASIOUDP::spin_once()
{
    service_ptr_->run_one();
}