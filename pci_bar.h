#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <pci/pci.h>
#ifdef __cplusplus
}
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <pci/pci.h>
#include <stdint.h>
#include <errno.h>

#define DOMAIN 0

class pci_bar
{
	public:
		pci_bar(unsigned int bus, unsigned int slot, unsigned int func, unsigned int bar);
		pci_bar(unsigned int vendorid, unsigned int deviceid, unsigned int bar);
		~pci_bar();
		int pci_write(unsigned int address, uint8_t   data);
		int pci_write(unsigned int address, uint16_t  data);
		int pci_write(unsigned int address, uint32_t  data);
		int pci_read (unsigned int address, uint8_t*  data);
		int pci_read (unsigned int address, uint16_t* data);
		int pci_read (unsigned int address, uint32_t* data);
		pciaddr_t get_base_address();
		
	private:
		int pci_bar_config(unsigned int bus, unsigned int slot, unsigned int func, unsigned int bar);
		int m_fd;
		pciaddr_t m_base;
		u32 m_flag;
		u32 m_size;
		u32 m_64;
		int m_bar;
		pciaddr_t m_pagesize;
		pciaddr_t m_pageoffset;
};

