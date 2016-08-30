#include "pci_bar.h"

int 
main(int argc, char* argv[])
{
	pci_bar* bar0 = new pci_bar(4,0,0,0);
	pci_bar* bar2 = new pci_bar(4,0,0,2);
	bar0->pci_write(0x4000000,(uint8_t)0x0f);
	bar0->pci_write(0x4000020,(uint8_t)0x0f);
	delete bar0;
	delete bar2;
	return 0;
}

