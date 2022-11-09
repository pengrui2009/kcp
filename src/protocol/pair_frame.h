#ifndef PAIR_FRAME_H
#define PAIR_FRAME_H

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

// // kcp frame header size
// constexpr size_t KCP_FRAME_SIZE = 24;
// constexpr size_t KCP_SYNC_SIZE = 4;
// constexpr size_t KCP_CONV_SIZE = 4;

// constexpr uint8_t SYNC_BYTE_0 = 0x55;
// constexpr uint8_t SYNC_BYTE_1 = 0x55;
// constexpr uint8_t SYNC_BYTE_2 = 0x55;
// constexpr uint8_t SYNC_BYTE_3 = 0x55;

constexpr size_t PAIRFRAME_MSGTIMESTAMP_OFFSET = 4;
constexpr size_t PAIRFRAME_MSGHOSTIP_OFFSET = 12;
constexpr size_t PAIRFRAME_MSGHOSTPORT_OFFSET = 16;
constexpr size_t PAIRFRAME_MSGCOUNT_OFFSET = 18;
constexpr size_t PAIRFRAME_MSGTYPE_OFFSET = 19;
constexpr size_t PAIRFRAME_MSGBODYLEN_OFFSET = 20;
//constexpr size_t PAIRFRAME_SIGNDATA_OFFSET = 0;
constexpr size_t PAIRFRAME_MSGBODYDATA_OFFSET = 24;

constexpr size_t PAIRFRAME_MSGSYNC_SIZE = FRAME_MSGSYNC_SIZE;
constexpr size_t PAIRFRAME_MSGTIMESTAMP_SIZE = FRAME_MSGTIMESTAMP_SIZE;
constexpr size_t PAIRFRAME_MSGHOSTIP_SIZE = 4;
constexpr size_t PAIRFRAME_MSGHOSTPORT_SIZE = 2;
constexpr size_t PAIRFRAME_MSGCOUNT_SIZE = FRAME_MSGCOUNT_SIZE;
constexpr size_t PAIRFRAME_MSGTYPE_SIZE = FRAME_MSGTYPE_SIZE;
constexpr size_t PAIRFRAME_MSGBODYLEN_SIZE = FRAME_MSGBODYLEN_SIZE;
constexpr size_t PAIRFRAME_MSGSIGDATA_SIZE = FRAME_MSGSIGNDATA_SIZE;
constexpr size_t PAIRFRAME_MIN_SIZE = 56;

// constexpr size_t FRAME_HEAD_SIZE = (FRAME_HOSTIP_SIZE + FRAME_HOSTPORT_SIZE + 
//     FRAME_TYPE_SIZE + FRAME_COUNT_SIZE + FRAME_SIGKEY_SIZE);

class PairFrame : public BaseFrame {
public:
    friend class KcpServer;
    friend class KcpClient;

    PairFrame(const std::string &signkey);
    PairFrame(const std::string &signkey, uint64_t timestamp, std::string &ip, uint16_t port, 
        uint8_t count, MsgType type);
    PairFrame(const std::string &signkey, uint64_t timestamp, std::string &ip, uint16_t port, 
        uint8_t count, MsgType type, uint8_t *data_ptr, size_t data_len);
    ~PairFrame();

    int encode() override;

    int encode(uint8_t *data_ptr, size_t data_len) override;

    int decode(const uint8_t *data_ptr, size_t data_len) override;

    int set_host_ip(const std::string &ip)
    {
        unsigned long addr = 0x00;
    #ifdef HAS_INET_PTON
        if (! inet_pton (AF_INET, name, &add))
    #else
        if (! inet_aton (ip.c_str(), (struct in_addr *)(&addr)))
    #endif
            return -1;
        
        msg_address_.host = addr;
        return 0;
    }

    int set_host_port(const uint16_t port) 
    {
        msg_address_.port = port;
        return 0;
    }

    int get_host_ip(std::string &ip) const
    {
        unsigned long addr = msg_address_.host;
    #ifdef HAS_INET_NTOP
        if (inet_ntop (AF_INET, & address -> host, name, nameLength) == NULL)
    #else
        char *address = inet_ntoa(*(struct in_addr *)(&addr));
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
        port = msg_address_.port;
        return 0;
    }    

protected:
    // data buffer of frame

    // elements of frame   
    UkcpNetAddress msg_address_;
};


#endif /* PAIR_FRAME_H */