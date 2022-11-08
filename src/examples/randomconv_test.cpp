#include "connection.h"

int main()
{
    int ret = 0;
    int offset = 0;
    kcp_conv_t value[10] = {0};
    

    RandomConv conv(1000, 10);

    // for (int i=0; i<10; i++)
    {
        ret = conv.get_conv(value[offset++]);
        if (ret)
        {
            std::cout << "get_conv failed." << std::endl;
            return -1;
        }
        std::cout << "get value:" << value[0] << std::endl;
    }

    {
        ret = conv.get_conv(value[offset++]);
        if (ret)
        {
            std::cout << "get_conv failed." << std::endl;
            return -1;
        }
        std::cout << "get value:" << value[1] << std::endl;
    }

    {
        ret = conv.get_conv(value[2]);
        if (ret)
        {
            std::cout << "get_conv failed." << std::endl;
            return -1;
        }
        std::cout << "get value:" << value[2] << std::endl;
    }

    {
        ret = conv.put_conv(value[1]);
        if (ret)
        {
            std::cout << "put_conv failed." << std::endl;
            return -1;
        }
        std::cout << "put value:" << value[1] << std::endl;
    }
    
    conv.print();

    for (int i=0; i<10; i++)
    {
        ret = conv.get_conv(value[offset++]);
        if (ret)
        {
            std::cout << "get_conv failed." << std::endl;
            return -1;
        }
        std::cout << "value:" << value[i] << std::endl;
    }


    return 0;
}