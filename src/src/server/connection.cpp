#include "connection.h"
#include "udp_kcp.h"
#include <mutex>

RandomConv::RandomConv(int offset, int size)
{
    this->seed_size_ = 0;

    if (offset >= RAND_MAX)
    {
        this->offset_ = RAND_MAX;
        this->seed_size_ = 0;
        throw std::runtime_error("offset >= 2147483647");
    }
    
    this->seed_size_ = ((RAND_MAX - offset) > size) ? size : (RAND_MAX - offset);
    this->left_size_ = this->seed_size_;
    this->offset_ = offset;
    this->seeds_.resize(this->seed_size_);

    for (int i=0; i< this->seed_size_; i++)
    {
        this->seeds_[i] = i;
    }
}

int RandomConv::get_conv(kcp_conv_t &conv)
{
    int ret = 0;
    int temp = 0;
    std::lock_guard<std::mutex> lock(mutex_);;

    if (left_size_ == 0) 
    {
        LOG_ERROR("get_conv vector size is empty.");
        return -1;
    }

    srand(time(0));
    temp = rand() % left_size_;
    conv = seeds_[temp] + offset_;
    seeds_[temp] = seeds_[--left_size_];

    return 0;
}

int RandomConv::put_conv(const kcp_conv_t &conv)
{
    std::lock_guard<std::mutex> lock(mutex_);;
    int temp = 0;

    if (left_size_ >= seed_size_)
    {
        LOG_ERROR("put_conv vector size is full.");
        return -1;
    }

    temp = conv - offset_;
    if (temp > seed_size_)
    {
        LOG_ERROR("invalid conv value.");
        return -1;
    }
    seeds_[left_size_++] = temp;
    
    return 0;
}

RandomConv::~RandomConv()
{
    seeds_.clear();
    seed_size_ = 0;
    left_size_ = 0;
}



Connection::Connection(const kcp_conv_t conv, asio_endpoint_t endpoint, 
    int mtu_size, KcpServer *server_ptr) :
    conv_(conv),
    mtu_size_(mtu_size),
    endpoint_(endpoint)
{
    kcp_ptr_ = ikcp_create(conv, (void *)server_ptr);
    kcp_ptr_->output = &Connection::kcp_output;

    // set mtu size
    ikcp_setmtu(kcp_ptr_, mtu_size);

    ikcp_wndsize(kcp_ptr_, 128, 128);

    // default mode
    // ikcp_nodelay(kcp_ptr_, 0, 10, 0, 0);

    // normal mode
    // ikcp_nodelay(kcp_ptr_, 0, 10, 0, 1);
    
    // fast mode interval: 10ms disable, enable resend, flow control
    ikcp_nodelay(kcp_ptr_, 2, 10, 2, 1);
    kcp_ptr_->rx_minrto = 10;
    kcp_ptr_->fastresend = 1;

    stop_.store(false, std::memory_order_release);
    LOG_INFO("Connection init");
}

Connection::~Connection()
{
    ikcp_release(kcp_ptr_);
    kcp_ptr_ = NULL;
    conv_ = 0;
    stop_.store(true, std::memory_order_release);
}

int Connection::kcp_input(const asio_endpoint_t& dest, const uint8_t* data_ptr, size_t data_len)
{
    int ret = 0;
    std::string ip = "";
    uint16_t port = 0x00;

    endpoint_ = dest;

    boost::asio::ip::address addr = dest.address();

    ip = addr.to_string();
    port = dest.port();
    // get_ipaddress(dest, ip, port);

    // LOG_INFO("[%s:%d] recv input data_len:%d ", ip.c_str(), port, data_len);
    // for (int i=0; i<data_len; i++)
    // {
    //     fprintf(stderr, " %02X", data_ptr[i]);
    // }
    // fprintf(stderr, "\n");

    ret = ikcp_input(kcp_ptr_, reinterpret_cast<const char *>(data_ptr), data_len);
    if (ret)
    {
        LOG_ERROR("ikcp_input failed, ret:%d", ret);
        return -1;
    }

    // LOG_INFO("ikcp_input succes");
    return 0;
}

int Connection::kcp_output(const char *data_ptr, int data_len, struct IKCPCB *kcp_ptr, void *user_ptr)
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
    // LOG_INFO("kcp_output send udp packet");
    // udp send 
    return ((KcpServer*)user_ptr)->send_udp_packet(kcp_ptr, reinterpret_cast<const uint8_t *>(data_ptr), data_len);
}

int Connection::kcp_send(const uint8_t *data_ptr, int data_len)
{
    int ret = 0;

    ret = ikcp_send(kcp_ptr_, reinterpret_cast<const char *>(data_ptr), data_len);
    if (ret)
    {
        LOG_ERROR("response base frame ikcp_send failed, ret:%d", ret);
        return -1;
    }

    return 0;
}

int Connection::kcp_receive(uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;
    // size_t buffer_size = 1024;
    // std::unique_ptr<uint8_t[]> buffer_ptr = std::make_unique<uint8_t[]>(buffer_size);
    // size_t buffer_size = 50;
    // uint8_t buffer_ptr[50] = {0};

    ret = ikcp_recv(kcp_ptr_, reinterpret_cast<char *>(data_ptr), data_len);
    if (ret <= 0)
    {
        // LOG_ERROR("response base frame ikcp_send failed, ret:%d", ret);
        return -1;
    }

    return ret;

}

void Connection::kcp_update(uint32_t clock)
{
    
    if (kcp_ptr_ == nullptr)
    {
        return;
    }

    ikcp_update(kcp_ptr_, clock);
}

void Connection::kcp_flush()
{
    if (kcp_ptr_ == nullptr)
    {
        return;
    }

    ikcp_flush(kcp_ptr_);
}