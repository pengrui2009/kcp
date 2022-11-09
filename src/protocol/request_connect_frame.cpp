#include "request_connect_frame.h"
#include "base_frame.h"
#include "pair_frame.h"

RequestConnectFrame::RequestConnectFrame(const std::string &signkey) :
    PairFrame(signkey)
{

}

RequestConnectFrame::RequestConnectFrame(const std::string &signkey, uint64_t timestamp, std::string &ip, uint16_t port, 
    uint8_t count) :
    PairFrame(signkey, timestamp, ip, port, count, MSGTYPE_REQUEST_CONNECT)
{

}

RequestConnectFrame::RequestConnectFrame(const std::string &signkey, uint64_t timestamp, std::string &ip, 
    uint16_t port, uint8_t count, uint8_t *data_ptr, size_t data_len) :
    PairFrame(signkey, timestamp, ip, port, count, MSGTYPE_REQUEST_CONNECT, data_ptr, data_len)
{

}

RequestConnectFrame::~RequestConnectFrame()
{

}