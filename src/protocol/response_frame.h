#ifndef RESPONSE_FRAME_H
#define RESPONSE_FRAME_H

#include "pair_frame.h"
#include "typedef.h"

constexpr size_t RESPONSEFRAME_CONV_SIZE = 4;

class ResponseFrame : public PairFrame {
public:
    ResponseFrame(const std::string &signkey);
    ResponseFrame(const std::string &signkey, uint64_t timestamp, std::string &ip, uint16_t port, 
        uint8_t count, MsgType type, const kcp_conv_t &conv);
    ~ResponseFrame();

    int encode() override;

    int encode(uint8_t *data_ptr, size_t data_len) override;

    int decode(const uint8_t *data_ptr, size_t data_len) override;

    int get_kcp_conv() const
    {
        return this->conv_;
    }
private:
    // elements of frame
    kcp_conv_t conv_;
};

#endif /* RESPONSE_FRAME_H */
