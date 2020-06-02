#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/io.h>

#define MAX_BUS 256  // bus number:0~255， 8 bit
#define MAX_DEV 32   // device number:0~31, 5 bit
#define MAX_FUN 8    // function number:0~7, 3 bit

#define BASE_ADDR 0x80000000 //base addr, enable=1
#define CONFIG_ADDR 0xcf8
#define CONFIG_DATA 0xcfc

typedef unsigned int WORD;   // 4bytes

int main()
{
    WORD bus, dev, fun;
    WORD addr, data;
    int ret = 0;

    printf("bus#\tdev#\tfun#\t\n");

    ret=iopl(3); //set high power
    if(ret < 0)
    {
        perror("iopl set high power error");
        return -1;
    }

    for(bus=0; bus<MAX_BUS; ++bus)
    {
        for(dev=0; dev<MAX_DEV; ++dev)
        {
            for(fun=0; fun<MAX_FUN; ++fun)
            {
                data = 0;
                addr=BASE_ADDR | (bus<<16) | (dev<<11 )| (fun<<8); //把number们分别左移到相应的位上去，register number默认设为0
                outl(addr, CONFIG_ADDR); //put addr into CONFIG_ADDR
                data = inl(CONFIG_DATA); //read address from CONFIG_DATA
                if((data != 0xffffffff) && (data != 0)) { //如果vender ID和Device ID位全是1，代表没有该设备
                    printf("%x\n", addr);
                    printf("%2x\t%2x\t%2x\n", bus, dev, fun); //找到PCI设备后打印他的bus,dev,fun number
                }
            }
        }
    }

    ret=iopl(0); //set low power
    if(ret<0)
    {
        perror("iopl set low power error");
        return -1;
    }

    return 0;
}
