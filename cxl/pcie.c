#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/mman.h>
//#include<linux/pci.h>

//#define DEBUG1

#define PCI_MAX_BUS 2
#define PCI_MAX_DEV 31
#define PCI_MAX_FUN 7
#define PCI_BASE_ADDR 0xfc000000L
#define LEN_SIZE sizeof(unsigned long)
#define SIZE 4096


typedef unsigned int WORD;
typedef unsigned long DWORD;
typedef unsigned char BYTE;


void typeshow(BYTE Type){
    switch(Type){
      case 0x0f:{
               printf("endpoint\t");
               break;
                 }
      case 0x1f:{
             printf("legacy endpoint\t");
                 break;
               }
      case 0x4f:{
             printf("root complex\t");
                break;
              }
      case 0x5f:{
             printf("upstream pci-e switch\t");
             break;
                }
      case 0x6f:{
             printf("downstream pci-e switch\t");
             break;
             }
      case 0x7f:{
            printf("pci-e to pci bridge\t");
             break;
              }
      case 0x8f:{
             printf("pci to pci-e bridge\t");
             break;
              }
     case 0x9f:{
             printf("root co inte endpoint\t");
             break;
               }
     case 0xaf:{
             printf("root co event endpoint\t");
             break;
                }
     defult:{
             printf("b\t");
             break;
            }
      }
}

void speedshow(int speed){
    if(speed == 0x0){
            printf("speed is 2.5G/s\t");
         }
    else if(speed == 0x2){
            printf("speed is 5G/s\t");
         }
    else if(speed == 0x4){
            printf("speed is 8G/s\t");
         }
    else{
           printf("no speed!\t");
        }
}
void linkspeedshow(int flag){
     switch(flag){
         case 0x1:{
            printf("link speed 0\t");
            break;
              }
         case 0x2:{
            printf("link speed 1\t");
            break;
              }
         case 0x3:{
            printf("link speed 2\t");
            break;
              }
         case 0x4:{
            printf("link speed 3\t");
            break;
              }
         case 0x5:{
            printf("link speed 4\t");
            break;
              }
         case 0x6:{
            printf("link speed 5\t");
            break;
              }
         case 0x7:{
            printf("link speed 6\t");
            break;
              }
         default:
               {
             printf("no speed!\t");
              break;
              }
     }
}
void linkwidthshow(int flag){
    printf("linkwidth:");
    switch(flag){
           case 0x00:{
                   printf("reserved\t");
                   break;
                 }
           case 0x01:{
                   printf("x1\t");
                   break;
                 }
           case 0x02:{
                   printf("x2\t");
                   break;
                 }
           case 0x04:{
                   printf("x4\t");
                   break;
                 }
           case 0x08:{
                   printf("x8\t");
                   break;
                 }
           case 0x0c:{
                   printf("x12\t");
                   break;
                 }
           case 0x10:{
                   printf("x16\t");
                   break;
                 }
           case 0x20:{
                   printf("x32\t");
                   break;
                 }
           default:{
                    printf("null\t");
                 }
  }
}

int main()
{
    DWORD addr = 0;
    WORD bus,dev,fun;
    WORD* DATA;
    WORD* dwDATA;
    BYTE nextPoint;
    int fd,i = 1;
    fd = open("/dev/mem",O_RDWR);
    if(fd < 0)
    {
      printf("can't open port!\n");
    }

    printf("fd = %d\n",fd);

     for(bus = 0; bus <= PCI_MAX_BUS; bus++)
        for(dev = 0; dev <= PCI_MAX_DEV; dev++)
           for(fun = 0; fun <= PCI_MAX_FUN; fun++)
              {
                 addr = 0;
                 addr = PCI_BASE_ADDR|(bus<<20)|(dev<<15)|(fun<<12);
                 DATA = mmap(NULL,LEN_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,addr);
                 if(DATA == (void *)-1)
                 {
                     munmap(DATA,LEN_SIZE);
                     break;
                 }
                 if(*DATA != 0xffffffff)
                 {
                    nextPoint = (BYTE)(*(DATA+0x34/4));
                    dwDATA = DATA+nextPoint/4;
                     #ifdef DEBUG1
                    printf("nextpoint = %x\n",nextPoint);
                    printf("addr1=%x\n",*dwDATA);
                     #endif
                 while(1){
                    if((BYTE)*(dwDATA)==0x10)
                     {
                             printf("PCI-E:");
                             printf("bus# = %x,dev# = %x,fun# = %x\t",bus,dev,fun);
                             printf("vender id:%.4x\t",*DATA&0xffff);
                             printf("device id:%.4x\t",((*DATA)>>8)&0XFFFF);
                             printf("\ntype:");
                             typeshow((BYTE)((*dwDATA)>>8)|0x0f);
                             speedshow(*(dwDATA+0x2c/4)>>1&0xf);
                          //   linkspeedshow(*(dwDATA+0x0c/4)&0xf);
                             linkwidthshow((*(dwDATA+0x0c/4)>>4)&0x3f);
                             printf("\n");
                             break;
                     }
                     dwDATA = DATA+((*(dwDATA)>>8)&0xff)/4;
                     #ifdef DEBUG1
                     printf("dwDATA = %x\n",*dwDATA);
                     #endif
                     if((BYTE)(*(dwDATA)) == 0x00)
                     break;
                     }
                 #ifdef DEBUG
                    printf("bus = %x,dev = %x,fun = %x\n",bus,dev,fun);
                    for(i = 0; i < 0;i++)
                    {
                        printf("data%d:%x\n",i,*(DATA+i));
                    }
                   printf(" next Point:%x\n",nextPoint);
                   printf("data:%x\n",(BYTE)*(DATA+nextPoint/4));
                 #endif
                 }
                  munmap(DATA,LEN_SIZE);
              }
    close(fd);
    return 0;
}
