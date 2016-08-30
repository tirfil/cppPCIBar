#include "pci_bar.h"

int 
main(int argc, char* argv[])
{
	uint32_t data;
	pci_bar* bar1 = new pci_bar(4,0,0,1);
	
	for(int i=0; i<16; i++)
		bar1->pci_read(i,&data);
	
	delete bar1;
	return 0;
}

