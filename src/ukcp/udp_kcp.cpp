#include "udp_kcp.h"
#include "asio_udp.h"
#include <atomic>
#include <cstdint>
#include <memory>


void UKcp::OnSendDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred)
{
    std::cout << "OnSendDataCallback bytes_transferred:" << bytes_transferred << std::endl;
}

void UKcp::OnReceiveDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred)
{ 
    int ret = 0;
    // TODO Fill client endpoint info
    ret = ikcp_input(system_kcp_ptr_.get(), reinterpret_cast<const char *>(databuffer_.data()), bytes_transferred);
    if (ret)
    {

    }
    
    // parse frame 

    databuffer_.fill(0U);
    socket_ptr_->async_receive_from(
        boost::asio::buffer(databuffer_, KEthPacketMaxLength), endpoint_ptr_,
        boost::bind(&UKcp::OnReceiveDataCallback, this, &boost::asio::placeholders::error,
            &boost::asio::placeholders::bytes_transferred));    
}

void UKcp::OnTimerCallback()
{
    ikcp_update(system_kcp_ptr_.get(), Timestamp::MillSecond());

    if (ikcp_waitsnd(system_kcp_ptr_.get()) > 128)
    {

    }
    // send connection heartbeat message

}

int UKcp::output(const char *data_ptr, int data_len, struct IKCPCB *kcp_ptr, void *user_ptr)
{
    // udp send 
    ((UKcp*)user_ptr)->send_udp_package(data_ptr, data_len);

	return 0;
}


int UKcp::send_udp_package(const char *data_ptr, int data_len)
{
    int ret = 0;

    boost::asio::ip::udp::endpoint remote_endpoint(
        boost::asio::ip::address_v4::from_string(server_ip_), server_port_);

    switch (txmode_)
    {
    case NORMAL_MODE:
        {
            size_t result = -1;
            result = socket_ptr_->send_to(boost::asio::buffer(data_ptr, data_len), remote_endpoint);
            if (result != data_len)
            {
                ret = -1;
            }
        }
        break;
    case ASIO_MODE:
        {
            socket_ptr_->async_send_to(boost::asio::buffer(data_ptr, data_len), remote_endpoint,
                boost::bind(
                &UKcp::OnSendDataCallback, this, &boost::asio::placeholders::error,
                &boost::asio::placeholders::bytes_transferred));
        }
        break;
    default:
        ret = -1;
        break;
    }
    
    return ret;
}

UKcp::UKcp(std::string &ip, uint16_t port) : ASIOUdp(ASIO_MODE, port),
    system_conv_(0x66668888)
{
    connect_status_.store(false, std::memory_order_release);

    system_kcp_ptr_.reset(ikcp_create(system_conv_, (void *)this));

    system_conv_->output = &UKcp::output;

    monitor_timer_ptr_ = std::make_shared<Timer>("kcp_update");

    // receive_task_ = std::make_shared<std::thread>(&ReliableASIOUDP::OnReceiveKcpCallback, this);
}

UKcp::~UKcp()
{

    ikcp_flush(system_kcp_ptr_.get());

    ikcp_release(system_kcp_ptr_.get());
    
    system_kcp_ptr_.reset();
}

int UKcp::connect(std::string &server_ip, uint16_t server_port)
{
    int ret = 0;
    uint8_t retry_count = 0;
        
    this->server_ip_ = server_ip;
    this->server_port_ = server_port;
    
    while((retry_count++ <= 3) && !connect_status_.load())
    {
        std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>(
            local_ip_, local_port_, FRAMETYPE_REQUEST_CONNECT, msg_count_++);

        ret = frame_ptr->encode(nullptr, 0);
        if (ret < 0)
        {
            LOG_ERROR("base frame encode failed, ret:%d", ret);
            return -1;
        }

        ret = send(frame_ptr->buffer_.data(), frame_ptr->buffer_.size(), server_ip, server_port);
        if (ret < 0)
        {
            LOG_ERROR("base frame send failed, ret:%d", ret);
            return -1;
        }
    }

    return 0;
}

int UKcp::start()
{
    int ret = 0;

    ret = ASIOUdp::start();
    if (ret)
    {
        LOG_ERROR("asio udp start failed, ret:%d", ret);
        return -1;
    }

    ikcp_wndsize(system_kcp_ptr_.get(), 128, 128);

    socket_ptr_->async_receive_from(
        boost::asio::buffer(databuffer_, KEthPacketMaxLength), endpoint_ptr_,
        boost::bind(&UKcp::OnReceiveDataCallback, this, &boost::asio::placeholders::error,
            &boost::asio::placeholders::bytes_transferred));
        
    run_.store(true, std::memory_order_release);

    // default mode
    // ikcp_nodelay(datakcp_ptr_.get(), 0, 10, 0, 0);

    // normal mode
    // ikcp_nodelay(datakcp_ptr_.get(), 0, 10, 0, 1);
    
    // fast mode interval: 10ms disable, enable resend, flow control
    ikcp_nodelay(system_kcp_ptr_.get(), 2, 10, 2, 1);
    system_kcp_ptr_->rx_minrto = 10;
	system_kcp_ptr_->fastresend = 1;

    monitor_timer_ptr_->start(10, std::bind(&UKcp::OnTimerCallback, this));

    return 0;
}

int UKcp::spin()
{
    ASIOUdp::run();
}

int UKcp::send_msg(uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;
    
    ret = ikcp_send(system_kcp_ptr_.get(), reinterpret_cast<char *>(data_ptr), data_len);
    
    return ret;
}

int UKcp::recv_msg(uint8_t *data_ptr, size_t data_size)
{
    int ret = 0;

    ret = ikcp_recv(system_kcp_ptr_.get(), reinterpret_cast<char *>(data_ptr), data_size);
    if (ret <= 0)
    {
        return -1;
    }

    return ret;
}
