CXX=clang++
CC=${CXX}
CXXFLAGS=-g -O3 -std=c++11 -DNDEBUG
FLATBUFFER_INC=/snap/flatbuffers/current/include
CHANNEL_INC=/usr/local/include/cppchannel
TARGETS=sendfile read-send read-send-pipeline seekable seek-client mmap mmap_crc32
INSTALL_DEST=$(HOME)

all: $(TARGETS)

sendfile: sendfile.o tvUtil.o

read-send: read-send.o tvUtil.o

req_generated.h: req.fbs
	flatc -c $^

seekable.o: CXXFLAGS+=-I$(FLATBUFFER_INC) -I$(CHANNEL_INC)
seekable.o: req_generated.h

seekable: seekable.o 
	$(CC) -o $@ $^ -lpthread -latomic

seek-client.o: CXXFLAGS+=-I$(FLATBUFFER_INC)
seek-client.o: req_generated.h

seek-client: seek-client.o
	$(CC) -o $@ $^ -lpthread -latomic

read-send-pipeline: read-send-pipeline.o tvUtil.o
	$(CC) -o $@ $^ -lboost_context -lboost_fiber -lpthread -latomic

mmap.o: CXXFLAGS+=-I$(FLATBUFFER_INC) -I$(CHANNEL_INC)
mmap.o: req_generated.h
mmap: mmap.o tvUtil.o
	$(CC) -o $@ $^ -lpthread -latomic

mmap_crc32.o: CXXFLAGS+=-I$(FLATBUFFER_INC) -I$(CHANNEL_INC)
mmap_crc32.o: req_generated.h
mmap_crc32: mmap_crc32.o tvUtil.o crcutil_blockword.o
	$(CC) -o $@ $^ -lpthread -latomic

clean:
	rm -f $(TARGETS) *.o *_generated.h

install:
	install -d $(INSTALL_DEST)/local/share/seekable
	install -m 0644 -t $(INSTALL_DEST)/local/share/seekable/ req.fbs
