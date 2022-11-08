#ifndef CONNECTION_H
#define CONNECTION_H

#include "ikcp.h"

#include "typedef.h"
#include "logger.h"
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <exception>

// TODO new reuse conv , but current not support
// random number
class Random
{
public:
	Random(int offset, size_t size) : offset_(offset)
    {
		this->size_ = 0;
        if (offset >= RAND_MAX)
        {
            offset_ = RAND_MAX;
            size = 0;
            throw std::runtime_error("offset >= 2147483647");
        }
        
        size = ((RAND_MAX - offset) > size) ? size : (RAND_MAX - offset);
		seeds_.resize(size);
	}

	int random() {
		uint32_t x, i;
		if (seeds_.size() == 0) 
        {
            return 0;
        }

		if (size_ == 0) 
        { 
			for (i = 0; i < (int)seeds_.size(); i++) 
            {
				seeds_[i] = offset_ + i;
			}
			size_ = seeds_.size();
		}
		i = rand() % size_;
		x = seeds_[i];
		seeds_[i] = seeds_[--size_];
		return x;
	}


protected:
	int offset_;
    size_t size_;

    
	std::vector<uint32_t> seeds_;
};

/**
 * @brief random get conversion
 * 
 */
class RandomConv {
public:
    RandomConv(int offset, int size);
    ~RandomConv();

    // int random() {
	// 	uint32_t x, i;
	// 	if (seeds_.size() == 0) 
    //     {
    //         return 0;
    //     }

	// 	if (size_ == 0) 
    //     { 
	// 		for (i = 0; i < (int)seeds_.size(); i++) 
    //         {
	// 			seeds_[i] = offset_ + i;
	// 		}
	// 		size_ = seeds_.size();
	// 	}
	// 	i = rand() % size_;
	// 	x = seeds_[i];
	// 	seeds_[i] = seeds_[--size_];
	// 	return x;
	// }


    int get_conv(kcp_conv_t &conv);

    int put_conv(const kcp_conv_t &conv);

    void print()
    {
        for (int i=0; i<left_size_; i++)
        {
            std::cout << "value:" << seeds_[i] << std::endl;
        }
    }
private:
    int offset_;
    size_t left_size_;
    size_t seed_size_;

    std::mutex mutex_;
	std::vector<uint32_t> seeds_;
    // std::unordered_map<kcp_conv_t, bool> conv_data_map_;
};

class KcpServer;


class Connection {
public:
    Connection(const kcp_conv_t conv, asio_endpoint_t endpoint, int mtu_size, KcpServer *server);

    ~Connection();

    // int kcp_input(uint8_t *data_ptr, size_t data_len)
    // {
    //     return 0;
    // }

    // int kcp_output()
    // {
    //     return 0;
    // }

    // int kcp_receive(uint8_t *data_ptr, size_t data_len)
    // {
    //     return 0;
    // }

    // int kcp_send(uint8_t *data_ptr, size_t data_len)
    // {
    //     return 0;
    // }

    kcp_conv_t get_kcpconv() const
    {
        return this->conv_;
    }

    asio_endpoint_t get_endpoint() const
    {
        return this->endpoint_;
    }

    ikcpcb* get_kcpcb()
    {
        return this->kcp_ptr_;
    }

    int kcp_input(const asio_endpoint_t& dest, const uint8_t* data_ptr, size_t data_len);

    static int kcp_output(const char *buf, int len, struct IKCPCB *kcp, void *user);

    int kcp_send(const uint8_t *data_ptr, int data_len);

    int kcp_receive(uint8_t *data_ptr, size_t data_len);

    void kcp_update(uint32_t millsec_time);
protected:

private:
    std::atomic_bool stop_;

    kcp_conv_t conv_;
    
    asio_endpoint_t endpoint_;
    // packet mtu size
    int mtu_size_;
    
    ikcpcb *kcp_ptr_;
    // std::shared_ptr<ikcpcb> kcp_ptr_;

    
    // static int output(const char *buf, int len, struct IKCPCB *kcp, void *user);
};


#endif /* CONNECTION_H */