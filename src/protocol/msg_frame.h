#ifndef MSG_FRAME_H
#define MSG_FRAME_H

#include "base_frame.h"
#include "typedef.h"
#include "logger.h"

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>

/* Save all the compiler settings. */
// #pragma pack(push)
// #pragma pack(1)

/* restore all compiler settings in stacks. */
// #pragma pack(pop)
constexpr size_t MSGFRAME_MSGTIMESTAMP_OFFSET = 4;
constexpr size_t MSGFRAME_MSGCOUNT_OFFSET = 12;
constexpr size_t MSGFRAME_MSGTYPE_OFFSET = 13;
constexpr size_t MSGFRAME_MSGBODYLEN_OFFSET = 14;
constexpr size_t MSGFRAME_MSGBODYDATA_OFFSET = 18;

constexpr size_t MSGFRAME_BODYDATA_SEQNUM_OFFSET = 0;
constexpr size_t MSGFRAME_BODYDATA_PRIVDATA_OFFSET = 4;

constexpr size_t MSGFRAME_BODYDATA_SEQNUM_SIZE = 4;

constexpr size_t MSGFRAME_MIN_SIZE = 50;

class MsgFrame : public BaseFrame {
public:
    friend class KcpServer;
    friend class KcpClient;

    MsgFrame(const std::string &signkey);
    MsgFrame(const std::string &signkey, uint64_t msg_timestamp, uint8_t msg_count, 
        uint8_t seqnum, std::vector<uint8_t> &priv_data);
    ~MsgFrame();

    int encode();

    int encode(uint8_t *data_ptr, size_t data_len);

    int encode(uint64_t msg_timestamp, uint8_t msg_count, uint32_t seqnum, 
        std::vector<uint8_t> &priv_data);

    int decode(const uint8_t *data_ptr, size_t data_len);
    
    std::vector<uint8_t> &get_buffer()
    {
        return this->buffer_;
    }

    uint32_t get_seqnum() const
    {
        return this->seqnum_;
    }
protected:
    // data buffer of frame

    // elements of frame
    uint32_t seqnum_;

    std::vector<uint8_t> priv_data_;
};


#endif /* MSG_FRAME_H */