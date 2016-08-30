CC = g++
CFLAGS = 
LDFLAGS =  
LIB = -lpci

TARGET = pci_main
SRCS = $(TARGET).cpp pci_bar.cpp
OBJS = $(SRCS:.cpp=.o)
 

$(TARGET): $(OBJS) 
	$(CC) $(LDFLAGS) $(OBJS) $(LIB) -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $< 


