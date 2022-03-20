CC=gcc
IDIR =./api-inc
SDIR =./api-src
CPPFLAGS= -O2 -I$(IDIR) -lcrypto -Wall -Wextra -lstdc++ -std=c++11


PREFIX = $(DESTDIR)/usr/local
BINDIR = $(PREFIX)/bin

OS:=$(shell uname -s)
ifeq ($(OS),Darwin)
  ifneq ($(wildcard /usr/local/opt/openssl/.),)
   OPENSSL_DIR = /usr/local/opt/openssl
  else
   OPENSSL_DIR = /opt/homebrew/opt/openssl
  endif
OPENSSL_SUPPORT = -I$(OPENSSL_DIR)/include -L$(OPENSSL_DIR)/lib
CPPFLAGS += $(OPENSSL_SUPPORT) 
endif

SRCS=$(wildcard $(SDIR)/*.cpp)

ALRNGDIAG = alrngdiag
ALRNG = alrng
ALPERFTEST = alperftest
SAMPLE = sample
ALRNG_PSERVER = run-alrng-pserver.sh

all: $(ALRNGDIAG) $(ALRNG) $(SAMPLE) $(ALPERFTEST)

$(ALRNGDIAG): alrngdiag.cpp
	$(CC) alrngdiag.cpp $(SRCS) -o $(ALRNGDIAG) $(CPPFLAGS)

$(ALRNG): alrng.cpp
	$(CC) alrng.cpp $(SRCS) -o $(ALRNG) $(CPPFLAGS)

$(SAMPLE): sample.cpp
	$(CC) sample.cpp $(SRCS) -o $(SAMPLE) $(CPPFLAGS)

$(ALPERFTEST): alperftest.cpp
	$(CC) alperftest.cpp $(SRCS) -o $(ALPERFTEST) $(CPPFLAGS)

clean:
	rm -f *.o ; rm $(ALRNGDIAG) $(ALRNG) $(SAMPLE) $(ALPERFTEST)

install:

	install -d $(BINDIR)
	install $(ALRNGDIAG) $(BINDIR)/$(ALRNGDIAG)
	install $(ALRNG) $(BINDIR)/$(ALRNG)
	install $(ALPERFTEST) $(BINDIR)/$(ALPERFTEST)
	cp $(ALRNG_PSERVER) $(BINDIR)/$(ALRNG_PSERVER)
	chmod a+x $(BINDIR)/$(ALRNG_PSERVER)

uninstall:
	rm $(BINDIR)/$(ALRNGDIAG)
	rm $(BINDIR)/$(ALRNG)
	rm $(BINDIR)/$(ALPERFTEST)
	rm $(BINDIR)/$(ALRNG_PSERVER)
