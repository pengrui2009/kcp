#ifndef HEARTBEAT_FRAME_H
#define HEARTBEAT_FRAME_H

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


// enum FrameType : std::uint8_t 
// {
//     FRAMETYPE_UNKNOWN = 0,
//     FRAMETYPE_REQUEST_CONNECT,
//     FRAMETYPE_CONNECTED,
//     FRAMETYPE_REQUEST_DISCONNECT,
//     FRAMETYPE_DISCONNECTED,
//     FRAMETYPE_HEARTBEAT,
//     FRAMETYPE_MSGDATA = 0x55,
//     FRAMETYPE_END
// };

/* restore all compiler settings in stacks. */
// #pragma pack(pop)
constexpr size_t HEARTBEATFRAME_MSGTIMESTAMP_OFFSET = 4;
constexpr size_t HEARTBEATFRAME_MSGCOUNT_OFFSET = 12;
constexpr size_t HEARTBEATFRAME_MSGTYPE_OFFSET = 13;
constexpr size_t HEARTBEATFRAME_MSGBODYLEN_OFFSET = 14;
constexpr size_t HEARTBEATFRAME_MSGBODYDATA_OFFSET = 18;
// body data
constexpr size_t HEARTBEATFRAME_BODY_TIMESTAMP_OFFSET = 0;
constexpr size_t HEARTBEATFRAME_BODY_COUNT_OFFSET = 8;

constexpr size_t HEARTBEATFRAME_BODY_TIMESTAMP_SIZE = 8;
constexpr size_t HEARTBEATFRAME_BODY_COUNT_SIZE = 1;
constexpr size_t HEARTBEATFRAME_BODY_SIZE = 9;

constexpr size_t HEARTBEATFRAME_MIN_SIZE = 50;

class HeartbeatFrame : BaseFrame {
public:
    friend class KcpServer;
    friend class KcpClient;

    HeartbeatFrame(const std::string &signkey);
    HeartbeatFrame(const std::string &signkey, uint64_t msg_timestamp, 
        uint8_t msg_count, uint64_t timestamp, uint8_t count);
    ~HeartbeatFrame();

    int encode() override;

    int encode(uint8_t *data_ptr, size_t data_len) override;

    int decode(const uint8_t *data_ptr, size_t data_len) override;
    
protected:
    // data buffer of frame

    // elements of frame
    uint64_t timestamp_;

    uint8_t count_;
};


#endif /* HEARTBEAT_FRAME_H */