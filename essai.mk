CC = g++
CFLAGS = -Wall 
LDFLAGS = -Wall 
LIB = -lpci

TARGET = pci_essai
SRCS = $(TARGET).cpp pci_bar.cpp
OBJS = $(SRCS:.cpp=.o)
 

$(TARGET): $(OBJS) 
	$(CC) $(LDFLAGS) $(OBJS) $(LIB) -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $< 


