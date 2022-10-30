#include "udp_kcp.h"
#include "asio_udp.h"
#include "timestamp.h"

#include "base_frame.h"

#include "logger.h"

#include <atomic>
#include <cstdint>
#include <memory>


int KcpClient::output(const char *data_ptr, int data_len, struct IKCPCB *kcp_ptr, void *user_ptr)
{
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
    return ((KcpClient*)user_ptr)->send_udp_packet(reinterpret_cast<const uint8_t *>(data_ptr), data_len);
}

void KcpClient::OnSendDataCallback(const boost::system::error_code &ec, 
    std::size_t bytes_transferred)
{
    // TODO
    // current not support udp asio mode
}
    
void KcpClient::OnReceiveDataCallback(const boost::system::error_code &ec, 
    std::size_t bytes_transferred)
{
    int ret = 0;
    boost::asio::ip::udp::endpoint endpoint = endpoint_;
    LOG_INFO("OnReceiveDataCallback data_len:%d", bytes_transferred);
    ret = do_receive_handle(endpoint, bytes_transferred);
    if (ret)
    {
        LOG_ERROR("HandleReceiveData failed, ret:%d", ret);
    }
    
    do_async_receive_once();
}

int KcpClient::handle_connect_response_frame(const asio_endpoint_t &dest, const uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;
    
    

    return 0;
}

int KcpClient::handle_kcp_packet(asio_endpoint_t dest, uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;
    kcp_conv_t packet_conv = 0;

    ret = get_packet_conv(data_ptr, data_len, packet_conv);
    if (ret)
    {
        LOG_ERROR("handle_connect_frame failed, ret:%d", ret);
        return -1;
    }

    if (packet_conv != conv_)
    {
        LOG_ERROR("invalid conv %d,%d", conv_, packet_conv);
        return -1;
    }
    // if (connections_.count(packet_conv) > 0)
    // {
    //     ret = connections_[packet_conv]->kcp_input(dest, data_ptr, data_len);
    //     if (ret)
    //     {
    //         LOG_ERROR("connection input failed, ret:%d", ret);
    //         return -1;
    //     }
    // }
    ret = ikcp_input(kcp_ptr_.get(), reinterpret_cast<const char *>(data_ptr), data_len);
    if (ret)
    {
        LOG_ERROR("ikcp_input failed, ret:%d", ret);
        return -1;
    }

    return 0;
}

int KcpClient::do_receive_handle(const asio_endpoint_t &dest, size_t bytes_transferred)
{
    int ret = 0;

    if (bytes_transferred < FRAME_SYNC_SIZE)
    {
        LOG_ERROR("bytes_transferred is not enough %d", bytes_transferred);
        return -1;
    }

    if ((SYNC_BYTE_0 == databuffer_[0]) && (SYNC_BYTE_2 == databuffer_[2]) &&
        (SYNC_BYTE_1 == databuffer_[1]) && (SYNC_BYTE_3 == databuffer_[3]))
    {
        // udp frame packet
        ret = handle_udp_packet(dest, databuffer_.data(), bytes_transferred);
        if (ret)
        {
            LOG_ERROR("handle_udp_packet failed, ret:%d", ret);
            return -1;
        }
    } else {
        // kcp frame packet
        ret = handle_kcp_packet(dest, databuffer_.data(), bytes_transferred);
        if (ret)
        {
            LOG_ERROR("handle_kcp_packet failed, ret:%d", ret);
            return -1;
        }
    }

    return 0;
}

int KcpClient::handle_udp_packet(asio_endpoint_t dest, uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;
    std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>();

    if (data_len < FRAME_HEADER_SIZE)
    {
        LOG_ERROR("udp frame length is not enought %d < %d", data_len, FRAME_HEADER_SIZE);
        return -1;
    }

    ret = frame_ptr->decode(data_ptr, data_len);
    if (ret)
    {
        LOG_ERROR("base frame decode failed, ret:%d", ret);
        return -1;
    }

    // ret = frame_ptr->get_host_ip(dest_ip);
    // if (ret)
    // {
    //     LOG_ERROR("base frame decode failed, ret:%d", ret);
    //     return -1;
    // }

    // ret = frame_ptr->get_host_port(dest_port);
    // if (ret)
    // {
    //     LOG_ERROR("base frame decode failed, ret:%d", ret);
    //     return -1;
    // }

    FrameType type = frame_ptr->get_frametype();
    switch(type)
    {
    case FRAMETYPE_REQUEST_CONNECT:
    {        
        break;
    }
    case FRAMETYPE_CONNECTED:
    {
        // ret = handle_connect_response_frame(dest);
        // if (ret)
        // {
        //     LOG_ERROR("handle_connect_frame failed, ret:%d", ret);
        //     return -1;
        // }
        LOG_INFO("FRAMETYPE_CONNECTED");
        if (frame_ptr->data_.size() != KCP_CONV_SIZE )
        {
            LOG_ERROR("invalid data length: %d != %d", frame_ptr->data_.size(), KCP_CONV_SIZE);
            return -1;
        }
        conv_ = *(reinterpret_cast<kcp_conv_t *>(frame_ptr->data_.data()));
        LOG_INFO("get conv:%d", conv_);
        run_state_.store(2, std::memory_order_release);

        connect_cond_.notify_one();
        
        break;
    }
    case FRAMETYPE_REQUEST_DISCONNECT:
        break;
    case FRAMETYPE_DISCONNECTED:
        break;
    }

    return 0;
}

int KcpClient::do_kcp_timer_handle()
{
    if (!run_.load())
    {
        return -1;
    }
    
    kcp_timer_.expires_from_now(boost::posix_time::milliseconds(10));
    kcp_timer_.async_wait(std::bind(&KcpClient::handle_kcp_time, this));

    return 0;
}

void KcpClient::handle_kcp_time()
{
    int ret = 0;

    uint32_t timestamp = 0x00;

    do_kcp_timer_handle();

    timestamp = Timestamp::iClock();

    ikcp_update(kcp_ptr_.get(), timestamp);

    // TODO max size
    size_t buffer_size = 10 * 1024;
    std::unique_ptr<uint8_t[]> buffer_ptr = std::make_unique<uint8_t[]>(buffer_size);
    ret = ikcp_recv(kcp_ptr_.get(), reinterpret_cast<char *>(buffer_size), buffer_size);
    if (ret <= 0)
    {
        //LOG_ERROR("ikcp_recv failed, ret:%d", ret);
        // return -1;
        return;
    } 

    LOG_INFO("data_len:%d %02X", ret, buffer_ptr[0]);
}

int KcpClient::get_packet_conv(const uint8_t *data_ptr, size_t data_len, kcp_conv_t &conv)
{
    if (data_len < KCP_CONV_SIZE)
    {
        LOG_ERROR("length is not enought %d < %d", data_len, KCP_CONV_SIZE);
        return -1;
    }

    conv = ikcp_getconv(reinterpret_cast<const void *>(data_ptr));
    return 0;
}


int KcpClient::send_udp_packet(const uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;
    asio_endpoint_t dest(boost::asio::ip::address_v4::from_string(server_ip_), server_port_); 

    if (run_state_.load() == 0)
    {
        LOG_ERROR("run_state:%d", run_state_.load());
        return -1;
    }

    ret = send(data_ptr, data_len, dest);
    if (ret)
    {    
        LOG_ERROR("frame send failed, ret:%d", ret);
        return -1;
    }

    return 0;
}

int KcpClient::send_kcp_packet(const uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;

    if (kcp_ptr_ == nullptr)
    {
        LOG_ERROR("kcp_ptr is nullptr");
        return -1;
    }

    if (run_state_.load() != 3)
    {
        LOG_ERROR("run_state:%d", run_state_.load());
        return -1;
    }

    ret = ikcp_send(kcp_ptr_.get(), reinterpret_cast<const char *>(data_ptr), data_len);
    if (ret)
    {
        LOG_ERROR("ikcp_send failed, ret:%d", ret);
        return -1;
    }

    return 0;
}

KcpClient::KcpClient(std::string ip, uint16_t port) : ASIOUdp(NORMAL_MODE, ip, port),
    local_ip_(ip),
    local_port_(port),
    kcp_timer_(*service_ptr_)
{
    conv_ = 0x00;
    run_state_.store(0, std::memory_order_release);
    kcp_ptr_.reset();
}

KcpClient::~KcpClient()
{

}

int KcpClient::initialize()
{
    int ret = 0;

    ret = ASIOUdp::initialize();
    if (ret)
    {
        LOG_ERROR("asio udp initialize failed, ret:%d", ret);
        return -1;
    }

    // do_kcp_timer_handle();

    run_.store(true, std::memory_order_release);
    run_state_.store(1, std::memory_order_release);

    return 0;
}

int KcpClient::connect(const std::string &server_ip, uint16_t server_port)
{
    int ret = 0;

    server_ip_ = server_ip;
    server_port_ = server_port;

    // make connect packet
    std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>(server_ip_, server_port_, 
        FRAMETYPE_REQUEST_CONNECT, 0);

    ret = frame_ptr->encode();
    if (ret)
    {
        LOG_ERROR("response connect packet encode failed, ret:%d", ret);
        return -1;
    }

    // send 
    ret = send_udp_packet(frame_ptr->buffer_.data(), frame_ptr->buffer_.size());
    if (ret)
    {
        LOG_ERROR("response connect packet send failed, ret:%d", ret);
        return -1;
    }

    // wait for response packet
    uint32_t timeout_ms = 1000;
    std::unique_lock<std::mutex> lock(connect_mutex_);
    auto status = connect_cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() -> bool {
        return ((this->run_.load() == false) || (this->run_state_.load() == 2));
    });

    if (!run_.load() || (status == false))
    {
        LOG_INFO("connect timeout %d", this->run_state_.load());
        return -1;
    }

    LOG_INFO("connect success conv:%d", conv_);

    

    return 0;
}

int KcpClient::run()
{
    int ret = 0;

    kcp_ptr_.reset(ikcp_create(conv_, (void *)this));
    kcp_ptr_->output = &KcpClient::output;

    ikcp_wndsize(kcp_ptr_.get(), 128, 128);

    // default mode
    // ikcp_nodelay(kcp_ptr_.get(), 0, 10, 0, 0);

    // normal mode
    // ikcp_nodelay(kcp_ptr_.get(), 0, 10, 0, 1);
    
    // fast mode interval: 10ms disable, enable resend, flow control
    ikcp_nodelay(kcp_ptr_.get(), 2, 10, 2, 1);
    kcp_ptr_->rx_minrto = 10;
    kcp_ptr_->fastresend = 1;

    run_state_.store(3, std::memory_order_release);

    ret = ASIOUdp::run();
    if (ret)
    {
        LOG_ERROR("asio udp run failed, ret:%d", ret);
        return -1;
    }

    do_kcp_timer_handle();

    usleep(100000);

    return 0;
}

// void UKcp::OnSendDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred)
// {
//     std::cout << "OnSendDataCallback bytes_transferred:" << bytes_transferred << std::endl;
// }

// int UKcp::HandleReceiveData(boost::asio::ip::udp::endpoint endpoint, std::size_t bytes_transferred)
// {
//     int ret = 0;

//     std::string remote_ip = "";
//     uint16_t remote_port = 0;
//     std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>();

//     LOG_INFO("receive data_len:%d", bytes_transferred);
//     if (KCP_FRAME_SIZE > bytes_transferred)
//     {
//         LOG_ERROR("receive data length is not enough, %d <= %d", bytes_transferred, KCP_FRAME_SIZE);
//         return -1;
//     }
    
//     if (bytes_transferred > KCP_FRAME_SIZE)
//     {
//         ret = frame_ptr->decode(&databuffer_[KCP_FRAME_SIZE], (bytes_transferred - KCP_FRAME_SIZE));
//         if (ret)
//         {
//             LOG_ERROR("base frame decode failed, ret:%d", ret);
//             return -1;
//         }

//         get_ipaddress(endpoint, remote_ip, remote_port);    

//         ret = frame_ptr->set_host_ip(remote_ip);
//         if (ret)
//         {
//             LOG_ERROR("base frame set_host_ip failed, ret:%d", ret);
//             return -1;
//         }

//         ret = frame_ptr->set_host_port(remote_port);
//         if (ret)
//         {
//             LOG_ERROR("base frame set_host_port failed, ret:%d", ret);
//             return -1;
//         }

//         ret = frame_ptr->encode();
//         if (ret)
//         {
//             LOG_ERROR("response base frame set_host_port failed, ret:%d", ret);
//             return -1;
//         }

//         memcpy(&databuffer_[KCP_FRAME_SIZE], frame_ptr->buffer_.data(), frame_ptr->buffer_.size());
//         ret = ikcp_input(system_kcp_ptr_.get(), reinterpret_cast<const char *>(databuffer_.data()), bytes_transferred);
//         if (ret)
//         {
//             LOG_ERROR("response base frame set_host_port failed, ret:%d", ret);
//             return -1;
//         }
//     } else {
//         ret = ikcp_input(system_kcp_ptr_.get(), reinterpret_cast<const char *>(databuffer_.data()), bytes_transferred);
//         if (ret)
//         {
//             LOG_ERROR("response base frame set_host_port failed, ret:%d", ret);
//             return -1;
//         }
//     }
    
//     return 0;
// }

// void UKcp::OnReceiveDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred)
// { 
//     int ret = 0;
   
//     boost::asio::ip::udp::endpoint endpoint = endpoint_;

//     ret = HandleReceiveData(endpoint, bytes_transferred);
//     if (ret)
//     {
//         LOG_ERROR("HandleReceiveData failed, ret:%d", ret);
//     }
    
//     do_async_receive_once();  
// #if 0      
//     LOG_INFO("OnReceiveDataCallback %d", bytes_transferred);
//     // Refill client endpoint info

//     ret = HandleReceiveData(endpoint, bytes_transferred);
//     if (ret)
//     {
//         LOG_ERROR("HandleReceiveData failed, ret:%d", ret);
//     }
       
//     // parse frame
//     std::string remote_ip = "";
//     uint16_t remote_port = 0;
//     std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>();

//     LOG_INFO("receive data_len:%d", bytes_transferred);
//     if (KCP_FRAME_SIZE >= bytes_transferred)
//     {
//         LOG_ERROR("receive data length is not enough, %d <= %d", bytes_transferred, KCP_FRAME_SIZE);
//         goto failed;
//     }
    
//     ret = frame_ptr->decode(&databuffer_[KCP_FRAME_SIZE], (bytes_transferred - KCP_FRAME_SIZE));
//     if (ret)
//     {
//         LOG_ERROR("base frame decode failed, ret:%d", ret);
//         goto failed;
//     }

//     get_ipaddress(endpoint, remote_ip, remote_port);    

//     ret = frame_ptr->set_host_ip(remote_ip);
//     if (ret)
//     {
//         LOG_ERROR("base frame set_host_ip failed, ret:%d", ret);
//         goto failed;
//     }

//     ret = frame_ptr->set_host_port(remote_port);
//     if (ret)
//     {
//         LOG_ERROR("base frame set_host_port failed, ret:%d", ret);
//         goto failed;
//     }

//     switch (frame_ptr->get_frametype())
//     {
//     case FRAMETYPE_UNKNOWN:
//     {
//         break;
//     }
//     case FRAMETYPE_REQUEST_CONNECT:
//     {
//         // response
//         std::shared_ptr<BaseFrame> response_frame_ptr = std::make_shared<BaseFrame>(remote_ip, remote_port, FRAMETYPE_CONNECTED, msg_count_++);
//         LOG_INFO("response frame encode");
//         ret = response_frame_ptr->encode();
//         if (ret)
//         {
//             LOG_ERROR("response base frame set_host_port failed, ret:%d", ret);
//             goto failed;
//         }
//         LOG_INFO("response frame send");
//         ret = ikcp_send(system_kcp_ptr_.get(), reinterpret_cast<char *>(response_frame_ptr->buffer_.data()), response_frame_ptr->buffer_.size());
//         if (ret)
//         {
//             LOG_ERROR("response base frame ikcp_send failed, ret:%d", ret);
//             goto failed;
//         }
//         LOG_INFO("response frame send success");
//         break;
//     }
//     case FRAMETYPE_CONNECTED:
//     {
//         LOG_INFO("receive FRAMETYPE_CONNECTED frame.");
//         connect_status_.store(true, std::memory_order_release);
//         break;
//     }
//     case FRAMETYPE_REQUEST_DISCONNECT:
//     {
//         // response
//         std::shared_ptr<BaseFrame> response_frame_ptr = std::make_shared<BaseFrame>(local_ip_, local_port_, FRAMETYPE_DISCONNECTED, msg_count_++);

//         ret = response_frame_ptr->encode();
//         if (ret)
//         {
//             LOG_ERROR("response base frame set_host_port failed, ret:%d", ret);
//             goto failed;
//         }

//         ret = ikcp_send(system_kcp_ptr_.get(), reinterpret_cast<char *>(response_frame_ptr->buffer_.data()), response_frame_ptr->buffer_.size());
//         if (ret)
//         {
//             LOG_ERROR("response base frame ikcp_send failed, ret:%d", ret);
//             goto failed;
//         }
//         break;
//     }
//     case FRAMETYPE_DISCONNECTED:
//     {

//         break;
//     }    
//     case FRAMETYPE_HEARTBEAT:
//     {
//         break;
//     }
//     case FRAMETYPE_MSGDATA:
//     {
//         ret = frame_ptr->encode();
//         if (ret)
//         {
//             LOG_ERROR("base frame encode failed, ret:%d", ret);
//             goto failed;
//         }

//         ret = ikcp_input(system_kcp_ptr_.get(), reinterpret_cast<const char *>(frame_ptr->buffer_.data()), frame_ptr->buffer_.size());
//         if (ret)
//         {
//             LOG_ERROR("ikcp_input failed, ret:%d", ret);
//             goto failed;
//         }
//         break;
//     }
//     default:
//         break;
//     }
// failed:
// #endif
//     // std::string remote_ip = "";
//     // uint16_t remote_port = 0;

//     // // get_ipaddress(endpoint, remote_ip, remote_port);

//     // // LOG_INFO("On receive callback %s:%d", remote_ip.c_str(), remote_port);
//     // ret = ikcp_input(system_kcp_ptr_.get(), reinterpret_cast<const char *>(databuffer_.data()), bytes_transferred);
//     // if (ret)
//     // {
//     //     LOG_ERROR("ikcp_input failed, ret:%d", ret);
//     //     // goto failed;
//     // }

//     // wait for receive once again
    
// }

// void UKcp::OnTimerCallback()
// {
//     ikcp_update(system_kcp_ptr_.get(), Timestamp::MillSecond());

//     if (ikcp_waitsnd(system_kcp_ptr_.get()) > 128)
//     {

//     }
//     // send connection heartbeat message

// }

// int UKcp::output(const char *data_ptr, int data_len, struct IKCPCB *kcp_ptr, void *user_ptr)
// {
//     LOG_INFO("udp output data_len:%d", data_len);

//     if (kcp_ptr == nullptr)
//     {
//         LOG_ERROR("kcp_ptr is nullptr");
//         return -1;
//     }

//     if (user_ptr == nullptr)
//     {
//         LOG_ERROR("user_ptr is nullptr");
//         return -1;
//     }

//     // LOG_INFO("output data_len:%d", data_len);
//     // udp send 
//     ((UKcp*)user_ptr)->send_udp_package(kcp_ptr, data_ptr, data_len);

// 	return 0;
// }


// int UKcp::send_udp_package(struct IKCPCB *kcp_ptr, const char *data_ptr, int data_len)
// {
//     int ret = 0;
//     std::string dest_ip = "";
//     uint16_t dest_port = 0;
//     kcp_conv_t kcp_conv = 0;

//     if (data_len < KCP_FRAME_SIZE)
//     {
//         LOG_ERROR("send_udp_package failed, %d <= %d", data_len, KCP_FRAME_SIZE);
//         return -1;
//     }

//     ret = get_packet_conv(data_ptr, data_len, kcp_conv);
//     if (ret)
//     {
//         LOG_ERROR("get_packet_conv failed, ret:%d", ret);
//         return -1;
//     }

//     if (endpoint_.count(kcp_conv) > 0)
//     {
//         ret = send(data_ptr, data_len, endpoint_[kcp_conv]);
//         if (ret)
//         {    
//             LOG_ERROR("frame send failed, ret:%d", ret);
//             return -1;
//         }
//         return 0;
//     } else {
//         LOG_ERROR("get_packet_conv failed, ret:%d", ret);
//         return -1;
//     }
// #if 0
//     if (data_len > KCP_FRAME_SIZE)
//     {        
//         std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>();
        
//         ret = frame_ptr->decode(reinterpret_cast<const uint8_t *>(&data_ptr[KCP_FRAME_SIZE]), (data_len-24));
//         if (ret)
//         {
//             LOG_ERROR("base frame decode failed, ret:%d", ret);
//             return -1;
//         }

//         ret = frame_ptr->get_host_ip(dest_ip);
//         if (ret)
//         {
//             LOG_ERROR("base frame decode failed, ret:%d", ret);
//             return -1;
//         }

//         ret = frame_ptr->get_host_port(dest_port);
//         if (ret)
//         {
//             LOG_ERROR("base frame decode failed, ret:%d", ret);
//             return -1;
//         }

//         // LOG_INFO("ip:%s port:%d", dest_ip.c_str(), dest_port);
//         boost::asio::ip::udp::endpoint remote_endpoint(
//             boost::asio::ip::address_v4::from_string(dest_ip), dest_port);
        
//         switch (txmode_)
//         {
//         case NORMAL_MODE:
//             {
//                 size_t result = -1;
//                 result = socket_ptr_->send_to(boost::asio::buffer(data_ptr, data_len), remote_endpoint);
//                 if (result != data_len)
//                 {
//                     LOG_ERROR("udp send_to %s:port failed", dest_ip.c_str(), dest_port);
//                     ret = -1;
//                 }
//             }
//             break;
//         case ASIO_MODE:
//             {
//                 socket_ptr_->async_send_to(boost::asio::buffer(data_ptr, data_len), remote_endpoint,
//                     boost::bind(
//                     &UKcp::OnSendDataCallback, this, &boost::asio::placeholders::error,
//                     &boost::asio::placeholders::bytes_transferred));
//             }
//             break;
//         default:
//             ret = -1;
//             break;
//         }
//     } else {
//         std::string remote_ip = "";
//         uint16_t remote_port = 0;
//         get_ipaddress(endpoint_, remote_ip, remote_port);
//         LOG_INFO("output addr:%s %d", remote_ip.c_str(), remote_port);

//         boost::asio::ip::udp::endpoint remote_endpoint(
//             boost::asio::ip::address_v4::from_string("127.0.0.1"), 37001);
//         switch (txmode_)
//         {
//         case NORMAL_MODE:
//             {
//                 size_t result = -1;
//                 result = socket_ptr_->send_to(boost::asio::buffer(data_ptr, data_len), remote_endpoint);
//                 if (result != data_len)
//                 {
//                     LOG_ERROR("udp send_to %s:port failed", dest_ip.c_str(), dest_port);
//                     ret = -1;
//                 }
//             }
//             break;
//         case ASIO_MODE:
//             {
//                 socket_ptr_->async_send_to(boost::asio::buffer(data_ptr, data_len), endpoint_,
//                     boost::bind(
//                     &UKcp::OnSendDataCallback, this, &boost::asio::placeholders::error,
//                     &boost::asio::placeholders::bytes_transferred));
//             }
//             break;
//         default:
//             ret = -1;
//             break;
//         }
//     }
//     return ret;
// #endif    
// }

// UKcp::UKcp(std::string ip, uint16_t port) : ASIOUdp(NORMAL_MODE, ip, port),
//     system_conv_(0x66668888)
// {
//     connect_status_.store(false, std::memory_order_release);

//     system_kcp_ptr_.reset(ikcp_create(system_conv_, (void *)this));

//     system_kcp_ptr_->output = &UKcp::output;

//     monitor_timer_ptr_ = std::make_shared<Timer>("kcp_update");

//     // receive_task_ = std::make_shared<std::thread>(&ReliableASIOUDP::OnReceiveKcpCallback, this);
// }

// UKcp::~UKcp()
// {
//     ikcp_flush(system_kcp_ptr_.get());

//     // ikcp_release(system_kcp_ptr_.get());
    
//     system_kcp_ptr_.reset();
// }

// int UKcp::initialize()
// {
//     int ret = 0;

//     ikcp_wndsize(system_kcp_ptr_.get(), 128, 128);

//     // default mode
//     ikcp_nodelay(system_kcp_ptr_.get(), 0, 10, 0, 0);

//     // normal mode
//     // ikcp_nodelay(system_kcp_ptr_.get(), 0, 10, 0, 1);
    
//     // fast mode interval: 10ms disable, enable resend, flow control
//     // ikcp_nodelay(system_kcp_ptr_.get(), 2, 10, 2, 1);

//     system_kcp_ptr_->rx_minrto = 10;
// 	system_kcp_ptr_->fastresend = 1;

//     ret = ASIOUdp::initialize();
//     if (ret)
//     {
//         LOG_ERROR("asio udp initialize failed, ret:%d", ret);
//         return -1;
//     }
        
//     run_.store(true, std::memory_order_release);

//     monitor_timer_ptr_->start(10, std::bind(&UKcp::OnTimerCallback, this));

//     return 0;
// }

// int UKcp::connect(std::string server_ip, uint16_t server_port)
// {
//     int ret = 0;
//     uint8_t retry_count = 0;
        
//     this->server_ip_ = server_ip;
//     this->server_port_ = server_port;
    
//     while((retry_count++ <= 1) && !connect_status_.load())
//     {
//         std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>(
//             server_ip, server_port, FRAMETYPE_REQUEST_CONNECT, msg_count_++);

//         ret = frame_ptr->encode();
//         if (ret < 0)
//         {
//             LOG_ERROR("base frame encode failed, ret:%d", ret);
//             return -1;
//         }

//         // ret = send(frame_ptr->buffer_.data(), frame_ptr->buffer_.size(), server_ip, server_port);
//         ret = ikcp_send(system_kcp_ptr_.get(), reinterpret_cast<char *>(frame_ptr->buffer_.data()), frame_ptr->buffer_.size());
//         if (ret < 0)
//         {
//             LOG_ERROR("base frame ikcp_send failed, ret:%d", ret);
//             return -1;
//         }
//         LOG_INFO("ikcp_send connect frame");
//         ikcp_flush(system_kcp_ptr_.get());
//         sleep(1);
//     }

//     if (!connect_status_.load())
//     {
//         LOG_ERROR("connect to server[%s] failed!", server_ip.c_str());
//         return -1;
//     }

//     return 0;
// }

// int UKcp::start()
// {
//     int ret = 0;

//     return 0;
// }

// int UKcp::spin()
// {
//     // ASIOUdp::run();
//     return 0;
// }

// int UKcp::send_msg(uint8_t *data_ptr, size_t data_len, std::string &ip, uint16_t port)
// {
//     int ret = 0;
//     std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>(
//             ip, port, FRAMETYPE_MSGDATA, msg_count_++);
    
//     // fill ip and port
//     ret = frame_ptr->encode(data_ptr, data_len);
//     if (ret < 0)
//     {
//         LOG_ERROR("base frame encode failed, ret:%d", ret);
//         return -1;
//     }

//     ret = ikcp_send(system_kcp_ptr_.get(), reinterpret_cast<char *>(frame_ptr->buffer_.data()), frame_ptr->buffer_.size());
    
//     return ret;
// }

// int UKcp::recv_msg(uint8_t *data_ptr, size_t data_size, std::string &ip, uint16_t port)
// {
//     int ret = 0;
//     std::string dest_ip = "";
//     uint16_t dest_port = 0;
//     int data_len = 0;
//     // uint32_t buffer_size = (system_kcp_ptr_->mtu * 3);
//     // std::unique_ptr<uint8_t[]> buffer_ptr = std::make_unique<uint8_t[]>(buffer_size);
//     std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>();

//     ret = ikcp_recv(system_kcp_ptr_.get(), reinterpret_cast<char *>(data_ptr), data_size);
//     if (ret <= 0)
//     {
//         // LOG_ERROR("ikcp_recv failed, ret:%d", ret);
//         return -1;
//     }

//     // if (KCP_FRAME_SIZE > data_len)
//     // {
//     //     LOG_ERROR("ikcp_recv data length is not enought %d < %d", data_len, KCP_FRAME_SIZE);
//     //     return -1;
//     // }

//     data_len = ret;
//     ret = frame_ptr->decode(data_ptr, data_len);
//     if (ret)
//     {
//         LOG_ERROR("base frame decode failed, ret:%d", ret);
//         return -1;
//     }

//     ret = frame_ptr->get_host_ip(dest_ip);
//     if (ret)
//     {
//         LOG_ERROR("base frame get_host_ip failed, ret:%d", ret);
//         return -1;
//     }

//     ret = frame_ptr->get_host_port(dest_port);
//     if (ret)
//     {
//         LOG_ERROR("base frame get_host_port failed, ret:%d", ret);
//         return -1;
//     }

//     ip = dest_ip;
//     port = dest_port;
//     // data_len = (data_len  > FRAME_MSGDATA_OFFSET) ? (data_len - FRAME_MSGDATA_OFFSET) : 0;
//     // memcpy(data_ptr, &data_ptr[KCP_FRAME_SIZE], (data_len - KCP_FRAME_SIZE));
    
//     return data_len;
// }

// int UKcp::service()
// {
//     int ret = 0;
//     std::string dest_ip = "";
//     uint16_t dest_port = 0;
//     uint32_t buffer_size = (system_kcp_ptr_->mtu * 3);
//     std::unique_ptr<uint8_t[]> buffer_ptr = std::make_unique<uint8_t[]>(buffer_size);

//     ret = recv_msg(buffer_ptr.get(), buffer_size, dest_ip, dest_port);
//     if (ret)
//     {
//         // LOG_ERROR("recv_msg failed, ret:%d", ret);
//         return 0;
//     }

//     int buffer_len = ret;
//     std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>();
//     ret = frame_ptr->decode(buffer_ptr.get(), buffer_len);
//     if (ret)
//     {
//         LOG_ERROR("base frame decode failed, ret:%d", ret);
//         return -1;
//     }

//     switch (frame_ptr->get_frametype())
//     {
//     case FRAMETYPE_UNKNOWN:
//     {
//         break;
//     }
//     case FRAMETYPE_REQUEST_CONNECT:
//     {
//         // response
//         std::shared_ptr<BaseFrame> response_frame_ptr = std::make_shared<BaseFrame>(dest_ip, dest_port, FRAMETYPE_CONNECTED, msg_count_++);
//         LOG_INFO("response frame encode");
//         ret = response_frame_ptr->encode();
//         if (ret)
//         {
//             LOG_ERROR("response base frame set_host_port failed, ret:%d", ret);
//             return -1;
//         }
//         LOG_INFO("response frame send");
//         ret = ikcp_send(system_kcp_ptr_.get(), reinterpret_cast<char *>(response_frame_ptr->buffer_.data()), response_frame_ptr->buffer_.size());
//         if (ret)
//         {
//             LOG_ERROR("response base frame ikcp_send failed, ret:%d", ret);
//             return -1;
//         }
//         LOG_INFO("response frame send success");
//         break;
//     }
//     case FRAMETYPE_CONNECTED:
//     {
//         LOG_INFO("receive FRAMETYPE_CONNECTED frame.");
//         connect_status_.store(true, std::memory_order_release);
//         break;
//     }
//     case FRAMETYPE_REQUEST_DISCONNECT:
//     {
//         // response
//         std::shared_ptr<BaseFrame> response_frame_ptr = std::make_shared<BaseFrame>(local_ip_, local_port_, FRAMETYPE_DISCONNECTED, msg_count_++);

//         ret = response_frame_ptr->encode();
//         if (ret)
//         {
//             LOG_ERROR("response base frame set_host_port failed, ret:%d", ret);
//             return -1;
//         }

//         ret = ikcp_send(system_kcp_ptr_.get(), reinterpret_cast<char *>(response_frame_ptr->buffer_.data()), response_frame_ptr->buffer_.size());
//         if (ret)
//         {
//             LOG_ERROR("response base frame ikcp_send failed, ret:%d", ret);
//             return -1;
//         }
//         break;
//     }
//     case FRAMETYPE_DISCONNECTED:
//     {

//         break;
//     }    
//     case FRAMETYPE_HEARTBEAT:
//     {
//         break;
//     }
//     case FRAMETYPE_MSGDATA:
//     {
//         ret = frame_ptr->encode();
//         if (ret)
//         {
//             LOG_ERROR("base frame encode failed, ret:%d", ret);
//             return -1;
//         }

//         ret = ikcp_input(system_kcp_ptr_.get(), reinterpret_cast<const char *>(frame_ptr->buffer_.data()), frame_ptr->buffer_.size());
//         if (ret)
//         {
//             LOG_ERROR("ikcp_input failed, ret:%d", ret);
//             return -1;
//         }
//         break;
//     }
//     default:
//         break;
//     }

//     return 0;
// }

// int UKcp::handle_request_connection_packet()
// {

// }

// int UKcp::get_packet_conv(uint8_t *data_ptr, size_t data_len, kcp_conv_t &conv)
// {
//     if (data_len < sizeof(kcp_conv_t))
//     {
//         LOG_ERROR("length is not enough, %d<%d", data_len, sizeof(kcp_conv_t));
//         return -1;
//     }

//     conv = ikcp_getconv(reinterpret_cast<void *>(data_ptr));

//     return 0;
// }

// kcp_conv_t UKcp::get_new_conv(void) const
// {
//     // todo using rand to get a conv. privent the attack from guess conv.
//     // and must bigger than 1000

//     // increase from 1001, must bigger than 1000
//     static uint32_t static_cur_conv = 1000;
//     static_cur_conv++;
//     return static_cur_conv;
// }