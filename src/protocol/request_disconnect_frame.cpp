#include "request_disconnect_frame.h"
#include "base_frame.h"
#include "pair_frame.h"

RequestDisconnectFrame::RequestDisconnectFrame(const std::string &signkey) :
    PairFrame(signkey)
{

}

RequestDisconnectFrame::RequestDisconnectFrame(const std::string &signkey, uint64_t timestamp, 
    std::string &ip, uint16_t port, uint8_t count) :
    PairFrame(signkey, timestamp, ip, port, count, MSGTYPE_REQUEST_DISCONNECT)
{

}

RequestDisconnectFrame::RequestDisconnectFrame(const std::string &signkey, uint64_t timestamp, std::string &ip, 
    uint16_t port, uint8_t count, const kcp_conv_t &conv) :
    PairFrame(signkey, timestamp, ip, port, count, MSGTYPE_REQUEST_DISCONNECT),
    conv_(conv)
{

}

RequestDisconnectFrame::~RequestDisconnectFrame()
{

}


int RequestDisconnectFrame::encode()
{
    int ret = 0;

    if (body_data_.empty())
    {
        uint8_t buffer_conv[REQUEST_DISCONNECT_FRAME_CONV_SIZE] = {0};
        WriteUint32BE(buffer_conv, conv_);
        for (int i=0; i<sizeof(buffer_conv); i++)
        {
            body_data_.push_back(buffer_conv[i]);
        }
    }

    ret = PairFrame::encode();
    if (ret)
    {
        LOG_ERROR("Request Disconnect Frame encode failed.");
        return -1;
    }

    return 0;
}

int RequestDisconnectFrame::encode(uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;

    // ret = PairFrame::encode();
    return 0;
}

int RequestDisconnectFrame::decode(const uint8_t *data_ptr, size_t data_len)
{
    int ret = 0;

    ret = PairFrame::decode(data_ptr, data_len);
    if (ret)
    {
        LOG_ERROR("Request Disconnect Frame decode failed.");
        return -1;
    }

    if (body_data_.size() >= REQUEST_DISCONNECT_FRAME_CONV_SIZE)
    {
        conv_ = ReadUint32BE(body_data_.data());
    }

    return 0;
}