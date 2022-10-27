#include "asio_udp.h"


void AsioUdp::OnSendDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred)
{
    size_t result = -1;
    
    // boost::asio::ip::udp::endpoint remote_endpoint(
    //     boost::asio::ip::address_v4::from_string(ip), port);
    // result = socket_ptr_->send_to(boost::asio::buffer(data_ptr, data_len), remote_endpoint);

    // if (result != data_len)
    // {
    //     ret = -1;
    // }
}

void AsioUdp::OnReceiveDataCallback(const boost::system::error_code &ec, std::size_t bytes_transferred) 
{
    if (!run_.load()) {
        return;
    }

    if (ec.value() != 0) {
        throw boost::system::system_error(ec);
    }

    std::cout << "OnReceiveDataCallback bytes_transferred:" << bytes_transferred << std::endl;        
    
    tempBuffer_.fill(0U);
    if (socket_ptr_ != nullptr) {
    socket_ptr_->async_receive_from(
        boost::asio::buffer(tempBuffer_, KEthPacketMaxLength), endpoint_ptr_,
        boost::bind(
            &AsioUdp::OnReceiveDataCallback, this, &boost::asio::placeholders::error,
            &boost::asio::placeholders::bytes_transferred));
    }
}

void AsioUdp::get_ipaddress(const boost::asio::ip::udp::endpoint &endpoint, std::string &ip, uint16_t &port)
{
    boost::asio::ip::address addr = endpoint.address();

    ip = addr.to_string();
    port = endpoint.port();
}

void AsioUdp::set_ipaddress(boost::asio::ip::udp::endpoint &endpoint, const std::string &ip, const uint16_t &port)
{
    boost::asio::ip::address addr = boost::asio::ip::address_v4::from_string(ip);
    
    endpoint = boost::asio::ip::udp::endpoint(addr, port);
}

AsioUdp::AsioUdp(TxMode mode = ASIO_MODE, uint16_t port) :
    local_ip_(""),
    local_port_(port)
{
    service_ptr_ = std::make_shared<boost::asio::io_service>();

    socket_ptr_ = std::make_shared<boost::asio::ip::udp::socket>(
        *service_ptr_.get(), boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port));
        
    run_.store(true, std::memory_order_release);
}


AsioUdp::AsioUdp(TxMode mode = ASIO_MODE, std::string &ip, uint16_t port) :
    local_ip_(ip),
    local_port_(port)
{
    databuffer_.clear();
    service_ptr_ = std::make_shared<boost::asio::io_service>();

    if (ip == nullptr)
    {
        socket_ptr_ = std::make_shared<boost::asio::ip::udp::socket>(
            *service_ptr_.get(), boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port));
    } else {
        socket_ptr_ = std::make_shared<boost::asio::ip::udp::socket>(
            *service_ptr_.get(), boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::from_string(ip), port));
    }
    
    run_.store(true, std::memory_order_release);
}

AsioUdp::~AsioUdp()
{
    databuffer_.clear();
    socket_ptr_.reset();
    service_ptr_.reset();
}

int AsioUdp::initialize()
{
    if ((socket_ptr_ == nullptr) || (service_ptr_ == nullptr))
    {
        return -1;
    }

    return 0;
}

int AsioUdp::start()
{
    if ((socket_ptr_ == nullptr) || (service_ptr_ == nullptr))
    {
        return -1;
    }

    databuffer_.fill(0);
    socket_ptr_->async_receive_from(
        boost::asio::buffer(databuffer_, KEthPacketMaxLength), endpoint_,
        boost::bind(
            &AsioUdp::OnReceiveDataCallback, this, &boost::asio::placeholders::error,
            &boost::asio::placeholders::bytes_transferred));
    return 0;            
}

int AsioUdp::run()
{
    if ((socket_ptr_ == nullptr) || (service_ptr_ == nullptr))
    {
        return -1;
    }

    service_ptr_->run();

    return 0;
}

int AsioUdp::send(uint8_t *data_ptr, size_t data_len, const std::string &ip, const uint16_t port)
{
    int ret = 0;

    boost::asio::ip::udp::endpoint remote_endpoint(
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
