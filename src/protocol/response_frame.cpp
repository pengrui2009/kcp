#include "response_frame.h"
#include "base_frame.h"
#include "pair_frame.h"
#include <cstdint>

ResponseFrame::ResponseFrame(const std::string &signkey) :
    PairFrame(signkey)
{

}

ResponseFrame::ResponseFrame(const std::string &signkey, uint64_t timestamp, std::string &ip, 
    uint16_t port, uint8_t count, MsgType type, const kcp_conv_t &conv) :
    PairFrame(signkey, timestamp, ip, port, count, type),
    conv_(conv)
{
    
}

ResponseFrame::~ResponseFrame()
{

}

int ResponseFrame::encode()
{
    int ret = 0;

    if (body_data_.empty())
    {
        uint8_t buffer_conv[RESPONSEFRAME_CONV_SIZE] = {0};
        WriteUint32BE(buffer_conv, conv_);
        for (int i=0; i<sizeof(buffer_conv); i++)
        {
            body_data_.push_back(buffer_conv[i]);
        }
        body_len_ = body_data_.size();
    }

    ret = PairFrame::encode();
    if (ret)
    {
        LOG_ERROR("Response Frame encode failed.");
        return -1;
    }

    return 0;
}

int ResponseFrame::encode(uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;

    // ret = PairFrame::encode();
    return 0;
}

int ResponseFrame::decode(const uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;

    ret = PairFrame::decode(data_ptr, data_len);
    if (ret)
    {
        LOG_ERROR("Response Frame decode failed.");
        return -1;
    }

    if (body_data_.size() >= RESPONSEFRAME_CONV_SIZE)
    {
        conv_ = ReadUint32BE(body_data_.data());
    }

    return 0;
}