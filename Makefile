CC=g++
CXXFLAGS=-g -O0 -std=c++11 -I/snap/flatbuffers/current/include -DNDEBUG

TARGETS=sendfile read-send read-send-pipeline seekable seek-client

all: $(TARGETS)

sendfile: sendfile.o tvUtil.o

read-send: read-send.o tvUtil.o

req_generated.h: req.fbs
	flatc -c $^

seekable: seekable.o req_generated.h
	$(CC) -o $@ $(patsubst %.h,,$^) -lboost_context -lboost_fiber -lpthread

seek-client: seek-client.o
	$(CC) -o $@ $^ -lboost_context -lboost_fiber -lpthread

read-send-pipeline: read-send-pipeline.o tvUtil.o
	$(CC) -o $@ $^ -lboost_context -lboost_fiber -lpthread

clean:
	rm -f $(TARGETS) *.o
