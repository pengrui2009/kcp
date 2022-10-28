#include "udp_kcp.h"
#include "asio_udp.h"
#include "timestamp.h"

#include "base_frame.h"

#include "logger.h"

#include <atomic>
#include <cstdint>
#include <memory>


void UKcp::OnSendDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred)
{
    std::cout << "OnSendDataCallback bytes_transferred:" << bytes_transferred << std::endl;
}

int UKcp::HandleReceiveData(boost::asio::ip::udp::endpoint endpoint, std::size_t bytes_transferred)
{
    int ret = 0;

    std::string remote_ip = "";
    uint16_t remote_port = 0;
    std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>();

    LOG_INFO("receive data_len:%d", bytes_transferred);
    if (KCP_FRAME_SIZE >= bytes_transferred)
    {
        LOG_ERROR("receive data length is not enough, %d <= %d", bytes_transferred, KCP_FRAME_SIZE);
        return -1;
    }
    
    ret = frame_ptr->decode(&databuffer_[KCP_FRAME_SIZE], (bytes_transferred - KCP_FRAME_SIZE));
    if (ret)
    {
        LOG_ERROR("base frame decode failed, ret:%d", ret);
        return -1;
    }

    get_ipaddress(endpoint, remote_ip, remote_port);    

    ret = frame_ptr->set_host_ip(remote_ip);
    if (ret)
    {
        LOG_ERROR("base frame set_host_ip failed, ret:%d", ret);
        return -1;
    }

    ret = frame_ptr->set_host_port(remote_port);
    if (ret)
    {
        LOG_ERROR("base frame set_host_port failed, ret:%d", ret);
        return -1;
    }

    ret = frame_ptr->encode();
    if (ret)
    {
        LOG_ERROR("response base frame set_host_port failed, ret:%d", ret);
        return -1;
    }

    memcpy(&databuffer_[KCP_FRAME_SIZE], frame_ptr->buffer_.data(), frame_ptr->buffer_.size());
    ret = ikcp_input(system_kcp_ptr_.get(), reinterpret_cast<const char *>(databuffer_.data()), bytes_transferred);
    if (ret)
    {
        LOG_ERROR("response base frame set_host_port failed, ret:%d", ret);
        return -1;
    }

    return 0;
}

void UKcp::OnReceiveDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred)
{ 
    int ret = 0;
    
    boost::asio::ip::udp::endpoint endpoint = endpoint_;
    
    LOG_INFO("OnReceiveDataCallback %d", bytes_transferred);
    // Refill client endpoint info

    ret = HandleReceiveData(endpoint, bytes_transferred);
    if (ret)
    {
        LOG_ERROR("HandleReceiveData failed, ret:%d", ret);
    }
#if 0        
    // parse frame
    std::string remote_ip = "";
    uint16_t remote_port = 0;
    std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>();

    LOG_INFO("receive data_len:%d", bytes_transferred);
    if (KCP_FRAME_SIZE >= bytes_transferred)
    {
        LOG_ERROR("receive data length is not enough, %d <= %d", bytes_transferred, KCP_FRAME_SIZE);
        goto failed;
    }
    
    ret = frame_ptr->decode(&databuffer_[KCP_FRAME_SIZE], (bytes_transferred - KCP_FRAME_SIZE));
    if (ret)
    {
        LOG_ERROR("base frame decode failed, ret:%d", ret);
        goto failed;
    }

    get_ipaddress(endpoint, remote_ip, remote_port);    

    ret = frame_ptr->set_host_ip(remote_ip);
    if (ret)
    {
        LOG_ERROR("base frame set_host_ip failed, ret:%d", ret);
        goto failed;
    }

    ret = frame_ptr->set_host_port(remote_port);
    if (ret)
    {
        LOG_ERROR("base frame set_host_port failed, ret:%d", ret);
        goto failed;
    }

    switch (frame_ptr->get_frametype())
    {
    case FRAMETYPE_UNKNOWN:
    {
        break;
    }
    case FRAMETYPE_REQUEST_CONNECT:
    {
        // response
        std::shared_ptr<BaseFrame> response_frame_ptr = std::make_shared<BaseFrame>(remote_ip, remote_port, FRAMETYPE_CONNECTED, msg_count_++);
        LOG_INFO("response frame encode");
        ret = response_frame_ptr->encode();
        if (ret)
        {
            LOG_ERROR("response base frame set_host_port failed, ret:%d", ret);
            goto failed;
        }
        LOG_INFO("response frame send");
        ret = ikcp_send(system_kcp_ptr_.get(), reinterpret_cast<char *>(response_frame_ptr->buffer_.data()), response_frame_ptr->buffer_.size());
        if (ret)
        {
            LOG_ERROR("response base frame ikcp_send failed, ret:%d", ret);
            goto failed;
        }
        LOG_INFO("response frame send success");
        break;
    }
    case FRAMETYPE_CONNECTED:
    {
        LOG_INFO("receive FRAMETYPE_CONNECTED frame.");
        connect_status_.store(true, std::memory_order_release);
        break;
    }
    case FRAMETYPE_REQUEST_DISCONNECT:
    {
        // response
        std::shared_ptr<BaseFrame> response_frame_ptr = std::make_shared<BaseFrame>(local_ip_, local_port_, FRAMETYPE_DISCONNECTED, msg_count_++);

        ret = response_frame_ptr->encode();
        if (ret)
        {
            LOG_ERROR("response base frame set_host_port failed, ret:%d", ret);
            goto failed;
        }

        ret = ikcp_send(system_kcp_ptr_.get(), reinterpret_cast<char *>(response_frame_ptr->buffer_.data()), response_frame_ptr->buffer_.size());
        if (ret)
        {
            LOG_ERROR("response base frame ikcp_send failed, ret:%d", ret);
            goto failed;
        }
        break;
    }
    case FRAMETYPE_DISCONNECTED:
    {

        break;
    }    
    case FRAMETYPE_HEARTBEAT:
    {
        break;
    }
    case FRAMETYPE_MSGDATA:
    {
        ret = frame_ptr->encode();
        if (ret)
        {
            LOG_ERROR("base frame encode failed, ret:%d", ret);
            goto failed;
        }

        ret = ikcp_input(system_kcp_ptr_.get(), reinterpret_cast<const char *>(frame_ptr->buffer_.data()), frame_ptr->buffer_.size());
        if (ret)
        {
            LOG_ERROR("ikcp_input failed, ret:%d", ret);
            goto failed;
        }
        break;
    }
    default:
        break;
    }
failed:
#endif
    // TODO perfect
    databuffer_.fill(0U);
    socket_ptr_->async_receive_from(
        boost::asio::buffer(databuffer_, KEthPacketMaxLength), endpoint_,
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
    LOG_INFO("udp output data_len:%d", data_len);

    if (kcp_ptr == nullptr)
    {
        LOG_ERROR("kcp_ptr is nullptr");
        return -1;
    }

    if (user_ptr == nullptr)
    {
        LOG_ERROR("user_ptr is nullptr");
        return -1;
    }

    // udp send 
    ((UKcp*)user_ptr)->send_udp_package(data_ptr, data_len);

	return 0;
}


int UKcp::send_udp_package(const char *data_ptr, int data_len)
{
    int ret = 0;
    std::string dest_ip = "";
    uint16_t dest_port = 0;

    if (KCP_FRAME_SIZE >= data_len)
    {
        LOG_ERROR("send_udp_package failed, %d <= %d", data_len, KCP_FRAME_SIZE);
        return -1;
    }

    std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>();
    
    ret = frame_ptr->decode(reinterpret_cast<const uint8_t *>(&data_ptr[KCP_FRAME_SIZE]), (data_len-24));
    if (ret)
    {
        LOG_ERROR("base frame decode failed, ret:%d", ret);
        return -1;
    }

    ret = frame_ptr->get_host_ip(dest_ip);
    if (ret)
    {
        LOG_ERROR("base frame decode failed, ret:%d", ret);
        return -1;
    }

    ret = frame_ptr->get_host_port(dest_port);
    if (ret)
    {
        LOG_ERROR("base frame decode failed, ret:%d", ret);
        return -1;
    }

    // LOG_INFO("ip:%s port:%d", dest_ip.c_str(), dest_port);
    boost::asio::ip::udp::endpoint remote_endpoint(
        boost::asio::ip::address_v4::from_string(dest_ip), dest_port);

    switch (txmode_)
    {
    case NORMAL_MODE:
        {
            size_t result = -1;
            result = socket_ptr_->send_to(boost::asio::buffer(data_ptr, data_len), remote_endpoint);
            if (result != data_len)
            {
                LOG_ERROR("udp send_to %s:port failed", dest_ip.c_str(), dest_port);
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

UKcp::UKcp(std::string ip, uint16_t port) : ASIOUdp(NORMAL_MODE, ip, port),
    system_conv_(0x66668888)
{
    connect_status_.store(false, std::memory_order_release);

    system_kcp_ptr_.reset(ikcp_create(system_conv_, (void *)this));

    system_kcp_ptr_->output = &UKcp::output;

    monitor_timer_ptr_ = std::make_shared<Timer>("kcp_update");

    // receive_task_ = std::make_shared<std::thread>(&ReliableASIOUDP::OnReceiveKcpCallback, this);
}

UKcp::~UKcp()
{
    ikcp_flush(system_kcp_ptr_.get());

    // ikcp_release(system_kcp_ptr_.get());
    
    system_kcp_ptr_.reset();
}

int UKcp::initialize()
{
    int ret = 0;

    ikcp_wndsize(system_kcp_ptr_.get(), 128, 128);

    // default mode
    ikcp_nodelay(system_kcp_ptr_.get(), 0, 10, 0, 0);

    // normal mode
    // ikcp_nodelay(system_kcp_ptr_.get(), 0, 10, 0, 1);
    
    // fast mode interval: 10ms disable, enable resend, flow control
    // ikcp_nodelay(system_kcp_ptr_.get(), 2, 10, 2, 1);

    system_kcp_ptr_->rx_minrto = 10;
	system_kcp_ptr_->fastresend = 1;

    ret = ASIOUdp::initialize();
    if (ret)
    {
        LOG_ERROR("asio udp initialize failed, ret:%d", ret);
        return -1;
    }
        
    run_.store(true, std::memory_order_release);

    monitor_timer_ptr_->start(10, std::bind(&UKcp::OnTimerCallback, this));

    return 0;
}

int UKcp::connect(std::string server_ip, uint16_t server_port)
{
    int ret = 0;
    uint8_t retry_count = 0;
        
    this->server_ip_ = server_ip;
    this->server_port_ = server_port;
    
    while((retry_count++ < 1) && !connect_status_.load())
    {
        std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>(
            server_ip, server_port, FRAMETYPE_REQUEST_CONNECT, msg_count_++);

        ret = frame_ptr->encode();
        if (ret < 0)
        {
            LOG_ERROR("base frame encode failed, ret:%d", ret);
            return -1;
        }

        // ret = send(frame_ptr->buffer_.data(), frame_ptr->buffer_.size(), server_ip, server_port);
        ret = ikcp_send(system_kcp_ptr_.get(), reinterpret_cast<char *>(frame_ptr->buffer_.data()), frame_ptr->buffer_.size());
        if (ret < 0)
        {
            LOG_ERROR("base frame ikcp_send failed, ret:%d", ret);
            return -1;
        }
        LOG_INFO("ikcp_send connect frame");
        ikcp_flush(system_kcp_ptr_.get());
        sleep(1);
    }

    if (!connect_status_.load())
    {
        LOG_ERROR("connect to server[%s] failed!", server_ip.c_str());
        return -1;
    }

    return 0;
}

int UKcp::start()
{
    int ret = 0;

    return 0;
}

int UKcp::spin()
{
    // ASIOUdp::run();
    return 0;
}

int UKcp::send_msg(uint8_t *data_ptr, size_t data_len, std::string &ip, uint16_t port)
{
    int ret = 0;
    std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>(
            ip, port, FRAMETYPE_MSGDATA, msg_count_++);
    
    // fill ip and port
    ret = frame_ptr->encode(data_ptr, data_len);
    if (ret < 0)
    {
        LOG_ERROR("base frame encode failed, ret:%d", ret);
        return -1;
    }

    ret = ikcp_send(system_kcp_ptr_.get(), reinterpret_cast<char *>(frame_ptr->buffer_.data()), frame_ptr->buffer_.size());
    
    return ret;
}

int UKcp::recv_msg(uint8_t *data_ptr, size_t data_size, std::string &ip, uint16_t port)
{
    int ret = 0;
    std::string dest_ip = "";
    uint16_t dest_port = 0;
    int data_len = 0;
    // uint32_t buffer_size = (system_kcp_ptr_->mtu * 3);
    // std::unique_ptr<uint8_t[]> buffer_ptr = std::make_unique<uint8_t[]>(buffer_size);
    std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>();

    ret = ikcp_recv(system_kcp_ptr_.get(), reinterpret_cast<char *>(data_ptr), data_size);
    if (ret <= 0)
    {
        LOG_ERROR("ikcp_recv failed, ret:%d", ret);
        return -1;
    }

    // if (KCP_FRAME_SIZE > data_len)
    // {
    //     LOG_ERROR("ikcp_recv data length is not enought %d < %d", data_len, KCP_FRAME_SIZE);
    //     return -1;
    // }

    data_len = ret;
    ret = frame_ptr->decode(data_ptr, data_len);
    if (ret)
    {
        LOG_ERROR("base frame decode failed, ret:%d", ret);
        return -1;
    }

    ret = frame_ptr->get_host_ip(dest_ip);
    if (ret)
    {
        LOG_ERROR("base frame get_host_ip failed, ret:%d", ret);
        return -1;
    }

    ret = frame_ptr->get_host_port(dest_port);
    if (ret)
    {
        LOG_ERROR("base frame get_host_port failed, ret:%d", ret);
        return -1;
    }

    ip = dest_ip;
    port = dest_port;
    // data_len = (data_len  > FRAME_MSGDATA_OFFSET) ? (data_len - FRAME_MSGDATA_OFFSET) : 0;
    // memcpy(data_ptr, &data_ptr[KCP_FRAME_SIZE], (data_len - KCP_FRAME_SIZE));
    
    return data_len;
}

int UKcp::service()
{
    int ret = 0;
    std::string dest_ip = "";
    uint16_t dest_port = 0;
    uint32_t buffer_size = (system_kcp_ptr_->mtu * 3);
    std::unique_ptr<uint8_t[]> buffer_ptr = std::make_unique<uint8_t[]>(buffer_size);

    ret = recv_msg(buffer_ptr.get(), buffer_size, dest_ip, dest_port);
    if (ret)
    {
        LOG_ERROR("recv_msg failed, ret:%d", ret);
        return -1;
    }

    int buffer_len = ret;
    std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>();
    ret = frame_ptr->decode(buffer_ptr.get(), buffer_len);
    if (ret)
    {
        LOG_ERROR("base frame decode failed, ret:%d", ret);
        return -1;
    }

    switch (frame_ptr->get_frametype())
    {
    case FRAMETYPE_UNKNOWN:
    {
        break;
    }
    case FRAMETYPE_REQUEST_CONNECT:
    {
        // response
        std::shared_ptr<BaseFrame> response_frame_ptr = std::make_shared<BaseFrame>(dest_ip, dest_port, FRAMETYPE_CONNECTED, msg_count_++);
        LOG_INFO("response frame encode");
        ret = response_frame_ptr->encode();
        if (ret)
        {
            LOG_ERROR("response base frame set_host_port failed, ret:%d", ret);
            return -1;
        }
        LOG_INFO("response frame send");
        ret = ikcp_send(system_kcp_ptr_.get(), reinterpret_cast<char *>(response_frame_ptr->buffer_.data()), response_frame_ptr->buffer_.size());
        if (ret)
        {
            LOG_ERROR("response base frame ikcp_send failed, ret:%d", ret);
            return -1;
        }
        LOG_INFO("response frame send success");
        break;
    }
    case FRAMETYPE_CONNECTED:
    {
        LOG_INFO("receive FRAMETYPE_CONNECTED frame.");
        connect_status_.store(true, std::memory_order_release);
        break;
    }
    case FRAMETYPE_REQUEST_DISCONNECT:
    {
        // response
        std::shared_ptr<BaseFrame> response_frame_ptr = std::make_shared<BaseFrame>(local_ip_, local_port_, FRAMETYPE_DISCONNECTED, msg_count_++);

        ret = response_frame_ptr->encode();
        if (ret)
        {
            LOG_ERROR("response base frame set_host_port failed, ret:%d", ret);
            return -1;
        }

        ret = ikcp_send(system_kcp_ptr_.get(), reinterpret_cast<char *>(response_frame_ptr->buffer_.data()), response_frame_ptr->buffer_.size());
        if (ret)
        {
            LOG_ERROR("response base frame ikcp_send failed, ret:%d", ret);
            return -1;
        }
        break;
    }
    case FRAMETYPE_DISCONNECTED:
    {

        break;
    }    
    case FRAMETYPE_HEARTBEAT:
    {
        break;
    }
    case FRAMETYPE_MSGDATA:
    {
        ret = frame_ptr->encode();
        if (ret)
        {
            LOG_ERROR("base frame encode failed, ret:%d", ret);
            return -1;
        }

        ret = ikcp_input(system_kcp_ptr_.get(), reinterpret_cast<const char *>(frame_ptr->buffer_.data()), frame_ptr->buffer_.size());
        if (ret)
        {
            LOG_ERROR("ikcp_input failed, ret:%d", ret);
            return -1;
        }
        break;
    }
    default:
        break;
    }

    return 0;
}