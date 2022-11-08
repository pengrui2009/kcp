#include "asio_udp.h"
#include <memory>


void ASIOUdp::OnSendDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred)
{
    size_t result = -1;
    
    // asio_endpoint_t remote_endpoint(
    //     boost::asio::ip::address_v4::from_string(ip), port);
    // result = socket_ptr_->send_to(boost::asio::buffer(data_ptr, data_len), remote_endpoint);

    // if (result != data_len)
    // {
    //     ret = -1;
    // }
}

void ASIOUdp::OnReceiveDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred) 
{
    if (!run_.load()) {
        return;
    }

    if (ec.value() != 0) {
        throw boost::system::system_error(ec);
    }

    std::cout << "OnReceiveDataCallback bytes_transferred:" << bytes_transferred << std::endl;        
    
    do_async_receive_once();
}

void ASIOUdp::get_ipaddress(const asio_endpoint_t &endpoint, std::string &ip, uint16_t &port)
{
    boost::asio::ip::address addr = endpoint.address();

    ip = addr.to_string();
    port = endpoint.port();
}

void ASIOUdp::set_ipaddress(asio_endpoint_t &endpoint, const std::string &ip, const uint16_t &port)
{
    boost::asio::ip::address addr = boost::asio::ip::address_v4::from_string(ip);
    
    endpoint = asio_endpoint_t(addr, port);
}

void ASIOUdp::do_async_receive_once()
{
    asio_endpoint_t endpoint;

    databuffer_.fill(0U);
    if (socket_ptr_ != nullptr) {
        socket_ptr_->async_receive_from(
            boost::asio::buffer(databuffer_, KEthPacketMaxLength), endpoint_,
            boost::bind(
                &ASIOUdp::OnReceiveDataCallback, this, &boost::asio::placeholders::error,
                &boost::asio::placeholders::bytes_transferred));
    }
}

ASIOUdp::ASIOUdp(TxMode mode, uint16_t port) :
    local_ip_(""),
    local_port_(port)
{
    service_ptr_ = std::make_shared<boost::asio::io_service>();

    socket_ptr_ = std::make_shared<boost::asio::ip::udp::socket>(
        *service_ptr_.get(), asio_endpoint_t(boost::asio::ip::udp::v4(), port));
        
    run_.store(true, std::memory_order_release);
}


ASIOUdp::ASIOUdp(TxMode mode, std::string &ip, uint16_t port) :
    local_ip_(ip),
    local_port_(port)
{
    databuffer_.fill(0);
    service_ptr_ = std::make_shared<boost::asio::io_service>();

    if (ip.empty())
    {
        socket_ptr_ = std::make_shared<boost::asio::ip::udp::socket>(
            *service_ptr_.get(), asio_endpoint_t(boost::asio::ip::udp::v4(), port));
    } else {
        socket_ptr_ = std::make_shared<boost::asio::ip::udp::socket>(
            *service_ptr_.get(), asio_endpoint_t(boost::asio::ip::address_v4::from_string(ip), port));
    }
    
    run_.store(true, std::memory_order_release);
}

ASIOUdp::~ASIOUdp()
{
    databuffer_.fill(0);
    socket_ptr_.reset();
    service_ptr_.reset();
}

int ASIOUdp::initialize()
{
    if ((socket_ptr_ == nullptr) || (service_ptr_ == nullptr))
    {
        return -1;
    }

    do_async_receive_once();

    // std::thread([this]() {
    //     service_ptr_->run();
    // }).detach();

    return 0;
}

int ASIOUdp::start()
{
    if ((socket_ptr_ == nullptr) || (service_ptr_ == nullptr))
    {
        return -1;
    }

    
    
    return 0;            
}

int ASIOUdp::run()
{
    if ((socket_ptr_ == nullptr) || (service_ptr_ == nullptr))
    {
        return -1;
    }

    service_ptr_->run();

    return 0;
}

int ASIOUdp::send(const uint8_t *data_ptr, size_t data_len, const std::string &ip, const uint16_t port)
{
    int ret = 0;

    asio_endpoint_t remote_endpoint(
        boost::asio::ip::address_v4::from_string(ip), port);

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
                &ASIOUdp::OnSendDataCallback, this, &boost::asio::placeholders::error,
                &boost::asio::placeholders::bytes_transferred));
        }
        break;
    default:
        ret = -1;
        break;
    }
    
    return ret;
}

int ASIOUdp::send(const uint8_t *data_ptr, size_t data_len, const asio_endpoint_t &dest)
{
    int ret = 0;

    switch (txmode_)
    {
    case NORMAL_MODE:
        {
            size_t result = -1;
            result = socket_ptr_->send_to(boost::asio::buffer(data_ptr, data_len), dest);
            if (result != data_len)
            {
                ret = -1;
            }
        }
        break;
    case ASIO_MODE:
        {
            socket_ptr_->async_send_to(boost::asio::buffer(data_ptr, data_len), dest,
                boost::bind(
                &ASIOUdp::OnSendDataCallback, this, &boost::asio::placeholders::error,
                &boost::asio::placeholders::bytes_transferred));
        }
        break;
    default:
        ret = -1;
        break;
    }
    
    return ret;
}