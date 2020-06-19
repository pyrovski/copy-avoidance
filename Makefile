CXXFLAGS= -g -O0 -std=c++11

TARGETS=sendfile

all: $(TARGETS)

clean:
	rm -f $(TARGETS)
