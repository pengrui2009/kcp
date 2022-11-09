#ifndef REQUEST_DISCONNECT_FRAME_H
#define REQUEST_DISCONNECT_FRAME_H

#include "base_frame.h"
#include "pair_frame.h"

constexpr size_t REQUEST_DISCONNECT_FRAME_CONV_SIZE = 4;

class RequestDisconnectFrame : public PairFrame {
public:
    RequestDisconnectFrame(const std::string &signkey);
    RequestDisconnectFrame(const std::string &signkey, uint64_t timestamp, std::string &ip, 
        uint16_t port, uint8_t count);
    RequestDisconnectFrame(const std::string &signkey, uint64_t timestamp, std::string &ip, 
        uint16_t port, uint8_t count, const kcp_conv_t &conv);
    ~RequestDisconnectFrame();

    int encode() override;

    int encode(uint8_t *data_ptr, size_t data_len) override;

    int decode(const uint8_t *data_ptr, size_t data_len) override;

    int get_kcp_conv() const
    {
        return this->conv_;
    }
private:
    kcp_conv_t conv_;
};

#endif /* REQUEST_DISCONNECT_FRAME_H */