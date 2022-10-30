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
    {
        LOG_INFO("FRAMETYPE_DISCONNECTED");
        if (frame_ptr->data_.size() != KCP_CONV_SIZE )
        {
            LOG_ERROR("invalid data length: %d != %d", frame_ptr->data_.size(), KCP_CONV_SIZE);
            return -1;
        }
        conv_ = *(reinterpret_cast<kcp_conv_t *>(frame_ptr->data_.data()));
        LOG_INFO("get conv:%d", conv_);
        run_state_.store(5, std::memory_order_release);

        connect_cond_.notify_one();
        
        break;
    }
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

    // TODO max buffer size
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

int KcpClient::set_mtu(int mtu_size)
{
    int ret = 0;

    if (mtu_size < 50 || mtu_size < (int)KCP_FRAME_SIZE)
    {
		return -1;
    }

    mtu_size_ = mtu_size;

    return 0;
}

void KcpClient::set_event_callback(const std::function<event_callback_func>& func)
{
    this->event_callback_ = func;
}


KcpClient::KcpClient(std::string ip, uint16_t port) : ASIOUdp(NORMAL_MODE, ip, port),
    local_ip_(ip),
    local_port_(port),
    mtu_size_(DEFAULT_MTU_SIZE),
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

int KcpClient::disconnect()
{
    int ret = 0;

    // make disconnect packet
    std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>(server_ip_, server_port_, 
        FRAMETYPE_REQUEST_DISCONNECT, 0);

    ret = frame_ptr->encode(reinterpret_cast<uint8_t *>(&conv_), sizeof(conv_));
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
        return ((this->run_.load() == false) || (this->run_state_.load() == 5));
    });

    if (!run_.load() || (status == false))
    {
        LOG_INFO("disconnect timeout %d", this->run_state_.load());
        return -1;
    }

    LOG_INFO("disconnect success conv:%d", conv_);

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
