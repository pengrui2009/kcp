#include "ikcp.h"

#include <cstdint>
#include <iostream>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <memory>

constexpr size_t KEthPacketMaxLength = 4096;

class asioudp {
public:
    enum SendMethod {
        NORMAL_SEND = 0,
        ASIO_SEND = 1,
    };

    asioudp(uint16_t port = 36001);
    ~asioudp();
    void run();

    int send(SendMethod method, uint8_t *data_ptr, size_t data_len, const std::string &ip, const uint16_t port);

private:
    uint16_t port_;
    std::atomic_bool run_;
    std::atomic_bool ready_;

    std::shared_ptr<boost::asio::ip::udp::socket> socket_ptr_;
    std::shared_ptr<boost::asio::io_service> service_ptr_;
    boost::asio::ip::udp::endpoint endpoint_ptr_;

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
        boost::asio::buffer(tempBuffer_, KEthPacketMaxLength), endpoint_ptr_,
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

int asioudp::send(SendMethod method, uint8_t *data_ptr, size_t data_len, const std::string &ip, const uint16_t port)
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
        // const uint64_t master_stamp_ns = middleware::Time::Now().ToNsec();
        // auto *eth_packet = reinterpret_cast<LivoxEthPacket *>(tempBuffer_.data());
        // LivoxTimeStamp cur_timestamp{};
        // memcpy(cur_timestamp.stamp_bytes.data(), eth_packet->timestamp.data(), sizeof(cur_timestamp));
        // if (eth_packet->data_type == static_cast<uint8_t>(PointDataType::kImu)) {
        //     if (bytes_transferred != kIMUMsgEthPktSize) {
        //     MW_ERROR("Recv IMU msg of size {}, which should be {}", bytes_transferred, kIMUMsgEthPktSize);
        //     }
        //     StoragePacket temp_packet{};
        //     temp_packet.time_rcv = master_stamp_ns;
        //     temp_packet.point_num = 1U;
        //     memcpy(temp_packet.raw_data.data(), tempBuffer_.data(), bytes_transferred);
        //     ProcessIMUData(&temp_packet);
        // }
        // if (eth_packet->data_type == static_cast<uint8_t>(PointDataType::kExtendCartesian)) {
        //     if (bytes_transferred != kLidarPacketTotalLength) {
        //     MW_ERROR("Recv lidar msg of size {}, which should be {}", bytes_transferred, kLidarPacketTotalLength);
        //     }
        //     CheckEthPacket(nullptr);
        //     StoragePacket temp_packet{};
        //     temp_packet.time_rcv = master_stamp_ns;
        //     temp_packet.point_num =
        //         static_cast<uint32_t>((bytes_transferred - kPrefixDataSize) / sizeof(LivoxExtendRawPoint));
        //     if (temp_packet.point_num != kPointPerEthPacket) {
        //     MW_WARN("Pointnum is not {} in this packet", kPointPerEthPacket);
        //     }
        //     memcpy(temp_packet.raw_data.data(), tempBuffer_.data(), bytes_transferred);
        //     udpPktsStorage_.push_back(temp_packet);
        //     if ((cur_timestamp.stamp / 100000000U) != lastPacketTimeStamp_) {  // publish every 0.1s
        //     if (lastPacketTimeStamp_ != 0U) {
        //         ProcessPointCloudData();
        //     }
        //     lastPacketTimeStamp_ = cur_timestamp.stamp / 100000000U;
        //     }
        // }
    }
    tempBuffer_.fill(0U);
    if (socket_ptr_ != nullptr) {
    socket_ptr_->async_receive_from(
        boost::asio::buffer(tempBuffer_, KEthPacketMaxLength), endpoint_ptr_,
        boost::bind(
            &asioudp::OnReceiveDataCallback, this, &boost::asio::placeholders::error,
            &boost::asio::placeholders::bytes_transferred));
    }

}

int main()
{
    std::shared_ptr<asioudp> asioudp_ptr = std::make_shared<asioudp>(36001);

    asioudp_ptr->run();

    return 0;
}