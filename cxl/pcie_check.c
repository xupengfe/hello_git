// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * PCIE/PCI info and capability check functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>

#define MAX_BUS 256
#define MAX_DEV 32
#define MAX_FUN 8
#define LEN_SIZE sizeof(unsigned long)
#define MAPS_LINE_LEN 128

static unsigned long BASE_ADDR;
static int check_list, is_pcie;

int usage(void)
{
	printf("Usage: [n|a|s|i|e bus dev func]\n");
	printf("n    Show all PCI and PCIE important capability info\n");
	printf("a    Show all PCI and PCIE info\n");
	printf("s    Show all PCi and PCIE speed and bandwidth\n");
	printf("i    Show all or specific PCI info\n");
	printf("I    Show all or specific PCI and binary info\n");
	printf("e    Show all or specific PCIE info\n");
	printf("bus  Specific bus number(HEX)\n");
	printf("dev  Specific device number(HEX)\n");
	printf("func Specific function number(HEX-optional)\n");
	exit(2);
}

int find_bar(void)
{
#ifdef __x86_64__
	FILE * maps;
	int find = 0;
	unsigned long *start = malloc(sizeof(unsigned long) * 5);
	char line[MAPS_LINE_LEN], name1[MAPS_LINE_LEN];
	char *mmio_bar = "MMCONFIG";

	printf("Try to open /proc/iomem\n");
	maps = fopen("/proc/iomem", "r");
	if (!maps) {
		printf("[WARN]\tCould not open /proc/iomem\n");
		exit(1);
	}

	while (fgets(line, MAPS_LINE_LEN, maps)) {
		if (!strstr(line, mmio_bar))
			continue;

		if (sscanf(line, "%p-%s",
				&start, name1) != 2) {
			continue;
			}
		printf("start:%p, name1:%s\n", start, name1);
		if (!start) {
			printf("BAR(start) is NULL, did you use root to execute?\n");
			exit(1);
		}
		printf("BAR(Base Address Register) for mmio MMCONFIG:%p\n", start);
		BASE_ADDR = (unsigned long)start;
		find = 1;
		break;
	}

	fclose(maps);

	if (find != 1) {
		printf("Could not find correct mmio base address:%d, exit.\n", find);
		exit(2);
	}
#endif
	return 0;
}

void typeshow(uint8_t data)
{
	printf("\tpcie type:%02x  - ", data);
	switch (data) {
	case 0x00:
		printf("PCI Express Endpoint device\n");
		break;
	case 0x01:
		printf("Legacy PCI Express Endpoint device\n");
		break;
	case 0x04:
		printf("RootPort of PCI Express Root Complex\n");
		break;
	case 0x05:
		printf("Upstream Port of PCI Express Switch\n");
		break;
	case 0x06:
		printf("Downstream Port of PCI Express Switch\n");
		break;
	case 0x07:
		printf("PCi Express-to-PCI/PCI-x Bridge\n");
		break;
	case 0x08:
		printf("PCI/PCI-xto PCi Express Bridge\n");
		break;
	case 0x09:
		printf("Root Complex Integrated Endpoint Device\n");
		break;
	case 0x0a:
		printf("Root Complex Event Collector\n");
		break;
	default:
		printf("reserved\n");
		break;
	}
}

void speedshow(uint8_t speed)
{
	printf("\tspeed: %x   - ", speed);
	switch (speed) {
	case 0x00:
		printf("2.5GT/S");
		break;
	case 0x02:
		printf("5GT/S");
		break;
	case 0x04:
		printf("8GT/S");
		break;
	default:
		printf("reserved");
		break;
	}
	printf("\n");
}

void linkspeed(uint8_t speed)
{
	printf("\tlink speed:%x   - ", speed);
	switch (speed) {
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

void linkwidth(uint8_t width)
{
	printf("\tlink width:%02x - ", width);
	switch (width) {
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

int check_pcie(uint32_t *ptrdata)
{
	uint8_t ver = 0;
	uint32_t next = 0x100, num = 0;
	uint16_t offset = 0, cap = 0;

	if (is_pcie == 1) {
		cap = (uint16_t)(*(ptrdata + next/4));
		offset = (uint16_t)(*(ptrdata + next/4) >> 20);
		ver = (uint8_t)((*(ptrdata + next/4) >> 16) & 0xf);
		if ((offset == 0) | (offset == 0xfff)) {
			printf("PCIE cap:%04x ver:%01x off:%03x|\n", cap, ver, offset);
			return 0;
		}
		printf("PCIE cap:%04x ver:%01x off:%03x|", cap, ver, offset);

		while (1) {
			num++;
			cap = (uint16_t)(*(ptrdata + offset/4));
			offset = (uint16_t)(*(ptrdata + offset/4) >> 20);
			ver = (uint8_t)((*(ptrdata + offset/4) >> 16) & 0xf);

			if (offset == 0) {
				printf("cap:%04x ver:%01x off:%03x|\n", cap, ver, offset);
				break;
			}
			printf("cap:%04x ver:%01x off:%03x|", cap, ver, offset);
			if (num > 21) {
				printf("PCIE num is more than 20, return\n");
				break;
			}
		}
	} else {
		printf("\n");
	}
	return 0;
}

int check_pci(uint32_t *ptrdata)
{
	uint8_t nextpoint = 0x34;
	uint32_t num = 0;
	uint32_t *ptrsearch;

	nextpoint = (uint8_t)(*(ptrdata + nextpoint/4));
	ptrsearch = ptrdata + nextpoint/4;

	if ((nextpoint == 0) | (nextpoint == 0xff)) {
		printf("off:0x34->%02x|\n", nextpoint);
		return 0;
	}
	printf("off:0x34->%02x cap:%02x|",
			nextpoint, (uint8_t)(*ptrsearch));

	while (1) {
		if ((uint8_t)((*ptrsearch) >> 8) == 0x00) {
			printf("off:%02x|", (uint8_t)((*ptrsearch) >> 8));
			break;
		}
		if (num >= 16)
			break;

		printf("off:%02x ", (uint8_t)(((*ptrsearch) >> 8) & 0x00ff));
		ptrsearch = ptrdata + ((uint8_t)(((*ptrsearch) >> 8) & 0x00ff))/4;
		printf("cap:%02x|", (uint8_t)(*ptrsearch));
		num++;
	}

	if (((check_list >> 3) & 0x1) == 1) {
		printf("\n");
		return 0;
	}
	check_pcie(ptrdata);

	return 0;
}

int pci_show(uint32_t bus, uint32_t dev, uint32_t fun)
{
	uint32_t *ptrdata = malloc(sizeof(unsigned long) * 4096);
	uint32_t addr = 0;
	int fd, offset;

	fd = open("/dev/mem", O_RDWR);
	if (fd < 0) {
		printf("open /dev/mem failed!\n");
		return -1;
	}

	if (BASE_ADDR == 0)
		find_bar();

	addr = BASE_ADDR | (bus << 20) | (dev << 15) | (fun << 12);
	ptrdata = mmap(NULL, LEN_SIZE, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd, addr);

	if ((*ptrdata != 0xffffffff) && (*ptrdata != 0)) {
		printf("%02x:%02x.%01x:", bus, dev, fun);

		if (((check_list >> 1) & 0x1) == 1) {
			for (offset = 0; offset < 64; offset++) {
				if (offset % 4 == 0)
					printf("\n%02x: ", offset * 4);
				printf("%02x ", (uint8_t)(*(ptrdata + offset) >> 0));
				printf("%02x ", (uint8_t)(*(ptrdata + offset) >> 8));
				printf("%02x ", (uint8_t)(*(ptrdata + offset) >> 16));
				printf("%02x ", (uint8_t)(*(ptrdata + offset) >> 24));
			}
			if (is_pcie == 1) {
				for (offset = 64; offset < 1024; offset++) {
					if (offset % 4 == 0)
						printf("\n%02x: ", offset * 4);
					printf("%02x ", (uint8_t)(*(ptrdata + offset) >> 0));
					printf("%02x ", (uint8_t)(*(ptrdata + offset) >> 8));
					printf("%02x ", (uint8_t)(*(ptrdata + offset) >> 16));
					printf("%02x ", (uint8_t)(*(ptrdata + offset) >> 24));
				}
			}
			printf("\n");
		}
		if (is_pcie == 1)
			check_pcie(ptrdata);
		else
			check_pci(ptrdata);
	}
	munmap(ptrdata, LEN_SIZE);
	close(fd);
	return 0;
}

int recognize_pcie(uint32_t *ptrdata, uint32_t *ptrsearch)
{
	int loop_num = 0;

	while (1) {
		loop_num++;
		/* 0x10 means PCIE capability */
		if ((uint8_t)(*ptrsearch) == 0x10) {
			is_pcie = 1;
			break;
		}
		if ((uint8_t)(*ptrsearch) == 0xff)
			return 2;

		/* no PCIE find */
		if ((uint8_t)((*ptrsearch) >> 8) == 0x00)
			break;
		if (loop_num >= 16)
			break;
		/* next capability */
		ptrsearch = ptrdata + ((uint8_t)(((*ptrsearch) >> 8)
				& 0x00ff))/4;
		loop_num++;
	}
	return 0;
}

int scan_pci(void)
{
	uint32_t addr = 0, ptr_content = 0xffffffff;
	uint32_t bus, dev, fun, *ptrsearch;
	uint32_t *ptrdata = malloc(sizeof(unsigned long) * 4096);
	uint8_t nextpoint;

	int fd;

	fd = open("/dev/mem", O_RDWR);

	if (fd < 0) {
		printf("open /dev/mem failed!\n");
		return -1;
	}
	printf("fd=%d open /dev/mem successfully.\n", fd);

	ptrdata = &ptr_content;
	for (bus = 0; bus < MAX_BUS; ++bus) {
		for (dev = 0; dev < MAX_DEV; ++dev) {
			for (fun = 0; fun < MAX_FUN; ++fun) {
				addr = BASE_ADDR | (bus << 20) | (dev << 15) | (fun << 12);
				ptrdata = mmap(NULL, LEN_SIZE, PROT_READ | PROT_WRITE,
							MAP_SHARED, fd, addr);

				if (ptrdata == (void *)-1) {
					munmap(ptrdata, LEN_SIZE);
					break;
				}

				if ((*ptrdata != 0xffffffff) && (*ptrdata != 0)) {
					/* 0x34/4 is capability pointer in PCI */
					nextpoint = (uint8_t)(*(ptrdata + 0x34/4));
					ptrsearch = ptrdata + nextpoint/4;

					is_pcie = 0;
					if (recognize_pcie(ptrdata, ptrsearch) == 2) {
						printf("Check %02x:%02x.%x cap is %02x, return\n",
								bus, dev, fun, (uint8_t)(*ptrsearch));
						return 2;
					}

					if (is_pcie == 0)
						printf("PCI  %02x:%02x.%x: ", bus, dev, fun);
					else
						printf("PCIE %02x:%02x.%x: ", bus, dev, fun);
					printf("vender:0x%04x dev:0x%04x ", (*ptrdata) & 0x0000ffff,
							((*ptrdata) >> 16) & 0x0000ffff);
					if (((check_list >> 2) & 0x1) == 1)
						check_pcie(ptrdata);
					else
						check_pci(ptrdata);

					if ((check_list & 0x1) == 1) {
						typeshow((uint8_t)(((*ptrsearch)>>20)&0x0f));
						speedshow((uint8_t)(((*(ptrsearch+0x2c/4))>>1)&0x7f));
						linkspeed((uint8_t)(*(ptrsearch+0x0c/4)&0x0f));
						linkwidth((uint8_t)(((*(ptrsearch+0x0c/4))>>4)&0x3f));
					}
					if (((check_list >> 1) & 0x1) == 1)
						pci_show(bus, dev, fun);
				}
				munmap(ptrdata, LEN_SIZE);
			}
		}
	}
	close(fd);
	return 0;
}

int main(int argc, char *argv[])
{
	char parm;
	uint32_t bus, dev, func;

	if (argc == 2) {
		if (sscanf(argv[1], "%c", &parm) != 1) {
			printf("Invalid parm:%c\n", parm);
			usage();
		}
		printf("1 parameters: parm=%c\n", parm);
		find_bar();

		switch (parm) {
		case 'a':
			check_list = (check_list | 0x7);
			break;
		case 's': // speed
			check_list = (check_list | 0x1);
			break;
		case 'x': // pci binary
			check_list = (check_list | 0x2);
			break;
		case 'i': // only check pci capability
			check_list = (check_list | 0x8);
			break;
		case 'e': // only pcie capability
			check_list = (check_list | 0x4);
			break;
		case 'n':
			check_list = 0;
			break;
		case 'h':
			usage();
			break;
		default:
			usage();
			break;
		}
		scan_pci();
	}  else if ((argc == 4) | (argc == 5)) {
		if (sscanf(argv[1], "%c", &parm) != 1) {
			printf("Invalid parm:%c\n", parm);
			usage();
		}
		find_bar();
		switch (parm) {
		case 'i':
			is_pcie = 0;
			break;
		case 'I':
			is_pcie = 0;
			check_list = (check_list | 0x2);
			break;
		case 'e':
			is_pcie = 1;
			break;
		case 'a':
			check_list = (check_list | 0x7);
			is_pcie = 1;
			break;
		default:
			usage();
			break;
		}

		if (sscanf(argv[2], "%x", &bus) != 1) {
			printf("Invalid bus:%x", bus);
			usage();
		}

		if (sscanf(argv[3], "%x", &dev) != 1) {
			printf("Invalid dev:%x", dev);
			usage();
		}
		if (argc == 5) {
			if (sscanf(argv[4], "%x", &func) != 1) {
				printf("Invalid func:%x", func);
				usage();
			}
		} else {
			printf("No useful input func, will scan all func\n");
			for (func = 0; func < MAX_FUN; ++func)
				pci_show(bus, dev, func);
			return 0;
		}
		printf("parm:%c bus:dev.func: %02x:%02x.%x\n", parm, bus, dev, func);

		pci_show(bus, dev, func);
	} else {
		find_bar();
		usage();
	}

	return 0;
}
