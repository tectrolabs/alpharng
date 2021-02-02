CC=gcc
IDIR =./api-inc
SDIR =./api-src
CPPFLAGS= -O2 -I$(IDIR) -lssl -lcrypto -Wall -Wextra -lstdc++ -std=c++11


PREFIX = $(DESTDIR)/usr/local
BINDIR = $(PREFIX)/bin

OS:=$(shell uname -s)
ifeq ($(OS),Darwin)
OPENSSL_DIR = /usr/local/opt/openssl@1.1
OPENSSL_SUPPORT = -I$(OPENSSL_DIR)/include -L$(OPENSSL_DIR)/lib
CPPFLAGS += $(OPENSSL_SUPPORT) 
endif

SRCS=$(wildcard $(SDIR)/*.cpp)

ALRNGDIAG = alrngdiag
ALRNG = alrng
SAMPLE = sample

all: $(ALRNGDIAG) $(ALRNG) $(SAMPLE)

$(ALRNGDIAG): alrngdiag.cpp
	$(CC) alrngdiag.cpp $(SRCS) -o $(ALRNGDIAG) $(CPPFLAGS)

$(ALRNG): alrng.cpp
	$(CC) alrng.cpp $(SRCS) -o $(ALRNG) $(CPPFLAGS)

$(SAMPLE): sample.cpp
	$(CC) sample.cpp $(SRCS) -o $(SAMPLE) $(CPPFLAGS)

clean:
	rm -f *.o ; rm $(ALRNGDIAG) $(ALRNG) $(SAMPLE)

install:
	install $(ALRNGDIAG) $(BINDIR)/$(ALRNGDIAG)
	install $(ALRNG) $(BINDIR)/$(ALRNG)

uninstall:
	rm $(BINDIR)/$(ALRNGDIAG)
	rm $(BINDIR)/$(ALRNG)
