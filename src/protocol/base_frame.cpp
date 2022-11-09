#include "base_frame.h"

#include "logger.h"

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

#include <cstddef>
#include <cstdint>

// std::array<uint8_t, 24> BaseFrame::signkey_ = {0x11, 0x12, 0x13, 0x14};

uint64_t BaseFrame::ReadUint64BE(const uint8_t* data)
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

uint64_t BaseFrame::ReadUint64LE(const uint8_t* data)
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

uint32_t BaseFrame::ReadUint32BE(const uint8_t* data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = static_cast<uint32_t>(static_cast<uint32_t>(p[0]) << 24) | \
                     static_cast<uint32_t>(static_cast<uint32_t>(p[1]) << 16) | \
                     static_cast<uint32_t>(static_cast<uint32_t>(p[2]) << 8)  | \
                     static_cast<uint32_t>(p[3]);
    return value;
}

uint32_t BaseFrame::ReadUint32LE(const uint8_t* data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = static_cast<uint32_t>(static_cast<uint32_t>(p[3]) << 24) | \
                     static_cast<uint32_t>(static_cast<uint32_t>(p[2]) << 16) | \
                     static_cast<uint32_t>(static_cast<uint32_t>(p[1]) << 8)  | \
                     static_cast<uint32_t>(p[0]);
    return value;
}

uint32_t BaseFrame::ReadUint24BE(const uint8_t* data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = static_cast<uint32_t>(static_cast<uint32_t>(p[0]) << 16) | \
                     static_cast<uint32_t>(static_cast<uint32_t>(p[1]) << 8)  | \
                     static_cast<uint32_t>(p[2]);
    return value;
}

uint32_t BaseFrame::ReadUint24LE(const uint8_t* data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = static_cast<uint32_t>(static_cast<uint32_t>(p[2]) << 16) | \
                     static_cast<uint32_t>(static_cast<uint32_t>(p[1]) << 8)  | \
                     static_cast<uint32_t>(p[0]);
    return value;
}

uint16_t BaseFrame::ReadUint16BE(const uint8_t* data)
{
    uint8_t* p = (uint8_t*)data;
    uint16_t value = static_cast<uint16_t>(static_cast<uint16_t>(p[0]) << 8) | \
                     static_cast<uint16_t>(p[1]);
    return value; 
}

uint16_t BaseFrame::ReadUint16LE(const uint8_t* data)
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

BaseFrame::BaseFrame(const std::string &signkey) : 
    msg_type_(MSGTYPE_UNKNOWN),
    msg_count_(0)
{
    buffer_.clear();
    body_data_.clear();
    body_len_ = 0;
}

BaseFrame::BaseFrame(const std::string &signkey, uint64_t timestamp, 
    uint8_t count, MsgType type) : 
    msg_count_(count),
    msg_type_(type)
{
    buffer_.clear();
    body_data_.clear();
    body_len_ = 0;
}

BaseFrame::BaseFrame(const std::string &signkey, uint64_t timestamp, 
    uint8_t count, MsgType type, uint8_t *data_ptr, size_t data_len) : 
    msg_count_(count),
    msg_type_(type)
{
    buffer_.clear();
    body_data_.clear();
    body_len_ = 0;
    
    if (data_len)
    {
        body_data_.resize(data_len);
        memcpy(body_data_.data(), data_ptr, data_len);
        body_len_ = data_len;
    }
}

BaseFrame::~BaseFrame()
{
    buffer_.clear();
}

// int BaseFrame::generate_signature()
// {
//     uint32_t signature_data_size = 0;
//     std::array<unsigned char, EVP_MAX_MD_SIZE> signature_data;

//     HMAC(EVP_sha256(), reinterpret_cast<const void*>(signature_key_.c_str()), static_cast<int>(signature_key_.size()),
//         body_data_.data(), body_data_.size(), signature_data.data(), &signature_data_size);

//     memcpy(msg_signdata_.data(), signature_data.data(), signature_data_size);
// }
/**
 * @brief calculate HMAC SHA256 data
 * 
 * @param data_ptr 
 * @param data_len 
 * @return int 
 */
int BaseFrame::generate_signature(uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;

    uint32_t signature_data_size = 0;
    std::array<unsigned char, EVP_MAX_MD_SIZE> signature_data;

    HMAC(EVP_sha256(), reinterpret_cast<const void*>(signature_key_.c_str()), static_cast<int>(signature_key_.size()),
        data_ptr, data_len, signature_data.data(), &signature_data_size);

    memcpy(msg_signdata_.data(), signature_data.data(), signature_data_size);

    return 0;
}

bool BaseFrame::verify_signature(const uint8_t *data_ptr, size_t data_len)
{
    uint32_t signature_data_size = 0;
    std::array<unsigned char, EVP_MAX_MD_SIZE> signature_data;

    HMAC(EVP_sha256(), reinterpret_cast<const void*>(signature_key_.c_str()), static_cast<int>(signature_key_.size()),
         data_ptr, data_len, signature_data.data(), &signature_data_size);
    
    if (std::equal(std::begin(msg_signdata_), std::end(msg_signdata_), std::begin(signature_data))) {
        return true;
    } else {
        return false;
    }
}