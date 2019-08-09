#include "pci_bar.h"

#define A printf("%d\n",__LINE__);

//#define DEBUG

pci_bar::pci_bar(unsigned int bus, unsigned int slot, unsigned int func, unsigned int bar)
{
	    if (getuid() != 0) {
             printf("ERROR: Must be run as root\n");
             exit(1);
        }
		m_pagesize = sysconf(_SC_PAGE_SIZE);
        
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

pci_bar::pci_bar(unsigned int vendorid, unsigned int deviceid, unsigned int bar)
{
	    
	    struct pci_access *pacc;
		struct pci_dev *dev;

	    
	    if (getuid() != 0) {
             printf("ERROR: Must be run as root\n");
             exit(1);
        }
		m_pagesize = sysconf(_SC_PAGE_SIZE);
        
		m_fd = open ("/dev/mem", O_RDWR | O_SYNC);
		
		if (!m_fd)
		{
			printf("ERROR: Can't open /dev/mem\n");
			exit(1);
		}
		
		pacc = pci_alloc();           /* Get the pci_access structure */
		pci_init(pacc);               /* Initialize the PCI library */
		pci_scan_bus(pacc);           /* We want to get the list of devices */
		for (dev=pacc->devices; dev; dev=dev->next)   /* Iterate over all devices */
		{
			pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS); 
			if ((dev->vendor_id == vendorid)&&(dev->device_id == deviceid)) break;
		}	
		
		if (dev == NULL)
		{
			printf("ERROR: PCI device %04x:%04x not detected\n",vendorid,deviceid);
			exit(1);
		}
		
		if (pci_bar_config(dev->bus, dev->dev,dev->func, bar) != 0)
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
    
    if (pacc == NULL) printf("pci_alloc() failed\n");
	pci_init(pacc);			/* Initialize the PCI library */
	pci_scan_bus(pacc);		/* We want to get the list of devices */

    pdev = pci_get_dev(pacc, DOMAIN , bus, slot, func);  
    if (pdev == NULL) printf("pci_get_dev() failed\n");
    
    pci_fill_info (pdev,
		 PCI_FILL_IDENT | PCI_FILL_IRQ | PCI_FILL_BASES |
		 PCI_FILL_SIZES | PCI_FILL_ROM_BASE | PCI_FILL_CLASS |
		 PCI_FILL_CAPS | PCI_FILL_EXT_CAPS);
		 
	name = pci_lookup_name(pacc, namebuf, sizeof(namebuf), PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE, pdev->vendor_id, pdev->device_id);
 
   if(name == NULL){
     printf("pci_lookup_name() failed\n");
   } else {
     printf("name is %s\n",name);
   }
	
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




	m_pageoffset = m_base % m_pagesize;
	printf("m_base = %08lx m_pagesize = %08lx m_pageoffset = %08lx\n", m_base,m_pagesize,m_pageoffset);
	m_base = m_base - m_pageoffset;
	printf("Page %08lx - Offset %08lx\n",m_base,m_pageoffset);
	
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
	int rc;
		
	map_base = mmap (NULL, m_pageoffset + address + sizeof(data), PROT_WRITE, MAP_SHARED, m_fd, (off_t)m_base);
	if (map_base == MAP_FAILED)
	{
		
		printf("mmap failed errno %d\n",errno);
		printf("args: NULL,0x%x,PROT_WRITE, MAP_SHARED, %d, 0x%x\n",m_pageoffset + address + sizeof(data),m_fd,m_base);
		//return -1;
		exit(1);
	}
	
	virt_addr = map_base + address + m_pageoffset;
	
	//printf("m_base    %016x\n",m_base);
	//printf("map_base  %016x\n",map_base);
	//printf("virt_addr %016x\n",virt_addr);
	
	*((uint8_t *) virt_addr) = data;
	
  #ifdef DEBUG
	printf("bar#%d: pci_write8 @0x%08x <- 0x%02x\n",m_bar,address,data);
  #endif
	
	rc = munmap (map_base,m_pageoffset + address + sizeof(data));
	if (rc<0) printf("munmap failed errno %d\n",errno);
	return 0;
}

int 
pci_bar::pci_write(unsigned int address, uint16_t  data){
	void *map_base;
	void *virt_addr;
	int rc;
	
	map_base = mmap (NULL, m_pageoffset + address + sizeof(data), PROT_WRITE, MAP_SHARED, m_fd, (off_t)m_base);
	if (map_base == MAP_FAILED)
	{
		printf("mmap failed errno %d\n",errno);
		printf("args: NULL,0x%x,PROT_WRITE, MAP_SHARED, %d, 0x%x\n",m_pageoffset + address + sizeof(data),m_fd,m_base);
		//return -1;
		exit(1);
	}
	
	virt_addr = map_base + address + m_pageoffset;
	
	//printf("m_base    %016x\n",m_base);
	//printf("map_base  %016x\n",map_base);
	//printf("virt_addr %016x\n",virt_addr);
	
	*((uint16_t *) virt_addr) = data;
 
  #ifdef DEBUG
	printf("bar#%d: pci_write16x @0x%08x <- 0x%04x\n",m_bar,address,data);
  #endif
	
	rc = munmap (map_base,m_pageoffset + address + sizeof(data));
	if (rc<0) printf("munmap failed errno %d\n",errno);
	return 0;
}

int 
pci_bar::pci_write(unsigned int address, uint32_t  data){
	void *map_base;
	void *virt_addr;
	int rc;
	
	map_base = mmap (NULL, m_pageoffset + address + sizeof(data), PROT_WRITE, MAP_SHARED, m_fd, (off_t)m_base);
	if (map_base == MAP_FAILED)
	{
		printf("mmap failed errno %d\n",errno);
		printf("args: NULL,0x%x,PROT_WRITE, MAP_SHARED, %d, 0x%x\n",m_pageoffset + address + sizeof(data),m_fd,m_base);
		//return -1;
		exit(1);
	}
	
	virt_addr = map_base + address + m_pageoffset;
	
	//printf("m_base    %016x\n",m_base);
	//printf("map_base  %016x\n",map_base);
	//printf("virt_addr %016x\n",virt_addr);
	
	*((uint32_t *) virt_addr) = data;
 
  #ifdef DEBUG
	printf("bar#%d: pci_write32 @0x%08x <- 0x%08x\n",m_bar,address,data);
  #endif
	
	rc = munmap (map_base,m_pageoffset + address + sizeof(data));
	if (rc<0) printf("munmap failed errno %d\n",errno);
	return 0;
}

int 
pci_bar::pci_read (unsigned int address, uint8_t*  data){
	void *map_base;
	void *virt_addr;
	int rc;
		
	if (address >= m_size)
	{
		printf("address out of available space max=0x%08x\n",m_size);
		return -1;
	}
	
	map_base = mmap (NULL, m_pageoffset + m_size , PROT_READ, MAP_SHARED, m_fd, (off_t)m_base);
	if (map_base == MAP_FAILED)
	{
		printf("mmap failed errno %d\n",errno);
		printf("args: NULL,0x%x,PROT_READ, MAP_SHARED, %d, 0x%x\n",m_pageoffset + m_size,m_fd,m_base);
		//return -1;
		exit(1);
	}
	
	virt_addr = map_base + address + m_pageoffset;
	
	*data = *(uint8_t*) virt_addr;
 
  #ifdef DEBUG
	printf("bar#%d: pci_read8 @0x%08x -> 0x%02x\n",m_bar,address,*data);
  #endif
	
	rc = munmap (map_base,m_pageoffset + address + sizeof(data));
	if (rc<0) printf("munmap failed errno %d\n",errno);
	return 0;
	
}

int 
pci_bar::pci_read (unsigned int address, uint16_t* data){
	void *map_base;
	void *virt_addr;
	int rc;
		
	if (address >= m_size)
	{
		printf("address out of available space max=0x%08x\n",m_size);
		return -1;
	}
	
	map_base = mmap (NULL, m_pageoffset + m_size , PROT_READ, MAP_SHARED, m_fd, (off_t)m_base);
	if (map_base == MAP_FAILED)
	{
		printf("mmap failed errno %d\n",errno);
		printf("args: NULL,0x%x,PROT_READ, MAP_SHARED, %d, 0x%x\n",m_pageoffset + m_size,m_fd,m_base);
		//return -1;
		exit(1);
	}
	
	virt_addr = map_base + address + m_pageoffset;
	
	*data = *(uint16_t*) virt_addr;

  #ifdef DEBUG
	printf("bar#%d: pci_read16 @0x%08x -> 0x%04x\n",m_bar,address,*data);
  #endif
  
	rc = munmap (map_base,m_pageoffset + address + sizeof(data));
	if (rc<0) printf("munmap failed errno %d\n",errno);
	return 0;
	
}

int 
pci_bar::pci_read (unsigned int address, uint32_t* data){
	void *map_base;
	void *virt_addr;
	int rc;
		
	if (address >= m_size)
	{
		printf("address out of available space max=0x%08x\n",m_size);
		return -1;
	}
	
	map_base = mmap (NULL, m_pageoffset + m_size , PROT_READ, MAP_SHARED, m_fd, (off_t)m_base);
	if (map_base == MAP_FAILED)
	{
		printf("mmap failed errno %d\n",errno);
		printf("args: NULL,0x%x,PROT_READ, MAP_SHARED, %d, 0x%x\n",m_pageoffset + m_size,m_fd,m_base);
		//return -1;
		exit(1);
	}
	
	virt_addr = map_base + address + m_pageoffset;
	
	*data = *(uint32_t*) virt_addr;
 
  #ifdef DEBUG
	printf("bar#%d: pci_read32 @0x%08x -> 0x%08x\n",m_bar,address,*data);
  #endif
	
	rc = munmap (map_base,m_pageoffset + address + sizeof(data));
	if (rc<0) printf("munmap failed errno %d\n",errno);
	return 0;
}
	
pciaddr_t
pci_bar::get_base_address(){
	printf("bar#%d: base address is 0x%016x\n",m_bar,(uint32_t)m_base + m_pageoffset);
	return m_base;
}
	
        
