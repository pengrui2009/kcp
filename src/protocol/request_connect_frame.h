#ifndef REQUEST_CONNECT_FRAME_H
#define REQUEST_CONNECT_FRAME_H

#include "base_frame.h"
#include "pair_frame.h"


class RequestConnectFrame : public PairFrame {
public:
    RequestConnectFrame(const std::string &signkey);
    RequestConnectFrame(const std::string &signkey, uint64_t timestamp, std::string &ip, uint16_t port, 
        uint8_t count);
    RequestConnectFrame(const std::string &signkey, uint64_t timestamp, std::string &ip, uint16_t port, 
        uint8_t count, uint8_t *data_ptr, size_t data_len);
    ~RequestConnectFrame();
    
private:

};

#endif /* REQUEST_CONNECT_FRAME_H */