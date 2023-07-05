CC=gcc
GPP=g++
IDIR =./api-inc
SDIR =./api-src

PREFIX = $(DESTDIR)/usr/local
BINDIR = $(PREFIX)/bin

OS:=$(shell uname -s)
ifeq ($(OS),Darwin)
  ifneq ($(wildcard /usr/local/opt/openssl/.),)
   OPENSSL_DIR = /usr/local/opt/openssl
  else
   OPENSSL_DIR = /opt/homebrew/opt/openssl
  endif
OPENSSL_SUPPORT_INC = -I$(OPENSSL_DIR)/include
OPENSSL_SUPPORT_LIB = -L$(OPENSSL_DIR)/lib
endif

CPPFLAGS = -O2 -I$(IDIR) -Wall -Wextra $(OPENSSL_SUPPORT_INC) -std=c++11
CFLAGS = -O2 -I$(IDIR) -Wall -Wextra
LDFLAGS = -lcrypto $(OPENSSL_SUPPORT_LIB)
LDALLFLAGS = $(LDFLAGS) -lstdc++
ALLCPPLAGS = $(CPPFLAGS) $(LDALLFLAGS)
SRCS=$(wildcard $(SDIR)/*.cpp)
OBJECTS = ./HmacSha1.o ./HealthTests.o ./UsbSerialDevice.o ./HmacSha256.o ./RsaCryptor.o ./RsaKeyRepo.o \
	./ShaEntropyExtractor.o ./AlphaRngApiCWrapper.o ./Sha256.o ./AlphaRngApi.o ./HmacMD5.o ./AesCryptor.o \
	./Sha512.o ./AppArguments.o


ALRNG = alrng
ALPERFTEST = alperftest
CPPSAMPLE = sample
CSAMPLE = sample_c
ALRNGDIAG = alrngdiag
ALRNG_PSERVER = run-alrng-pserver.sh

all: $(ALRNGDIAG) $(ALRNG) $(CPPSAMPLE) $(ALPERFTEST) $(CSAMPLE)

$(ALRNGDIAG): alrngdiag.cpp
	$(CC) alrngdiag.cpp $(SRCS) -o $(ALRNGDIAG) $(ALLCPPLAGS)

$(ALRNG): alrng.cpp
	$(CC) alrng.cpp $(SRCS) -o $(ALRNG) $(ALLCPPLAGS)

$(CPPSAMPLE): sample.cpp
	$(CC) sample.cpp $(SRCS) -o $(CPPSAMPLE) $(ALLCPPLAGS)

$(CSAMPLE): sample_c.c
	${GPP} -c $(SRCS) $(CPPFLAGS)
	${CC} -c sample_c.c $(CFLAGS)
	${GPP} sample_c.o $(OBJECTS) -o $(CSAMPLE) $(LDFLAGS)

$(ALPERFTEST): alperftest.cpp
	$(CC) alperftest.cpp $(SRCS) -o $(ALPERFTEST) $(ALLCPPLAGS)

clean:
	rm -f *.o ; rm $(ALRNGDIAG) $(ALRNG) $(CPPSAMPLE) $(ALPERFTEST) $(CSAMPLE)

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
