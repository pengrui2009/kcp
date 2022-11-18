#include "msg_frame.h"

#include "base_frame.h"
#include "logger.h"
#include <cstdint>

// std::array<uint8_t, 24> MsgFrame::signkey_ = {0x11, 0x12, 0x13, 0x14};

MsgFrame::MsgFrame(const std::string &signkey) : 
    BaseFrame(signkey),
    seqnum_(0)
{
    msg_type_ = MSGTYPE_MSGDATA;

    buffer_.clear();
    body_data_.clear();
    body_len_ = 0;
}

MsgFrame::MsgFrame(const std::string &signkey, uint64_t msg_timestamp, uint8_t msg_count, 
    uint8_t seqnum, std::vector<uint8_t> &priv_data) :
    BaseFrame(signkey, msg_timestamp, msg_count, MSGTYPE_MSGDATA),
    priv_data_(priv_data)
    
{
    buffer_.clear();
    body_data_.clear();
    body_len_ = 0;
}

MsgFrame::~MsgFrame()
{
    buffer_.clear();
    body_data_.clear();
    body_len_ = 0;
}

int MsgFrame::encode()
{
    int ret = 0;

    buffer_.clear();    
    body_data_.clear();
    body_len_ = MSGFRAME_BODYDATA_SEQNUM_SIZE + priv_data_.size();

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
    buffer_.push_back(seqnum_);

    for(int i=0; i<priv_data_.size(); i++)
    {
        buffer_.push_back(priv_data_[i]);
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

int MsgFrame::encode(uint8_t *data_ptr, size_t data_len)
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

int MsgFrame::encode(uint64_t msg_timestamp, uint8_t msg_count,  
    uint8_t seqnum, std::vector<uint8_t> &priv_data)
{
    int ret = 0;

    this->msg_timestamp_ = msg_timestamp;
    this->msg_count_ = msg_count;
    this->seqnum_ = seqnum;
    this->priv_data_ = priv_data;

    ret = encode();
    if (ret)
    {
        LOG_ERROR("encode failed.");
        return -1;
    }    

    
    return 0;
}

int MsgFrame::decode(const uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;
    size_t offset = 0x00;

    if (MSGFRAME_MIN_SIZE > data_len)
    {
        LOG_ERROR("Msg frame decode failed, len:%d < %d", data_len, MSGFRAME_MIN_SIZE);
        return -1;
    }

    if ((SYNC_BYTE_0 != data_ptr[0]) || (SYNC_BYTE_1 != data_ptr[1]) ||
        (SYNC_BYTE_2 != data_ptr[2]) || (SYNC_BYTE_3 != data_ptr[3]))
    {
        LOG_ERROR("sync error, %02X %02X %02X %02X", data_ptr[0], data_ptr[1], data_ptr[2], data_ptr[3]);
        return -1;
    }

    msg_timestamp_ = ReadUint64BE(&data_ptr[MSGFRAME_MSGTIMESTAMP_OFFSET]); 
    msg_count_ = data_ptr[MSGFRAME_MSGCOUNT_OFFSET];
    msg_type_ = static_cast<MsgType>(data_ptr[MSGFRAME_MSGTYPE_OFFSET]);
    body_len_ = ReadUint32BE(&data_ptr[MSGFRAME_MSGBODYLEN_OFFSET]);

    if (data_len < (body_len_+ MSGFRAME_MIN_SIZE))
    {
        LOG_ERROR("length is invalid, %d < %d", (body_len_+ MSGFRAME_MIN_SIZE), data_len);
        return -1;
    }
    body_data_.resize(body_len_);
    memcpy(body_data_.data(), &data_ptr[MSGFRAME_MSGBODYDATA_OFFSET], body_len_);
    memcpy(msg_signdata_.data(), &data_ptr[MSGFRAME_MSGBODYDATA_OFFSET + body_len_], FRAME_MSGSIGNDATA_SIZE);

    if (!verify_signature(data_ptr, (data_len - FRAME_MSGSIGNDATA_SIZE)))
    {
        LOG_ERROR("verify_signature failed.");
        return -1;
    }
    
    if (body_data_.size() < MSGFRAME_BODYDATA_SEQNUM_SIZE)
    {
        LOG_ERROR("data length is not enought %d < %d", body_data_.size(), MSGFRAME_BODYDATA_SEQNUM_SIZE);
        return -1;
    }
    
    seqnum_ = body_data_[MSGFRAME_BODYDATA_SEQNUM_OFFSET];

    for (int i=1; i<body_data_.size(); i++)
    {
        priv_data_.push_back(body_data_[i]);
    }

    return 0;
}
