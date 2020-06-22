#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/mman.h> //mmap
#include<fcntl.h>   //open filel

#define MAX_BUS 31 //一般来说到4就够了
#define MAX_DEV 32
#define MAX_FUN 8
#define BASE_ADDR 0xf8000000  //我的电脑的基地址，你的可能不一样哦
#define LEN_SIZE sizeof(unsigned long)

typedef unsigned int WORD;
typedef unsigned char BYTE;//8bit


void typeshow(BYTE data) //
{
    printf("pcie type:%02x  - ",data);
    switch(data)
    {
        case 0x00:printf("PCIExpress Endpoint device\n");
                  break;
        case 0x01:printf("LegacyPCI Express Endpoint device\n");
                  break;
        case 0x04:printf("RootPort of PCI Express Root Complex\n");
                  break;
        case 0x05:printf("Upstream Port of PCI Express Switch\n");
                  break;
        case 0x06:printf("DownstreamPort of PCI Express Switch\n");
                  break;
        case 0x07:printf("PCiExpress-to-PCI/PCI-x Bridge\n");
                  break;
        case 0x08:printf("PCI/PCI-xto PCi Express Bridge\n");
                  break;
        case 0x09:printf("RootComplex Integrated Endpoint Device\n");
                  break;
        case 0x0a:printf("RootComplex Event Collector\n");
                  break;

        default:printf("reserved\n");
                break;
    }
}

void speedshow(BYTE speed)
{
    printf("speed: %x   - ",speed);
    switch(speed)
    {
        case 0x00:printf("2.5GT/S");
                  break;
        case 0x02:printf("5GT/S");
                  break;
        case 0x04:printf("8GT/S");
                  break;

        default:printf("reserved");
                break;
    }
    printf("\n");
}

void linkspeedshow(BYTE speed)
{
    printf("link speed:%x   - ",speed);
    switch(speed)
    {
        case 0x01:printf("SupportedLink Speeds Vector filed bit 0");
                         break;
        case 0x02:printf("SupportedLink Speeds Vector filed bit 1");
                         break;
        case 0x03:printf("SupportedLink Speeds Vector filed bit 2");
                         break;
        case 0x04:printf("SupportedLink Speeds Vector filed bit 3");
                         break;
        case 0x05:printf("SupportedLink Speeds Vector filed bit 4");
                         break;
        case 0x06:printf("SupportedLink Speeds Vector filed bit 5");
                         break;
        case 0x07:printf("SupportedLink Speeds Vector filed bit 6");
                          break;

        default:printf("reserved");
                         break;
    }
    printf("\n");
}

void linkwidthshow(BYTE width)
{
    printf("link width:%02x - ",width);
    switch(width)
    {
        case 0x01:printf("x1");
                          break;
        case 0x02:printf("x2");
                         break;
        case 0x04:printf("x4");
                         break;
        case 0x08:printf("x8");
                         break;
        case 0x0c:printf("x12");
                          break;
        case 0x10:printf("x16");
                         break;
        case 0x20:printf("x32");
                         break;

        default:printf("reserved");
                         break;
    }
    printf("\n");
}

int main()
{
    WORD addr=0;
    WORD bus,dev,fun;
    WORD *ptrdata;
    WORD*ptrsearch;
    BYTE nextpoint;//8bit

    int fd;
    int i;

    fd=open("/dev/mem",O_RDWR);
    //“/dev/mem”物理內存全映像，可以用來訪問物理內存，一般是open("/dev/mem",O_RD_WR),然後mmap，接著就可以用mmap地址訪問物理內存

    if(fd<0)
    {
        printf("openmemory failed!\n");
        return -1;
    }
    printf("fd=%d open /dev/mem successfully.\n",fd);

    for(bus=0; bus<MAX_BUS; ++bus)
    {
        for(dev=0; dev<MAX_DEV; ++dev)
        {
            for(fun=0; fun<MAX_FUN; ++fun)
            {
                //addr=0;
                addr=BASE_ADDR|(bus<<20)|(dev<<15)|(fun<<12);//要寻找的偏移地址，根据PCIe的物理内存偏移
                ptrdata=mmap(NULL,LEN_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,addr);//映射后返回的首地址
                //void*mmap(void* start,size_t length,int prot,int flags,int fd,off_t offset);
                //start映射區開始地址，設置為NULL時表示由系統決定
                //length映射區長度單位，地址自然是sizeof(unsigned long)
                //prot設置為PORT_READ|PORT_WRITE表示頁可以被讀寫
                //flag指定映射對象類型，MAP_SHARED表示與其他所有映射這個對象的所有進程共享映射空間
                //fd文件描述符
                //offset被映射對象內容的起點

                if(ptrdata==(void *)-1)
                {
                   munmap(ptrdata,LEN_SIZE);
                   break;
                }
                if(*ptrdata != 0xffffffff)
                {
                   nextpoint=(BYTE)(*(ptrdata+0x34/4));//capability pointer
                   ptrsearch=ptrdata + nextpoint/4;//the base address of capability structure

                   printf("PCI %02x:%02x.%x -> next point:%x    ",
                    bus, dev, fun, nextpoint);
                   printf("ptrdata:0x%x, search:%x\n",*ptrdata, *ptrsearch);

                   while(1)//search for the pcie device
                   {
                       if((BYTE)(*ptrsearch)==0x10)//capability id of 10h indicating pcie capabilitystructure
                       {
                            printf("\n/***************************\n");
                            printf("PCIE -> %02x:%02x.%x\n",bus, dev, fun);
                            printf("vender id:%x\t",(*ptrdata)&0x0000ffff);
                            printf("device id:%x\n",((*ptrdata)>>8)&0x0000ffff);

                            printf("ptrsearch:%x\n",*ptrsearch);
                            typeshow((BYTE)(((*ptrsearch)>>20)&0x0f));

                            speedshow((BYTE)(((*(ptrsearch+0x2c/4))>>1)&0x7f));

                            linkspeedshow((BYTE)(*(ptrsearch+0x0c/4)&0x0f));

                            linkwidthshow((BYTE)(((*(ptrsearch+0x0c/4))>>4)&0x3f));
                            printf("***************************/\n");
                            break;//havefound the pcie device and printed all the message,break the while
                       }
                       if((BYTE)((*ptrsearch)>>8)==0x00)//no pcie device exsist
                            break;
                       ptrsearch=ptrdata+((BYTE)(((*ptrsearch)>>8)&0x00ff))/4;//next cap pointer
                       printf("next pointer:%x\n",*ptrsearch);
                   }
                }
                munmap(ptrdata,LEN_SIZE);
            }
        }
    }
    close(fd);
    return 0;
}
