#include "pci_bar.h"

#define A printf("%d\n",__LINE__);

pci_bar::pci_bar(unsigned int bus, unsigned int slot, unsigned int func, unsigned int bar)
{
	    if (getuid() != 0) {
             printf("ERROR: Must be run as root\n");
             exit(1);
        }
        
		m_fd = open ("/dev/mem", O_RDWR | O_SYNC);
		
		if (!m_fd)
		{
			printf("ERROR: Can't open /dev/mem\n");
			exit(1);
		}
		
		if (pci_bar_config(bus, slot,func, bar) != 0)
		{
			printf("ERROR: pci_bar_config error\n");
			exit(1);
		}	
		
		m_bar = bar;
}

pci_bar::~pci_bar()
{
	close(m_fd);
}

int
pci_bar::pci_bar_config(unsigned int bus, unsigned int slot, unsigned int func, unsigned int bar)
{
	struct pci_access *pacc;
    struct pci_dev *pdev;
    char namebuf[1024];
    char *name;
    unsigned int c;
    pciaddr_t base;
    pciaddr_t mask;
    pciaddr_t address;
    u32 base_address_register;

    pacc = pci_alloc();		/* Get the pci_access structure */
	pci_init(pacc);			/* Initialize the PCI library */
	pci_scan_bus(pacc);		/* We want to get the list of devices */

    pdev = pci_get_dev(pacc, DOMAIN , bus, slot, func);  
    
    pci_fill_info (pdev,
		 PCI_FILL_IDENT | PCI_FILL_IRQ | PCI_FILL_BASES |
		 PCI_FILL_SIZES | PCI_FILL_ROM_BASE | PCI_FILL_CLASS |
		 PCI_FILL_CAPS | PCI_FILL_EXT_CAPS);
		 
	name = pci_lookup_name(pacc, namebuf, sizeof(namebuf), PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE, pdev->vendor_id, pdev->device_id);
	
    c = pci_read_byte(pdev, PCI_INTERRUPT_PIN); 
    
	
    base = pdev->base_addr[bar];
    
    
	if (!base) {
		printf("Base %d doesn't exits\n", bar);
		return -1;
	}
	
	// IO = 1 MEM= 0
	m_flag = 0;
	if (base & PCI_BASE_ADDRESS_SPACE_IO)
	{
		m_flag = 1;
		printf("Manage only MEM space\n");
		return -1;
	}
	
	m_base = base & PCI_ADDR_MEM_MASK;
	m_64 = 0;
	if (( base & PCI_BASE_ADDRESS_MEM_TYPE_MASK) == PCI_BASE_ADDRESS_MEM_TYPE_64) {
		m_64 = 1;
		m_base = m_base | ((pdev->base_addr[bar+1] & PCI_ADDR_MEM_MASK) << 32 );
	}
	
	// check size
	address = PCI_BASE_ADDRESS_0 + 4*bar;	
	base_address_register = pci_read_long(pdev, address) & PCI_ADDR_MEM_MASK;
	pci_write_long(pdev,address,~0);
	mask = pci_read_long(pdev, address) & PCI_ADDR_MEM_MASK;
	pci_write_long(pdev,address,base_address_register);
	m_size = ~mask + 1;
	
	printf("%04x:%02x:%02x.%d vendor=%04x device=%04x class=%04x (%s)\n",
		pdev->domain, pdev->bus, pdev->dev, pdev->func, pdev->vendor_id, pdev->device_id,pdev->device_class,name);
	printf("irq=%d (pin %d) base%d=%lx [%dK]\n\n",pdev->irq, c, bar,(long) pdev->base_addr[bar],m_size/1024);
	
	pci_free_dev(pdev);
	pci_cleanup(pacc);
	return 0;
}

int 
pci_bar::pci_write(unsigned int address, uint8_t data){
	void *map_base;
	void *virt_addr;
	
	map_base = mmap (NULL, address + sizeof(data), PROT_WRITE, MAP_SHARED, m_fd, (off_t)m_base);
	if (map_base == MAP_FAILED)
	{
		printf("mmap failed errno %d\n",errno);
		return -1;
	}
	
	virt_addr = map_base + address;
	
	//printf("m_base    %016x\n",m_base);
	//printf("map_base  %016x\n",map_base);
	//printf("virt_addr %016x\n",virt_addr);
	
	*((uint8_t *) virt_addr) = data;
	
	printf("bar#%d: pci_write8 @0x%08x <- 0x%02x\n",m_bar,address,data);
	
	munmap (map_base,address + sizeof(data));
	return 0;
}

int 
pci_bar::pci_write(unsigned int address, uint16_t  data){
	void *map_base;
	void *virt_addr;
	
	map_base = mmap (NULL, address + sizeof(data), PROT_WRITE, MAP_SHARED, m_fd, (off_t)m_base);
	if (map_base == MAP_FAILED)
	{
		printf("mmap failed errno %d\n",errno);
		return -1;
	}
	
	virt_addr = map_base + address;
	
	//printf("m_base    %016x\n",m_base);
	//printf("map_base  %016x\n",map_base);
	//printf("virt_addr %016x\n",virt_addr);
	
	*((uint16_t *) virt_addr) = data;
	printf("bar#%d: pci_write16x @0x%08x <- 0x%04x\n",m_bar,address,data);
	
	munmap (map_base,address + sizeof(data));
	return 0;
}

int 
pci_bar::pci_write(unsigned int address, uint32_t  data){
	void *map_base;
	void *virt_addr;
	
	map_base = mmap (NULL, address + sizeof(data), PROT_WRITE, MAP_SHARED, m_fd, (off_t)m_base);
	if (map_base == MAP_FAILED)
	{
		printf("mmap failed errno %d\n",errno);
		return -1;
	}
	
	virt_addr = map_base + address;
	
	//printf("m_base    %016x\n",m_base);
	//printf("map_base  %016x\n",map_base);
	//printf("virt_addr %016x\n",virt_addr);
	
	*((uint32_t *) virt_addr) = data;
	printf("bar#%d: pci_write32 @0x%08x <- 0x%08x\n",m_bar,address,data);
	
	munmap (map_base,address + sizeof(data));
	return 0;
}

int 
pci_bar::pci_read (unsigned int address, uint8_t*  data){
	void *map_base;
	void *virt_addr;
		
	if (address >= m_size)
	{
		printf("address out of available space max=0x%08x\n",m_size);
		return -1;
	}
	
	map_base = mmap (NULL, m_size , PROT_READ, MAP_SHARED, m_fd, (off_t)m_base);
	if (map_base == MAP_FAILED)
	{
		printf("mmap failed errno %d\n",errno);
		return -1;
	}
	
	virt_addr = map_base + address;
	
	*data = *(uint8_t*) virt_addr;
	printf("bar#%d: pci_read8 @0x%08x -> 0x%02x\n",m_bar,address,*data);
	
	munmap (map_base,address + sizeof(data));
	return 0;
	
}

int 
pci_bar::pci_read (unsigned int address, uint16_t* data){
		void *map_base;
	void *virt_addr;
		
	if (address >= m_size)
	{
		printf("address out of available space max=0x%08x\n",m_size);
		return -1;
	}
	
	map_base = mmap (NULL, m_size , PROT_READ, MAP_SHARED, m_fd, (off_t)m_base);
	if (map_base == MAP_FAILED)
	{
		printf("mmap failed errno %d\n",errno);
		return -1;
	}
	
	virt_addr = map_base + address;
	
	*data = *(uint16_t*) virt_addr;
	printf("bar#%d: pci_read16 @0x%08x -> 0x%04x\n",m_bar,address,*data);
	
	
	munmap (map_base,address + sizeof(data));
	return 0;
	
}

int 
pci_bar::pci_read (unsigned int address, uint32_t* data){
		void *map_base;
	void *virt_addr;
		
	if (address >= m_size)
	{
		printf("address out of available space max=0x%08x\n",m_size);
		return -1;
	}
	
	map_base = mmap (NULL, m_size , PROT_READ, MAP_SHARED, m_fd, (off_t)m_base);
	if (map_base == MAP_FAILED)
	{
		printf("mmap failed errno %d\n",errno);
		return -1;
	}
	
	virt_addr = map_base + address;
	
	*data = *(uint32_t*) virt_addr;
	printf("bar#%d: pci_read32 @0x%08x -> 0x%08x\n",m_bar,address,*data);
	
	munmap (map_base,address + sizeof(data));
	return 0;
}
	
	
	
        
