#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<fcntl.h>

#define MAX_BUS 100
#define MAX_DEV 32
#define MAX_FUN 8
#define LEN_SIZE sizeof(unsigned long)
#define MAPS_LINE_LEN 128

typedef unsigned int WORD;
typedef unsigned char BYTE;

static unsigned long BASE_ADDR;
static int check_list;

void typeshow(BYTE data)
{
	printf("\tpcie type:%02x  - ",data);
	switch(data) {
		case 0x00:printf("PCI Express Endpoint device\n");
			break;
		case 0x01:printf("Legacy PCI Express Endpoint device\n");
			break;
		case 0x04:printf("RootPort of PCI Express Root Complex\n");
			break;
		case 0x05:printf("Upstream Port of PCI Express Switch\n");
			break;
		case 0x06:printf("Downstream Port of PCI Express Switch\n");
			break;
		case 0x07:printf("PCi Express-to-PCI/PCI-x Bridge\n");
			break;
		case 0x08:printf("PCI/PCI-xto PCi Express Bridge\n");
			break;
		case 0x09:printf("Root Complex Integrated Endpoint Device\n");
			break;
		case 0x0a:printf("Root Complex Event Collector\n");
			break;

		default:printf("reserved\n");
			break;
	}
}

void speedshow(BYTE speed)
{
	printf("\tspeed: %x   - ",speed);
	switch(speed) {
	case 0x00:
		printf("2.5GT/S");
		break;
	case 0x02:
		printf("5GT/S");
		break;
	case 0x04: printf("8GT/S");
		break;

	default: printf("reserved");
		break;
	}
	printf("\n");
}

void linkspeed(BYTE speed)
{
	printf("\tlink speed:%x   - ",speed);
	switch(speed) {
	case 0x01:
		printf("SupportedLink Speeds Vector filed bit 0");
		 break;
	case 0x02:
		printf("SupportedLink Speeds Vector filed bit 1");
		break;
	case 0x03:
		printf("SupportedLink Speeds Vector filed bit 2");
		break;
	case 0x04:
		printf("SupportedLink Speeds Vector filed bit 3");
		break;
	case 0x05:
		printf("SupportedLink Speeds Vector filed bit 4");
		break;
	case 0x06:
		printf("SupportedLink Speeds Vector filed bit 5");
		break;
	case 0x07:
		printf("SupportedLink Speeds Vector filed bit 6");
		break;
	default:
		printf("reserved");
		break;
	}
	printf("\n");
}

void linkwidth(BYTE width)
{
	printf("\tlink width:%02x - ",width);
	switch(width) {
	case 0x01:
		printf("x1");
		break;
	case 0x02:
		printf("x2");
		break;
	case 0x04:
		printf("x4");
		break;
	case 0x08:
		printf("x8");
		break;
	case 0x0c:
		printf("x12");
		break;
	case 0x10:
		printf("x16");
		break;
	case 0x20:
		printf("x32");
		break;
	default:
		printf("reserved");
		break;
	}
	printf("\n");
}

int check_pci(WORD *ptrdata, WORD bus, WORD dev, WORD fun)
{
	BYTE nextpoint = 0x34;
	WORD *ptrsearch;
	int num = 0;

	nextpoint = (BYTE)(*(ptrdata + nextpoint/4));
	ptrsearch = ptrdata + nextpoint/4;
	printf("PCI  %02x:%02x.%x-> ",bus, dev, fun);
	printf("vender:0x%04x ", (*ptrdata) & 0x0000ffff);
	printf("dev:0x%04x offset:0x34->%02x cap:%02x|", 
		((*ptrdata) >> 16) & 0x0000ffff, nextpoint, (BYTE)(*ptrsearch));

	while(1) {
		if((BYTE)((*ptrsearch) >> 8) == 0x00) {
			printf("\n");
			break;
		}
		if ( num >= 16) {
			printf("\n");
			break;
		}

		printf("offset:%02x ", (BYTE)(((*ptrsearch) >> 8) & 0x00ff));
		ptrsearch = ptrdata + ((BYTE)(((*ptrsearch) >> 8) & 0x00ff))/4;
		printf("cap:%02x|", (BYTE)(*ptrsearch));
		num++;
	}
	return 0;
}

int check_pcie(WORD *ptrdata, WORD bus, WORD dev, WORD fun)
{
	BYTE nextpoint = 0x34;
	WORD *ptrsearch;
	int num = 0;

	nextpoint = (BYTE)(*(ptrdata + nextpoint/4));
	ptrsearch = ptrdata + nextpoint/4;
	printf("PCIE %02x:%02x.%x-> ",bus, dev, fun);
	printf("vender:0x%04x ", (*ptrdata) & 0x0000ffff);
	printf("dev:0x%04x offset:0x34->%02x cap:%02x|", 
		((*ptrdata) >> 16) & 0x0000ffff, nextpoint, (BYTE)(*ptrsearch));

	while(1) {
		if((BYTE)((*ptrsearch) >> 8) == 0x00) {
			printf("\n");
			break;
		}
		if ( num >= 16) {
			printf("\n");
			break;
		}

		printf("offset:%02x ", (BYTE)(((*ptrsearch) >> 8) & 0x00ff));
		ptrsearch = ptrdata + ((BYTE)(((*ptrsearch) >> 8) & 0x00ff))/4;
		printf("cap:%02x|", (BYTE)(*ptrsearch));
		num++;
	}
	return 0;
}


int scan_pci(void)
{
	WORD addr = 0, ptr_content = 0xffffffff;
	WORD bus, dev, fun;
	WORD *ptrdata = malloc(sizeof(unsigned long) * 1024);
	WORD *ptrsearch;
	BYTE nextpoint;

	int fd, i, is_pcie = 0, loop_num = 0;

	fd = open("/dev/mem",O_RDWR);

	if(fd < 0)
	{
		printf("open /dev/mem failed!\n");
		return -1;
	}
	printf("fd=%d open /dev/mem successfully.\n",fd);

	ptrdata = &ptr_content;
	for (bus = 0; bus < MAX_BUS; ++bus) {
		for (dev = 0; dev < MAX_DEV; ++dev) {
			for (fun = 0; fun < MAX_FUN; ++fun) {
				addr = BASE_ADDR | (bus << 20) | (dev << 15) | (fun << 12);//要寻找的偏移地址，根据PCIe的物理内存偏移

				ptrdata = mmap(NULL, LEN_SIZE, PROT_READ | PROT_WRITE,
							MAP_SHARED, fd, addr);//映射后返回的首地址
				//void*mmap(void* start,size_t length,int prot,int flags,int fd,off_t offset);
				//start映射區開始地址，設置為NULL時表示由系統決定
				//length映射區長度單位，地址自然是sizeof(unsigned long)
				//prot設置為PORT_READ|PORT_WRITE表示頁可以被讀寫
				//flag指定映射對象類型，MAP_SHARED表示與其他所有映射這個對象的所有進程共享映射空間
				//fd文件描述符
				//offset被映射對象內容的起點

				if (ptrdata == (void *)-1) {
				   munmap(ptrdata, LEN_SIZE);
				   break;
				}

				if ((*ptrdata != 0xffffffff) && (*ptrdata != 0)) {
					/* 0x34/4 is capability pointer in PCI */
					nextpoint = (BYTE)(*(ptrdata + 0x34/4));
					ptrsearch = ptrdata + nextpoint/4;

					is_pcie = 0;
					loop_num = 0;
					/* search for PCIE */
					while(1) {
						loop_num++;
						/* 0x10 means PCIE capability */
						if ((BYTE)(*ptrsearch) == 0x10) {
							is_pcie = 1;
							break; /* find the pcie and break */
						}
						/* no PCIE find */
						if((BYTE)((*ptrsearch) >> 8) == 0x00)
							break;
						if ( loop_num >= 16)
							break;
						/* next capability */
						ptrsearch = ptrdata + ((BYTE)(((*ptrsearch) >> 8)
								& 0x00ff))/4;
						loop_num++;
					}

					if (is_pcie == 1) {
						check_pcie(ptrdata, bus, dev, fun);
						if ( check_list != 0 ) {
							typeshow((BYTE)(((*ptrsearch)>>20)&0x0f));
							speedshow((BYTE)(((*(ptrsearch+0x2c/4))>>1)&0x7f));
							linkspeed((BYTE)(*(ptrsearch+0x0c/4)&0x0f));
							linkwidth((BYTE)(((*(ptrsearch+0x0c/4))>>4)&0x3f));
						}
					}
					if (is_pcie == 0) {
						check_pci(ptrdata, bus, dev, fun);
						if ( check_list != 0 ) {
							typeshow((BYTE)(((*ptrsearch)>>20)&0x0f));
							speedshow((BYTE)(((*(ptrsearch+0x2c/4))>>1)&0x7f));
							linkspeed((BYTE)(*(ptrsearch+0x0c/4)&0x0f));
							linkwidth((BYTE)(((*(ptrsearch+0x0c/4))>>4)&0x3f));
						}
					}
				}
				munmap(ptrdata,LEN_SIZE);
			}
		}
	}
	close(fd);
	return 0;
}

int find_bar(void)
{
#ifdef __x86_64__
	FILE *maps;
	BASE_ADDR = 0xf8000000; // set it as a default BAR
	int find = 0;
	unsigned long *start = malloc(sizeof(unsigned long) * 5);
	char line[MAPS_LINE_LEN], name1[MAPS_LINE_LEN];
	char *mmio_bar = "MMCONFIG";

	printf("Try to open /proc/iomem\n");
	maps = fopen("/proc/iomem", "r");
	if (!maps) {
		printf("[WARN]\tCould not open /proc/iomem\n");
		exit (1);
	}

	while (fgets(line, MAPS_LINE_LEN, maps)) {
		if (!strstr(line, mmio_bar))
			continue;

		if (sscanf(line, "%p-%s",
				&start, name1) != 2) {
			continue;
			}
		printf("start:%p, name1:%s\n", start, name1);
		if ( !start ) {
			printf("BAR(start) is NULL, did you use root to execute?\n");
			exit (1);
		}
		printf("Find base address for mmio(MMCONFIG):%p\n", start);
		BASE_ADDR = (unsigned long)start;
		find = 1;
		break;
	}

	fclose(maps);

	if (find != 1) {
		printf("Could not find correct mmio base address:%d, exit.\n", find);
		exit (2);
	}

	return 0;
#else
	return 0;
#endif
}

int main(int argc, char *argv[])
{
	char parm;

	if (argc >= 2) {
		sscanf(argv[1], "%c", &parm);
		printf("1 parameters: parm=%c\n", parm);
	}

	find_bar();

	switch (parm) {
		case 'a' :
			scan_pci();
			printf("Show detailed info.\n");
			break;
		case 's' :
			check_list = 1;
			scan_pci();
			break;
		default :
			break;
	}

	return 0;
}
