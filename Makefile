CC=g++
CXXFLAGS=-g -O3 -std=c++11

TARGETS=sendfile read-send read-send-pipeline

all: $(TARGETS)

sendfile: sendfile.o tvUtil.o

read-send: read-send.o tvUtil.o

read-send-pipeline: read-send-pipeline.o tvUtil.o
	$(CC) -o $@ $^ -lboost_context -lboost_fiber -lpthread

clean:
	rm -f $(TARGETS) *.o
