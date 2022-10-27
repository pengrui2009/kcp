#include "base_frame.h"

#include "logger.h"
#include <cstdint>

// std::array<uint8_t, 24> BaseFrame::signkey_ = {0x11, 0x12, 0x13, 0x14};

uint64_t BaseFrame::ReadUint64BE(uint8_t* data)
{
    uint8_t* p = (uint8_t*)data;
    uint64_t value = static_cast<uint64_t>(static_cast<uint64_t>(p[0]) << 56) | \
                     static_cast<uint64_t>(static_cast<uint64_t>(p[1]) << 48) | \
                     static_cast<uint64_t>(static_cast<uint64_t>(p[2]) << 40) | \
                     static_cast<uint64_t>(static_cast<uint64_t>(p[3]) << 32) | \
                     static_cast<uint64_t>(static_cast<uint64_t>(p[4]) << 24) | \
                     static_cast<uint64_t>(static_cast<uint64_t>(p[5]) << 16) | \
                     static_cast<uint64_t>(static_cast<uint64_t>(p[6]) << 8)  | \
                     static_cast<uint64_t>(p[7]);
    return value;
}

uint64_t BaseFrame::ReadUint64LE(uint8_t* data)
{
    uint8_t* p = (uint8_t*)data;
    uint64_t value = static_cast<uint64_t>(static_cast<uint64_t>(p[7]) << 56) | \
                     static_cast<uint64_t>(static_cast<uint64_t>(p[6]) << 48) | \
                     static_cast<uint64_t>(static_cast<uint64_t>(p[5]) << 40) | \
                     static_cast<uint64_t>(static_cast<uint64_t>(p[4]) << 32) | \
                     static_cast<uint64_t>(static_cast<uint64_t>(p[3]) << 24) | \
                     static_cast<uint64_t>(static_cast<uint64_t>(p[2]) << 16) | \
                     static_cast<uint64_t>(static_cast<uint64_t>(p[1]) << 8)  | \
                     static_cast<uint64_t>(p[0]);
    return value;
}

uint32_t BaseFrame::ReadUint32BE(uint8_t* data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = static_cast<uint32_t>(static_cast<uint32_t>(p[0]) << 24) | \
                     static_cast<uint32_t>(static_cast<uint32_t>(p[1]) << 16) | \
                     static_cast<uint32_t>(static_cast<uint32_t>(p[2]) << 8)  | \
                     static_cast<uint32_t>(p[3]);
    return value;
}

uint32_t BaseFrame::ReadUint32LE(uint8_t* data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = static_cast<uint32_t>(static_cast<uint32_t>(p[3]) << 24) | \
                     static_cast<uint32_t>(static_cast<uint32_t>(p[2]) << 16) | \
                     static_cast<uint32_t>(static_cast<uint32_t>(p[1]) << 8)  | \
                     static_cast<uint32_t>(p[0]);
    return value;
}

uint32_t BaseFrame::ReadUint24BE(uint8_t* data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = static_cast<uint32_t>(static_cast<uint32_t>(p[0]) << 16) | \
                     static_cast<uint32_t>(static_cast<uint32_t>(p[1]) << 8)  | \
                     static_cast<uint32_t>(p[2]);
    return value;
}

uint32_t BaseFrame::ReadUint24LE(uint8_t* data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = static_cast<uint32_t>(static_cast<uint32_t>(p[2]) << 16) | \
                     static_cast<uint32_t>(static_cast<uint32_t>(p[1]) << 8)  | \
                     static_cast<uint32_t>(p[0]);
    return value;
}

uint16_t BaseFrame::ReadUint16BE(uint8_t* data)
{
    uint8_t* p = (uint8_t*)data;
    uint16_t value = static_cast<uint16_t>(static_cast<uint16_t>(p[0]) << 8) | \
                     static_cast<uint16_t>(p[1]);
    return value; 
}

uint16_t BaseFrame::ReadUint16LE(uint8_t* data)
{
    uint8_t* p = (uint8_t*)data;
    uint16_t value = static_cast<uint16_t>(static_cast<uint16_t>(p[1]) << 8) | \
                     static_cast<uint16_t>(p[0]);
    return value; 
}

void BaseFrame::WriteUint64BE(uint8_t* p, uint64_t value)
{
    p[0] = (value >> 56) & 0xFF;
    p[1] = (value >> 48) & 0xFF;
    p[2] = (value >> 40) & 0xFF;
    p[3] = (value >> 32) & 0xFF;
    p[4] = (value >> 24) & 0xFF;
    p[5] = (value >> 16) & 0xFF;
    p[6] = (value >> 8) & 0xFF;
    p[7] = value & 0xFF;
}

void BaseFrame::WriteUint64LE(uint8_t* p, uint64_t value)
{
    p[0] = value & 0xFF;
    p[1] = (value >> 8) & 0xFF;
    p[2] = (value >> 16) & 0xFF;
    p[3] = (value >> 24) & 0xFF;
    p[4] = (value >> 32) & 0xFF;
    p[5] = (value >> 40) & 0xFF;
    p[6] = (value >> 48) & 0xFF;
    p[7] = (value >> 56) & 0xFF;
}

void BaseFrame::WriteUint32BE(uint8_t* p, uint32_t value)
{
    p[0] = (value >> 24) & 0xFF;
    p[1] = (value >> 16) & 0xFF;
    p[2] = (value >> 8) & 0xFF;
    p[3] = value & 0xFF;
}

void BaseFrame::WriteUint32LE(uint8_t* p, uint32_t value)
{
    p[0] = value & 0xFF;
    p[1] = (value >> 8) & 0xFF;
    p[2] = (value >> 16) & 0xFF;
    p[3] = (value >> 24) & 0xFF;
}

void BaseFrame::WriteUint24BE(uint8_t* p, uint32_t value)
{
    p[0] = (value >> 16) & 0xFF;
    p[1] = (value >> 8) & 0xFF;
    p[2] = value & 0xFF;
}

void BaseFrame::WriteUint24LE(uint8_t* p, uint32_t value)
{
    p[0] = value & 0xFF;
    p[1] = (value >> 8) & 0xFF;
    p[2] = (value >> 16) & 0xFF;
}

void BaseFrame::WriteUint16BE(uint8_t* p, uint16_t value)
{
    p[0] = ((value >> 8) & 0xFF);
    p[1] = value & 0xFF;
}

void BaseFrame::WriteUint16LE(uint8_t* p, uint16_t value)
{
    p[0] = value & 0xFF;
    p[1] = (value >> 8) & 0xFF;
}

BaseFrame::BaseFrame() : 
    type_(FRAMETYPE_UNKNOWN),
    count_(0)
{
    buffer_.clear();
}

BaseFrame::BaseFrame(std::string &ip, uint16_t port, FrameType type, uint8_t count) : 
    count_(count),
    type_(type)
{
    buffer_.clear();
    
    if (set_host_ip(ip) != 0)
    {
        throw("set_host_ip error!");
    }
    
    address_.port = port;
}

BaseFrame::~BaseFrame()
{
    buffer_.clear();
}

int BaseFrame::encode(uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;

    buffer_.clear();

    // ip
    uint8_t buffer_ip[FRAME_HOSTIP_SIZE] = {0};
    WriteUint32BE(reinterpret_cast<char *>(buffer_ip), this->address_.ip);
    for (int i=0; i<sizeof(buffer_ip); i++)
    {
        buffer_.push_back(buffer_ip[i]);
    }

    // port
    uint8_t buffer_port[FRAME_HOSTPORT_SIZE] = {0};
    WriteUint16BE(reinterpret_cast<char *>(buffer_port), this->address_.port);
    for (int i=0; i<sizeof(buffer_port); i++)
    {
        buffer_.push_back(buffer_port[i]);
    }

    // count 
    buffer_.push_back(count_);

    // type
    buffer_.push_back(static_cast<uint8_t>(type_));

    // signkey
    for (int i=0; i<signkey_.size(); i++)
    {
        buffer_.push_back(signkey_[i]);
    }

    for (size_t i=0; i<data_len; i++)
    {
        buffer_.push_back(data_ptr[i]);
    }

    return 0;
}

int BaseFrame::decode(uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;
    size_t offset = 0x00;

    if (FRAME_HOSTIP_SIZE > data_len)
    {
        LOG_ERROR("data frame decode failed, data_len:%d < %d", data_len, FRAME_HOSTIP_SIZE_);
        return -1;
    }

    address_.ip = ReadUint32BE(&data_ptr[FRAME_HOSTIP_OFFSET]);
    address_.port = ReadUint16BE(&data_ptr[FRAME_HOSTPORT_OFFSET]);
    count_ = data_ptr[FRAME_COUNT_OFFSET];
    type_ = data_ptr[FRAME_TYPE_OFFSET];
    memcpy(signkey_,data(), &data[FRAME_MSGDATA_OFFSET], signkey_.size());

    return 0;
}

// int BaseFrame::send(ENetPeer *peer_ptr, uint8_t chan)
// {
//     int ret = 0;

//     if (nullptr == peer_ptr)
//     {
//         LOG_ERROR("IM frame send failed, ret:%d", ret);
//         return -1;
//     }

//     ENetPacket *packet = enet_packet_create(NULL, buffer_.size(), ENET_PACKET_FLAG_RELIABLE);
//     if (nullptr == packet)
//     {
//         return -1;
//     }
    
//     memcpy(packet->data, buffer_.data(), buffer_.size());

//     ret = enet_peer_send(peer_ptr, chan, packet);
//     if (ret)
//     {
//         LOG_ERROR("enet_peer_send failed ret:%d", ret);
//         return ret;
//     }

//     return 0;
// }