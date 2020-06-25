CXXFLAGS= -g -O0 -std=c++11

TARGETS=sendfile read-send


all: $(TARGETS)

sendfile: sendfile.o tvUtil.o

read-send: read-send.o tvUtil.o

clean:
	rm -f $(TARGETS) *.o
