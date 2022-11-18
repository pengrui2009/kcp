#include "pair_frame.h"

#include "base_frame.h"
#include "logger.h"
#include <cstdint>
#include <cstring>
#include <stdexcept>

// std::array<uint8_t, 24> PairFrame::signkey_ = {0x11, 0x12, 0x13, 0x14};

PairFrame::PairFrame(const std::string &signkey) : 
    BaseFrame(signkey)
{
    
}

PairFrame::PairFrame(const std::string &signkey, uint64_t timestamp, std::string &ip, uint16_t port, 
    uint8_t count, MsgType type) :
    BaseFrame(signkey, timestamp, count, type)
{    
    if (set_host_ip(ip) != 0)
    {
        throw("set_host_ip error!");
    }
    
    if (set_host_ip(ip))
    {
        throw std::runtime_error("PairFrame set_host_ip error");
    }

    msg_address_.port = port;
}

PairFrame::PairFrame(const std::string &signkey, uint64_t timestamp, std::string &ip, uint16_t port, 
    uint8_t count, MsgType type, uint8_t *data_ptr, size_t data_len) : 
    BaseFrame(signkey, timestamp, count, type, data_ptr, data_len)    
{    
    if (set_host_ip(ip) != 0)
    {
        throw("set_host_ip error!");
    }
    
    if (set_host_ip(ip))
    {
        throw std::runtime_error("PairFrame set_host_ip error");
    }

    msg_address_.port = port;
}

PairFrame::~PairFrame()
{
    buffer_.clear();
}

int PairFrame::encode()
{
    int ret = 0;

    buffer_.clear();    
    
    // sync 
    buffer_.push_back(SYNC_BYTE_0);
    buffer_.push_back(SYNC_BYTE_1);
    buffer_.push_back(SYNC_BYTE_2);
    buffer_.push_back(SYNC_BYTE_3);

    // timestamp 
    uint8_t buffer_timestamp[PAIRFRAME_MSGTIMESTAMP_SIZE] = {0};
    WriteUint64BE(buffer_timestamp, msg_timestamp_);
    for (int i=0; i<sizeof(buffer_timestamp); i++)
    {
        buffer_.push_back(buffer_timestamp[i]);
    }

    // ip
    uint8_t buffer_ip[PAIRFRAME_MSGHOSTIP_SIZE] = {0};    
    WriteUint32BE(buffer_ip, this->msg_address_.host);
    for (int i=0; i<sizeof(buffer_ip); i++)
    {
        buffer_.push_back(buffer_ip[i]);
    }

    // port
    uint8_t buffer_port[PAIRFRAME_MSGHOSTPORT_SIZE] = {0};
    WriteUint16BE(buffer_port, this->msg_address_.port);
    for (int i=0; i<sizeof(buffer_port); i++)
    {
        buffer_.push_back(buffer_port[i]);
    }

    // count 
    buffer_.push_back(msg_count_);

    // type
    buffer_.push_back(static_cast<uint8_t>(msg_type_));

    // body len
    uint8_t buffer_bodylen[PAIRFRAME_MSGBODYLEN_SIZE] = {0};    
    WriteUint32BE(buffer_bodylen, this->body_len_);
    for (int i=0; i<sizeof(buffer_bodylen); i++)
    {
        buffer_.push_back(buffer_bodylen[i]);
    }

    // body data
    for (size_t i=0; i<body_data_.size(); i++)
    {
        buffer_.push_back(body_data_[i]);
    }

    ret = generate_signature(buffer_.data(), buffer_.size());
    if (ret)
    {
        LOG_ERROR("generate signature failed.");
        return -1;
    }

    // signdata
    for (int i=0; i< msg_signdata_.size(); i++)
    {
        buffer_.push_back(msg_signdata_[i]);
    }    

    return 0;
}

int PairFrame::encode(uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;

    buffer_.clear();
    body_data_.clear();

    if (data_len)
    {
        body_data_.resize(data_len);
        memcpy(body_data_.data(), data_ptr, data_len);
        body_len_ = data_len;
    }
    
    ret = encode();
    if (ret)
    {
        LOG_ERROR("encode failed.");
        return -1;
    }    

    return 0;
}

int PairFrame::decode(const uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;
    size_t offset = 0x00;

    if (PAIRFRAME_MIN_SIZE > data_len)
    {
        LOG_ERROR("pair frame decode failed, len:%d < %d", data_len, PAIRFRAME_MIN_SIZE);
        return -1;
    }

    if ((SYNC_BYTE_0 != data_ptr[0]) || (SYNC_BYTE_1 != data_ptr[1]) ||
        (SYNC_BYTE_2 != data_ptr[2]) || (SYNC_BYTE_3 != data_ptr[3]))
    {
        LOG_ERROR("sync error, %02X %02X %02X %02X", data_ptr[0], data_ptr[1], data_ptr[2], data_ptr[3]);
        return -1;
    }

    msg_timestamp_ = ReadUint64BE(&data_ptr[PAIRFRAME_MSGTIMESTAMP_OFFSET]);
    msg_address_.host = ReadUint32BE(&data_ptr[PAIRFRAME_MSGHOSTIP_OFFSET]);
    msg_address_.port = ReadUint16BE(&data_ptr[PAIRFRAME_MSGHOSTPORT_OFFSET]);
    msg_count_ = data_ptr[PAIRFRAME_MSGCOUNT_OFFSET];
    msg_type_ = static_cast<MsgType>(data_ptr[PAIRFRAME_MSGTYPE_OFFSET]);
    body_len_ = ReadUint32BE(&data_ptr[PAIRFRAME_MSGBODYLEN_OFFSET]);
    
    if (data_len != (body_len_+PAIRFRAME_MIN_SIZE))
    {
        LOG_ERROR("length is invalid, %d != %d", (body_len_+PAIRFRAME_MIN_SIZE), data_len);
        return -1;
    }
    body_data_.resize(body_len_);
    memcpy(body_data_.data(), &data_ptr[PAIRFRAME_MSGBODYDATA_OFFSET], body_len_);
    memcpy(msg_signdata_.data(), &data_ptr[PAIRFRAME_MSGBODYDATA_OFFSET + body_len_], PAIRFRAME_MSGSIGDATA_SIZE);

    if (!verify_signature(data_ptr, (data_len - PAIRFRAME_MSGSIGDATA_SIZE)))
    {
        LOG_ERROR("verify_signature failed.");
        return -1;
    }
    
    return 0;
}
