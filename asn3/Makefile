CC = gcc
CFLAGS = -Wall -g -fpic -std=c99 -O2
INCLUDE = -I ~pn-cs453/Given/Asgn3/include -I ~/csc453/asn3
LIBPATH = -L ~pn-cs453/Given/Asgn3/lib64 -L ~/csc453/asn3

LIBS = -lpthread -lrt

TARGET = dine

SRC = dine.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBPATH) -o $(TARGET) $(SRC) $(LIBS)

dine: $(SRC)
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBPATH) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)
	rm -f *.o
	rm -f *~
	rm -f core*
	rm -f *.dSYM