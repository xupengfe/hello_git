#include <sys/io.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_BUS 256      // bus number:0~255， 8 bit
#define MAX_DEVICE 32    // device number:0~31, 5 bit
#define MAX_FUNCTION 8   // function number:0~7, 3 bit

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA 0xCFC

#define BASE_ADDR 0x80000000 // base addr, enable=1

typedef unsigned int WORD; // 4bytes

int main()
{
	WORD bus, device, func, reg;
	WORD data, address; //read info from CONFIG_DATA,address set to CONFIG_ADDRESS
	int ret=0;
	ret = iopl(3);
	if(ret < 0)
	{
		perror("iopl set to high level error\n");
		return -1;
	}

	/*  bus dev func */
	for(bus=0; bus<MAX_BUS; bus++)
		for(device=0; device<MAX_DEVICE; device++)
			for(func=0; func<MAX_FUNCTION; func++)
			{
				address = BASE_ADDR | (bus << 16) | (device << 11) | (func << 8);
				outl(address, CONFIG_ADDRESS); //put addr to config_address
				data = 0;
				data = inl(CONFIG_DATA); //read data from config data;
				if((data!=0xffffffff) && (data!=0))
			 	{
					printf("\n%02x:%02x.%01x\n",bus,device,func);
					//for(reg = 0; reg < 192; reg++)
					for(reg = 0; reg < 64; reg++)
					{
						address = BASE_ADDR | (bus << 16) | (device << 11) | (func << 8) | (reg << 2);
						outl(address , CONFIG_ADDRESS); //put addr to config_address
						data = inl(CONFIG_DATA); //read data from config data;
						if (data == 0xffffffff)
							continue;
						if(reg % 4 == 0)
							printf("%02x: ", reg * 4);

						printf("%02x ",(unsigned char)(data >> 0));
						printf("%02x ",(unsigned char)(data >> 8));
						printf("%02x ",(unsigned char)(data >> 16));
						printf("%02x ",(unsigned char)(data >> 24));
						if(reg % 4 == 3)
						printf("\n");
					}
				}
			}
	iopl(0);
	if(ret<0)
	{
		perror("iopl set to low level error\n");
		return -1;
	}
	return 0;
}
