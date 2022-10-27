#ifndef BASE_FRAME_H
#define BASE_FRAME_H

#include "typedef.h"

#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>

/* Save all the compiler settings. */
// #pragma pack(push)
// #pragma pack(1)
class UKcp;

enum FrameType : std::uint8_t 
{
    FRAMETYPE_UNKNOWN = 0,
    FRAMETYPE_REQUEST_CONNECT,
    FRAMETYPE_CONNECTED,
    FRAMETYPE_REQUEST_DISCONNECT,
    FRAMETYPE_DISCONNECTED,
    FRAMETYPE_HEARTBEAT,
    FRAMETYPE_MSGDATA = 0x55,
    FRAMETYPE_END
};

/* restore all compiler settings in stacks. */
// #pragma pack(pop)
struct UkcpNetAddress {
    uint32_t host;
    uint16_t port;
};

constexpr size_t FRAME_HOSTIP_OFFSET = 0;
constexpr size_t FRAME_HOSTPORT_OFFSET = 4;
constexpr size_t FRAME_COUNT_OFFSET = 6;
constexpr size_t FRAME_TYPE_OFFSET = 7;
constexpr size_t FRAME_SIGNKEY_OFFSET = 8;
constexpr size_t FRAME_MSGDATA_OFFSET = 40;

constexpr size_t FRAME_HOSTIP_SIZE = 4;
constexpr size_t FRAME_HOSTPORT_SIZE = 2;
constexpr size_t FRAME_TYPE_SIZE = 1;
constexpr size_t FRAME_COUNT_SIZE = 1;
constexpr size_t FRAME_SIGKEY_SIZE = 32;

constexpr size_t FRAME_HEAD_SIZE = (FRAME_HOSTIP_SIZE + FRAME_HOSTPORT_SIZE + 
    FRAME_TYPE_SIZE + FRAME_COUNT_SIZE + FRAME_SIGKEY_SIZE);

class BaseFrame {
public:
    friend class UKcp;

    BaseFrame();
    BaseFrame(std::string &ip, uint16_t port, FrameType frame_type, uint8_t count);
    ~BaseFrame();

    virtual int encode()=0;
    virtual int decode(uint8_t *data_ptr, size_t data_len)=0;

    uint64_t ReadUint64BE(uint8_t* data);
    uint64_t ReadUint64LE(uint8_t* data);
    uint32_t ReadUint32BE(uint8_t* data);
    uint32_t ReadUint32LE(uint8_t* data);
    uint32_t ReadUint24BE(uint8_t* data);
    uint32_t ReadUint24LE(uint8_t* data);
    uint16_t ReadUint16BE(uint8_t* data);
    uint16_t ReadUint16LE(uint8_t* data);

    void WriteUint64BE(uint8_t* p, uint64_t value);
    void WriteUint64LE(uint8_t* p, uint64_t value);
    void WriteUint32BE(uint8_t* p, uint32_t value);
    void WriteUint32LE(uint8_t* p, uint32_t value);
    void WriteUint24BE(uint8_t* p, uint32_t value);
    void WriteUint24LE(uint8_t* p, uint32_t value);
    void WriteUint16BE(uint8_t* p, uint16_t value);
    void WriteUint16LE(uint8_t* p, uint16_t value);

    // int send(ENetPeer *peer, uint8_t chan);

    int set_host_ip(const std::string &ip)
    {
        unsigned long addr = 0x00;
    #ifdef HAS_INET_PTON
        if (! inet_pton (AF_INET, name, &add))
    #else
        if (! inet_aton (ip.c_str()), (struct in_addr *)(&addr))
    #endif
            return -1;
        address_.host = addr;
        return 0;
    }

    int set_host_port(const uint16_t port) 
    {
        address_.port = port;
        return 0;
    }

    int get_host_ip(std::string &ip) const
    {
        unsigned long addr = address_.host;
    #ifdef HAS_INET_NTOP
        if (inet_ntop (AF_INET, & address -> host, name, nameLength) == NULL)
    #else
        char * address = inet_ntoa(*(struct in_addr *)(&addr));
        if (address != NULL)
        {
            size_t addrlen = strlen(address);
            if (addrlen <= 0)
                return -1;
            ip = address;
        } 
        else
    #endif
            return -1;
        return 0;
    }

    int get_host_port(uint16_t &port) const
    {
        return  address_.port;
    }

        FrameType get_frametype() const
    {
        return this->type_;
    }

    uint8_t get_msgcount() const
    {
        return this->count_;
    }

    std::array<uint8_t, 32> get_signkey() const
    {
        return this->signkey_;
    }

    void set_signkey(std::array<uint8_t, 32> &signkey)
    {
        this->signkey_ = signkey;
    }

    int encode(uint8_t *data_ptr, size_t data_len);

    int decode(uint8_t *data_ptr, size_t data_len);
    
protected:
    std::vector<uint8_t> buffer_;

    UkcpNetAddress address_;
    uint8_t count_;
    FrameType type_;
    std::array<uint8_t, 32> signkey_;


};


#endif /* IM_BASE_FRAME_H */