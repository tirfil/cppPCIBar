# cppPCIBar
PCI debug tool "library" in c++

Dependency: pciutils-dev 

This version dealt only with PCI MEM space.


usage example:
==============
```cpp
// PCI card is located at 04:00.0
// We write different data at address 0x400_0000 bar0
// and at address 0x400_0020 bar0 
// And then we read back modified value.
#include "pci_bar.h"

int 
main(int argc, char* argv[])
{
	uint8_t data;
	pci_bar* bar0 = new pci_bar(4,0,0,0);
	bar0->pci_write(0x4000000,(uint8_t)0x01);
	bar0->pci_write(0x4000020,(uint8_t)0x02);
	bar0->pci_read(0x4000000,&data);
	printf("value is %d\n",data);
        bar0->pci_read(0x4000020,&data);
	printf("value is %d\n",data);
	delete bar0;
	return 0;
}
