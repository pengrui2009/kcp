#ifndef BASE_FRAME_H
#define BASE_FRAME_H

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


enum MsgType : std::uint8_t 
{
    MSGTYPE_UNKNOWN = 0,
    MSGTYPE_REQUEST_CONNECT = 0x10,
    MSGTYPE_CONNECTED,
    MSGTYPE_REQUEST_DISCONNECT,
    MSGTYPE_DISCONNECTED,
    MSGTYPE_HEARTBEAT = 20,
    MSGTYPE_MSGDATA = 0x30,
    MSGTYPE_END
};

/* restore all compiler settings in stacks. */
// #pragma pack(pop)
struct UkcpNetAddress {
    uint32_t host;
    uint16_t port;
};

// kcp frame header size
constexpr size_t KCP_FRAME_SIZE = 24;
constexpr size_t KCP_SYNC_SIZE = 4;
constexpr size_t KCP_CONV_SIZE = 4;

constexpr uint8_t SYNC_BYTE_0 = 0x55;
constexpr uint8_t SYNC_BYTE_1 = 0x55;
constexpr uint8_t SYNC_BYTE_2 = 0x55;
constexpr uint8_t SYNC_BYTE_3 = 0x55;

constexpr size_t FRAME_MSGTIMESTAMP_OFFSET = 4;

constexpr size_t FRAME_MSGSYNC_SIZE = 4;
constexpr size_t FRAME_MSGTIMESTAMP_SIZE = 8;
constexpr size_t FRAME_MSGCOUNT_SIZE = 1;
constexpr size_t FRAME_MSGTYPE_SIZE = 1;
constexpr size_t FRAME_MSGBODYLEN_SIZE = 4;
constexpr size_t FRAME_MSGSIGNDATA_SIZE = 32;
// constexpr size_t FRAME_MINIMUM_SIZE = 44;

// constexpr size_t FRAME_HEAD_SIZE = (FRAME_HOSTIP_SIZE + FRAME_HOSTPORT_SIZE + 
//     FRAME_TYPE_SIZE + FRAME_COUNT_SIZE + FRAME_SIGKEY_SIZE);

class BaseFrame {
public:

    BaseFrame(const std::string &signkey);
    BaseFrame(const std::string &signkey, uint64_t timestamp, uint8_t count, 
        MsgType type);
    BaseFrame(const std::string &signkey, uint64_t timestamp, uint8_t count, 
        MsgType type, uint8_t *data_ptr, size_t data_len);
    ~BaseFrame();

    uint64_t ReadUint64BE(const uint8_t* data);
    uint64_t ReadUint64LE(const uint8_t* data);
    uint32_t ReadUint32BE(const uint8_t* data);
    uint32_t ReadUint32LE(const uint8_t* data);
    uint32_t ReadUint24BE(const uint8_t* data);
    uint32_t ReadUint24LE(const uint8_t* data);
    uint16_t ReadUint16BE(const uint8_t* data);
    uint16_t ReadUint16LE(const uint8_t* data);

    void WriteUint64BE(uint8_t* p, uint64_t value);
    void WriteUint64LE(uint8_t* p, uint64_t value);
    void WriteUint32BE(uint8_t* p, uint32_t value);
    void WriteUint32LE(uint8_t* p, uint32_t value);
    void WriteUint24BE(uint8_t* p, uint32_t value);
    void WriteUint24LE(uint8_t* p, uint32_t value);
    void WriteUint16BE(uint8_t* p, uint16_t value);
    void WriteUint16LE(uint8_t* p, uint16_t value);

    // int send(ENetPeer *peer, uint8_t chan);

    // int set_host_ip(const std::string &ip)
    // {
    //     unsigned long addr = 0x00;
    // #ifdef HAS_INET_PTON
    //     if (! inet_pton (AF_INET, name, &add))
    // #else
    //     if (! inet_aton (ip.c_str(), (struct in_addr *)(&addr)))
    // #endif
    //         return -1;
        
    //     address_.host = addr;
    //     return 0;
    // }

    // int set_host_port(const uint16_t port) 
    // {
    //     address_.port = port;
    //     return 0;
    // }

    // int get_host_ip(std::string &ip) const
    // {
    //     unsigned long addr = address_.host;
    // #ifdef HAS_INET_NTOP
    //     if (inet_ntop (AF_INET, & address -> host, name, nameLength) == NULL)
    // #else
    //     char *address = inet_ntoa(*(struct in_addr *)(&addr));
    //     if (address != NULL)
    //     {
    //         size_t addrlen = strlen(address);
    //         if (addrlen <= 0)
    //             return -1;
    //         ip = address;
    //     } 
    //     else
    // #endif
    //         return -1;
    //     return 0;
    // }

    // int get_host_port(uint16_t &port) const
    // {
    //     port = address_.port;
    //     return 0;
    // }
    uint64_t get_msgtimestamp() const
    {
        return this->msg_timestamp_;
    }

    uint8_t get_msgcount() const
    {
        return this->msg_count_;
    }

    MsgType get_msgtype() const
    {
        return this->msg_type_;
    }

    uint32_t get_msglen() const
    {
        return this->body_len_;
    }

    virtual int encode() = 0;

    virtual int encode(uint8_t *data_ptr, size_t data_len) = 0;

    virtual int decode(const uint8_t *data_ptr, size_t data_len) = 0;

    // int generate_signature();
    
    int generate_signature(uint8_t *data_ptr, size_t data_len);

    bool verify_signature(const uint8_t *data_ptr, size_t data_len);
    
protected:
    // signature key
    std::string signature_key_;
    // data buffer of frame
    std::vector<uint8_t> buffer_;

    // elements of frame
    uint8_t msg_sync0_;
    uint8_t msg_sync1_;
    uint8_t msg_sync2_;
    uint8_t msg_sync3_;
    uint64_t msg_timestamp_;
    
    uint8_t msg_count_;
    MsgType msg_type_;
    uint32_t body_len_;
    std::vector<uint8_t> body_data_;
    std::array<uint8_t, FRAME_MSGSIGNDATA_SIZE> msg_signdata_;
};


#endif /* IM_BASE_FRAME_H */