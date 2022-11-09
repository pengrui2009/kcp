#include "udp_kcp.h"
#include "asio_udp.h"
#include "connection.h"
#include "timestamp.h"

#include "base_frame.h"
#include "request_connect_frame.h"
#include "request_disconnect_frame.h"
#include "response_frame.h"

#include "logger.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

Packet::Packet()
{

}

Packet::~Packet()
{

}

int KcpServer::output(const char *data_ptr, int data_len, struct IKCPCB *kcp_ptr, void *user_ptr)
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
    return ((KcpServer*)user_ptr)->send_udp_packet(kcp_ptr, reinterpret_cast<const uint8_t *>(data_ptr), data_len);
}

int KcpServer::send_udp_packet(struct IKCPCB *kcp_ptr, const uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;
    // std::string dest_ip = "";
    // uint16_t dest_port = 0;
    kcp_conv_t kcp_conv = 0;
    // std::lock_guard<std::mutex> lock(connections_mutex_);

    // if (data_len < KCP_FRAME_SIZE)
    // {
    //     LOG_ERROR("send_udp_packet failed, %d <= %d", data_len, KCP_FRAME_SIZE);
    //     return -1;
    // }

    ret = get_packet_conv(data_ptr, data_len, kcp_conv);
    if (ret)
    {
        LOG_ERROR("get_packet_conv failed, ret:%d", ret);
        return -1;
    }

    if (connections_.count(kcp_conv) > 0)
    {
        asio_endpoint_t dest = connections_[kcp_conv]->get_endpoint();
        ret = send(data_ptr, data_len, dest);
        if (ret)
        {    
            LOG_ERROR("frame send failed, ret:%d", ret);
            return -1;
        }
        
        return 0;
    } else {
        LOG_ERROR("get_packet_conv failed, ret:%d", ret);
        return -1;
    }
}

void KcpServer::OnSendDataCallback(const boost::system::error_code &ec, 
    std::size_t bytes_transferred)
{
    // TODO current not support
    // asio udp send
}

int KcpServer::send_udp_packet(const asio_endpoint_t &dest, const uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;

    ret = send(data_ptr, data_len, dest);
    if (ret)
    {    
        LOG_ERROR("frame send failed, ret:%d", ret);
        return -1;
    }

    return 0;
}

int KcpServer::send_kcp_packet(const kcp_conv_t conv, const uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;

    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    if (!connections_.count(conv))
    {
        LOG_ERROR("conv %d is invalid.", conv);
        return -1;
    }

    ret = connections_[conv]->kcp_send(data_ptr, data_len);
    if (ret)
    {
        LOG_ERROR("kcp_send failed, ret:%d", ret);
        return -1;
    }

    return 0;
}

int KcpServer::handle_connect_frame(const asio_endpoint_t &dest, kcp_conv_t &kcp_conv)
{
    int ret = 0;
    kcp_conv_t conv = 0;
    std::string dest_ip = "";
    uint16_t dest_port = 0x00;

    ret = get_new_conv(conv);
    if (ret)
    {
        LOG_ERROR("get new conv failed.");
        return -1;
    }

    kcp_conv = conv;
    get_ipaddress(dest, dest_ip, dest_port);
    // LOG_INFO("dest info %s:%d", dest_ip.c_str(), dest_port);
    
    // create response packet
    uint64_t timestamp = Timestamp::NanoSecond();
    std::shared_ptr<ResponseFrame> frame_ptr = std::make_shared<ResponseFrame>(sign_key_, 
        timestamp, dest_ip, dest_port, 0, MSGTYPE_CONNECTED, conv);

    ret = frame_ptr->encode();
    if (ret)
    {
        LOG_ERROR("response connect packet encode failed, ret:%d", ret);
        return -1;
    }

    // udp send
    ret = send_udp_packet(dest, frame_ptr->buffer_.data(), frame_ptr->buffer_.size());
    if (ret)
    {
        LOG_ERROR("response connect packet send failed, ret:%d", ret);
        return -1;
    }
    // add connection
    std::shared_ptr<Connection> connect_ptr = std::make_shared<Connection>(
        conv, dest, DEFAULT_MTU_SIZE, this);

    connections_[conv] = connect_ptr;

    return 0;
}

int KcpServer::handle_disconnect_frame(const asio_endpoint_t &dest, kcp_conv_t conv)
{
    int ret = 0;

    std::string dest_ip = "";
    uint16_t dest_port = 0x00;

    get_ipaddress(dest, dest_ip, dest_port);
    // LOG_INFO("dest info %s:%d", dest_ip.c_str(), dest_port);
    // create response packet
    uint64_t timestamp = Timestamp::NanoSecond();
    std::shared_ptr<ResponseFrame> frame_ptr = std::make_shared<ResponseFrame>(sign_key_, 
        timestamp, dest_ip, dest_port, 0, MSGTYPE_DISCONNECTED, conv);

    ret = frame_ptr->encode();
    if (ret)
    {
        LOG_ERROR("response disconnect packet encode failed, ret:%d", ret);
        return -1;
    }

    // udp send
    ret = send_udp_packet(dest, frame_ptr->buffer_.data(), frame_ptr->buffer_.size());
    if (ret)
    {
        LOG_ERROR("response disconnect packet send failed, ret:%d", ret);
        return -1;
    }

    std::lock_guard<std::mutex> lock(connections_mutex_);
    // remove connection
    auto iter = connections_.find(conv);
    if (iter != connections_.end())
        connections_.erase(iter);

    return 0;
}

int KcpServer::handle_udp_packet(asio_endpoint_t dest, uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;
    // std::shared_ptr<BaseFrame> frame_ptr = std::make_shared<BaseFrame>();

    // if (data_len < FRAME_HEADER_SIZE)
    // {
    //     LOG_ERROR("udp frame length is not enought %d < %d", data_len, FRAME_HEADER_SIZE);
    //     return -1;
    // }

    // ret = frame_ptr->decode(data_ptr, data_len);
    // if (ret)
    // {
    //     LOG_ERROR("base frame decode failed, ret:%d", ret);
    //     return -1;
    // }

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
    if (data_len < PAIRFRAME_MIN_SIZE)
    {
        LOG_ERROR("length is not enought %d < %d", data_len, PAIRFRAME_MIN_SIZE);
        return -1;
    }

    MsgType type = static_cast<MsgType>(data_ptr[PAIRFRAME_MSGTYPE_OFFSET]);
    switch(type)
    {
    case MSGTYPE_REQUEST_CONNECT:
    {
        ukcp_info_t ukcp;

        std::shared_ptr<RequestConnectFrame> frame_ptr = std::make_shared<RequestConnectFrame>(sign_key_);

        ret = frame_ptr->decode(data_ptr, data_len);
        if(ret)
        {
            LOG_ERROR("Request Connect Frame disconnect failed.");
            return -1;
        }

        ret = handle_connect_frame(dest, ukcp.conv);
        if (ret < 0)
        {
            LOG_ERROR("handle_connect_frame failed, ret:%d", ret);
            return -1;
        }

        // ukcp.conv = kcp_conv;
        ukcp.endpoint = dest;

        event_callback_(ukcp, EVENT_TYPE_CONNECT, nullptr, 0);

        break;
    }
    case MSGTYPE_CONNECTED:
        break;
    case MSGTYPE_REQUEST_DISCONNECT:
    {
        ukcp_info_t ukcp;
        std::shared_ptr<RequestDisconnectFrame> frame_ptr = std::make_shared<RequestDisconnectFrame>(sign_key_);
        
        ret = frame_ptr->decode(data_ptr, data_len);
        if(ret)
        {
            LOG_ERROR("Request Connect Frame disconnect failed.");
            return -1;
        }

        kcp_conv_t kcp_conv = frame_ptr->get_kcp_conv();

        ukcp.conv = kcp_conv;
        ukcp.endpoint = dest;

        ret = handle_disconnect_frame(dest, kcp_conv);
        if (ret)
        {
            LOG_ERROR("handle_disconnect_frame failed, ret:%d", ret);
            return -1;
        }

        event_callback_(ukcp, EVENT_TYPE_DISCONNECT, nullptr, 0);

        break;
    }
    case MSGTYPE_DISCONNECTED:
        break;
    }

    return 0;
}

int KcpServer::handle_kcp_packet(asio_endpoint_t dest, uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;
    kcp_conv_t packet_conv = 0;
    std::lock_guard<std::mutex> lock(connections_mutex_);

    ret = get_packet_conv(data_ptr, data_len, packet_conv);
    if (ret)
    {
        LOG_ERROR("get_packet_conv failed, ret:%d", ret);
        return -1;
    }

    if (connections_.count(packet_conv) > 0)
    {
        ret = connections_[packet_conv]->kcp_input(dest, data_ptr, data_len);
        if (ret)
        {
            LOG_ERROR("connection input failed, ret:%d", ret);
            return -1;
        }
    }
    // LOG_INFO("handle_kcp_packet success.");

    return 0;
}


int KcpServer::do_receive_handle(asio_endpoint_t dest, std::size_t bytes_transferred)
{
    int ret = 0;
    // do receive data handle

    if (bytes_transferred < PAIRFRAME_MSGSYNC_SIZE)
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
        // LOG_INFO("handle_kcp_packet");
        ret = handle_kcp_packet(dest, databuffer_.data(), bytes_transferred);
        if (ret)
        {
            LOG_ERROR("handle_kcp_packet failed, ret:%d", ret);
            return -1;
        }
    }

    return 0;
}

void KcpServer::OnReceiveDataCallback(const boost::system::error_code &ec, 
    std::size_t bytes_transferred)
{
    int ret = 0;
    boost::asio::ip::udp::endpoint endpoint = endpoint_;
    std::string ip = ""; 
    uint16_t port = 0x00;
    get_ipaddress(endpoint, ip, port);
    // LOG_INFO("OnReceiveDataCallback:%s:%d", ip.c_str(), port);
    ret = do_receive_handle(endpoint, bytes_transferred);
    if (ret)
    {
        LOG_ERROR("HandleReceiveData failed, ret:%d", ret);
    }
    
    do_async_receive_once();
}


int KcpServer::get_packet_conv(const uint8_t *data_ptr, size_t data_len, kcp_conv_t &conv)
{
    if (data_len < KCP_CONV_SIZE)
    {
        LOG_ERROR("length is not enought %d < %d", data_len, KCP_CONV_SIZE);
        return -1;
    }

    conv = ikcp_getconv(reinterpret_cast<const void *>(data_ptr));
    return 0;
}

/**
 * @brief get random conv 
 * 
 * @return kcp_conv_t 
 */
int KcpServer::get_new_conv(kcp_conv_t &conv) const
{
    int ret = 0;
    // todo using rand to get a conv. privent the attack from guess conv.
    // and must bigger than 1000

    // increase from 1001, must bigger than 1000
    // static uint32_t static_cur_conv = 0x6666888;
    // static_cur_conv++;
    kcp_conv_t new_conv = 0x00;
    
    ret = random_conv_ptr_->get_conv(new_conv);
    if (ret)
    {
        LOG_ERROR("random conv get_conv failed.");
        return -1;
    }
    conv = new_conv;

    return 0;
}

int KcpServer::do_kcp_timer_handle()
{
    if (!run_.load())
    {
        return -1;
    }
    
    kcp_timer_.expires_from_now(boost::posix_time::milliseconds(10));
    kcp_timer_.async_wait(std::bind(&KcpServer::handle_kcp_time, this));

    return 0;
}

void KcpServer::handle_kcp_time()
{
    int ret = 0;
    uint32_t timestamp = 0x00;

    do_kcp_timer_handle();

    timestamp = Timestamp::iClock();
    
    this->update(timestamp);

    // std::lock_guard<std::mutex> lock(connections_mutex_);
    size_t buffer_size = mtu_size_ + KCP_FRAME_SIZE;
    std::unique_ptr<uint8_t[]> buffer_ptr = std::make_unique<uint8_t[]>(buffer_size);
    for (auto iter = connections_.begin(); iter != connections_.end(); iter++)    
    {
        ukcp_info_t ukcp;
        std::shared_ptr<Connection> &ptr = iter->second;

        ukcp.conv = iter->second->get_kcpconv();
        ukcp.endpoint = iter->second->get_endpoint();

        ret = ptr->kcp_receive(buffer_ptr.get(), buffer_size);
        if (ret > 0)
        {
            // LOG_INFO("receive data_len:%d ", ret);
            event_callback_(ukcp, EVENT_TYPE_RECEIVE, buffer_ptr.get(), ret);
        }
        
    }
}

int KcpServer::set_mtu(int mtu_size)
{
    int ret = 0;

    if (mtu_size < 50 || mtu_size < (int)KCP_FRAME_SIZE)
    {
		return -1;
    }

    mtu_size_ = mtu_size;

    return 0;
}

void KcpServer::set_event_callback(const std::function<event_callback_func>& func)
{
    this->event_callback_ = func;
}

KcpServer::KcpServer(std::string ip, uint16_t port) : ASIOUdp(NORMAL_MODE, ip, port),
    kcp_timer_(*service_ptr_),
    mtu_size_(DEFAULT_MTU_SIZE)
{
    max_clients_size_ = DEFAULT_MAX_CLIENT_SIZE;
    random_conv_ptr_ = std::make_shared<RandomConv>(MIN_CONV_VALUE, max_clients_size_);
}

KcpServer::~KcpServer()
{
    run_.store(false, std::memory_order_release);
}

int KcpServer::initialize()
{
    int ret = 0;

    ret = ASIOUdp::initialize();
    if (ret)
    {
        LOG_ERROR("asio udp initialize failed, ret:%d", ret);
        return -1;
    }

    run_.store(true, std::memory_order_release);
    
    do_kcp_timer_handle();
    
    return 0;
}

int KcpServer::run()
{
    int ret = 0;

    ret = ASIOUdp::run();
    if (ret)
    {
        LOG_ERROR("asioudp run failed, ret:%d", ret);
        return -1;
    }
    return 0;
}

int KcpServer::update(uint32_t clock)
{
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    for (auto iter = connections_.begin(); iter != connections_.end();)
    {
        std::shared_ptr<Connection> &ptr = iter->second;

        ptr->kcp_update(clock);

        // check timeout
        // if (ptr->is_timeout())
        // {
        //     ptr->do_timeout();
        //     connections_.erase(iter++);
        //     continue;
        // }

        iter++;
    }

}
