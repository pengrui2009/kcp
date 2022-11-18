#include "heartbeat_frame.h"

#include "base_frame.h"
#include "heartbeat_frame.h"
#include "logger.h"
#include <cstdint>

// std::array<uint8_t, 24> BaseFrame::signkey_ = {0x11, 0x12, 0x13, 0x14};

HeartbeatFrame::HeartbeatFrame(const std::string &signkey) : 
    BaseFrame(signkey),
    count_(0)
{
    buffer_.clear();
    body_data_.clear();
    body_len_ = 0;
}

HeartbeatFrame::HeartbeatFrame(const std::string &signkey, uint64_t msg_timestamp, uint8_t msg_count, 
    uint64_t timestamp, uint8_t count) : 
    BaseFrame(signkey, msg_timestamp, msg_count, MSGTYPE_HEARTBEAT),
    timestamp_(timestamp), 
    count_(count)
{
    buffer_.clear();
    
    body_data_.clear();
    body_len_ = 0;
}

HeartbeatFrame::~HeartbeatFrame()
{
    buffer_.clear();
}

int HeartbeatFrame::encode()
{
    int ret = 0;

    buffer_.clear();    
    
    // sync 
    buffer_.push_back(SYNC_BYTE_0);
    buffer_.push_back(SYNC_BYTE_1);
    buffer_.push_back(SYNC_BYTE_2);
    buffer_.push_back(SYNC_BYTE_3);

    // timestamp 
    uint8_t buffer_timestamp[FRAME_MSGTIMESTAMP_SIZE] = {0};
    WriteUint64BE(buffer_timestamp, msg_timestamp_);
    for (int i=0; i<sizeof(buffer_timestamp); i++)
    {
        buffer_.push_back(buffer_timestamp[i]);
    }

    // count 
    buffer_.push_back(msg_count_);

    // type
    buffer_.push_back(static_cast<uint8_t>(msg_type_));

    // body len
    uint8_t buffer_bodylen[FRAME_MSGBODYLEN_SIZE] = {0};    
    WriteUint32BE(buffer_bodylen, this->body_len_);
    for (int i=0; i<sizeof(buffer_bodylen); i++)
    {
        buffer_.push_back(buffer_bodylen[i]);
    }

    // body data
    // for (size_t i=0; i<body_data_.size(); i++)
    // {
    //     buffer_.push_back(body_data_[i]);
    // }
    // body data
    uint8_t buffer_bodytimestamp[HEARTBEATFRAME_BODY_TIMESTAMP_SIZE] = {0};    
    WriteUint64BE(buffer_bodylen, this->timestamp_);
    for (int i=0; i<sizeof(buffer_bodytimestamp); i++)
    {
        buffer_.push_back(buffer_bodytimestamp[i]);
    }

    buffer_.push_back(count_);

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

int HeartbeatFrame::encode(uint8_t *data_ptr, size_t data_len)
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

int HeartbeatFrame::decode(const uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;
    size_t offset = 0x00;

    if (HEARTBEATFRAME_MIN_SIZE > data_len)
    {
        LOG_ERROR("Heartbeat frame decode failed, len:%d < %d", data_len, HEARTBEATFRAME_MIN_SIZE);
        return -1;
    }

    if ((SYNC_BYTE_0 != data_ptr[0]) || (SYNC_BYTE_1 != data_ptr[1]) ||
        (SYNC_BYTE_2 != data_ptr[2]) || (SYNC_BYTE_3 != data_ptr[3]))
    {
        LOG_ERROR("sync error, %02X %02X %02X %02X", data_ptr[0], data_ptr[1], data_ptr[2], data_ptr[3]);
        return -1;
    }

    msg_timestamp_ = ReadUint64BE(&data_ptr[HEARTBEATFRAME_MSGTIMESTAMP_OFFSET]);
    msg_count_ = data_ptr[HEARTBEATFRAME_MSGCOUNT_OFFSET];
    msg_type_ = static_cast<MsgType>(data_ptr[HEARTBEATFRAME_MSGTYPE_OFFSET]);
    body_len_ = ReadUint32BE(&data_ptr[HEARTBEATFRAME_MSGBODYLEN_OFFSET]);
    
    if (data_len != (body_len_+ HEARTBEATFRAME_MIN_SIZE))
    {
        LOG_ERROR("length is invalid, %d != %d", (body_len_+HEARTBEATFRAME_MIN_SIZE), data_len);
        return -1;
    }
    body_data_.resize(body_len_);
    memcpy(body_data_.data(), &data_ptr[HEARTBEATFRAME_MSGBODYDATA_OFFSET], body_len_);
    memcpy(msg_signdata_.data(), &data_ptr[HEARTBEATFRAME_MSGBODYDATA_OFFSET + body_len_], FRAME_MSGSIGNDATA_SIZE);

    if (!verify_signature(data_ptr, (data_len - FRAME_MSGSIGNDATA_SIZE)))
    {
        LOG_ERROR("verify_signature failed.");
        return -1;
    }

    if (body_data_.size() != HEARTBEATFRAME_BODY_SIZE)
    {
        LOG_ERROR("data length is not enought %d < %d.", body_data_.size(), HEARTBEATFRAME_BODY_SIZE);
        return -1;
    }

    msg_timestamp_ = ReadUint64BE(&body_data_[HEARTBEATFRAME_BODY_TIMESTAMP_OFFSET]);
    count_ = body_data_[HEARTBEATFRAME_BODY_COUNT_OFFSET];

    
    return 0;
}
