CXXFLAGS= -g -O0 -std=c++11

TARGETS=sendfile

sendfile: sendfile.o tvUtil.o

all: $(TARGETS)

clean:
	rm -f $(TARGETS) *.o
