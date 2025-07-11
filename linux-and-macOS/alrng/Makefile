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

CLANGSTD = -ansi
CFLAGS = -O2 -I$(IDIR) -Wall -Wextra
CPPFLAGS = $(CFLAGS) $(OPENSSL_SUPPORT_INC) -std=c++11
LDFLAGS = -lcrypto $(OPENSSL_SUPPORT_LIB)
LDCPPFLAGS = $(LDFLAGS) -lstdc++
SRCS=$(wildcard $(SDIR)/*.cpp)
OBJECTS = HmacSha1.o HealthTests.o UsbSerialDevice.o HmacSha256.o RsaCryptor.o RsaKeyRepo.o \
	ShaEntropyExtractor.o AlphaRngApiCWrapper.o Sha256.o AlphaRngApi.o HmacMD5.o AesCryptor.o \
	Sha512.o AppArguments.o RandomRangeSequence.o AlphaRandomRangeSequence.o


ALRNG = alrng
ALPERFTEST = alperftest
CPPSAMPLE = sample
CSAMPLE = sample_c
ALRNGDIAG = alrngdiag
ALRNG_PSERVER = run-alrng-pserver.sh
ALSEQGEN = alseqgen
BITGRPS = bitgrps

all: $(ALRNGDIAG) $(ALRNG) $(ALPERFTEST) $(CPPSAMPLE) $(CSAMPLE) $(ALSEQGEN) $(BITGRPS)

$(ALRNGDIAG): $(ALRNGDIAG).cpp $(OBJECTS)
	@echo
	@echo "Creating alrngdiag ..."
	$(CC) -c $(ALRNGDIAG).cpp $(CPPFLAGS)
	$(CC) $(ALRNGDIAG).o $(OBJECTS) -o $(ALRNGDIAG) $(LDCPPFLAGS)

$(ALRNG): $(ALRNG).cpp $(OBJECTS)
	@echo
	@echo "Creating alrng ..."
	$(CC) -c $(ALRNG).cpp $(CPPFLAGS)
	$(CC) $(ALRNG).o $(OBJECTS) -o $(ALRNG) $(LDCPPFLAGS)

$(ALPERFTEST): $(ALPERFTEST).cpp $(OBJECTS)
	@echo
	@echo "Creating alperftest ..."
	$(CC) -c $(ALPERFTEST).cpp $(CPPFLAGS)
	$(CC) $(ALPERFTEST).o $(OBJECTS) -o $(ALPERFTEST) $(LDCPPFLAGS)

$(CPPSAMPLE): $(CPPSAMPLE).cpp $(OBJECTS)
	@echo
	@echo "Creating sample ..."
	$(CC) -c $(CPPSAMPLE).cpp $(CPPFLAGS)
	$(CC) $(CPPSAMPLE).o $(OBJECTS) -o $(CPPSAMPLE) $(LDCPPFLAGS)

$(ALSEQGEN) : $(ALSEQGEN).cpp $(OBJECTS)
	@echo
	@echo "Creating alseqgen ..."
	$(CC) -c $(ALSEQGEN).cpp $(CPPFLAGS)
	$(CC) $(ALSEQGEN).o $(OBJECTS) -o $(ALSEQGEN) $(LDCPPFLAGS)

$(BITGRPS) : $(BITGRPS).cpp $(OBJECTS)
	@echo
	@echo "Creating bitgrps ..."
	$(CC) -c $(BITGRPS).cpp $(CPPFLAGS)
	$(CC) $(BITGRPS).o $(OBJECTS) -o $(BITGRPS) $(LDCPPFLAGS)

$(CSAMPLE): $(CSAMPLE).c $(OBJECTS)
	@echo
	@echo "Creating sample_c ..."
	$(CC) -c $(CSAMPLE).c $(CFLAGS) $(CLANGSTD)
	$(GPP) $(CSAMPLE).o $(OBJECTS) -o $(CSAMPLE) $(LDFLAGS)

HmacSha1.o:
	$(GPP) -c $(SDIR)/HmacSha1.cpp $(CPPFLAGS)

HealthTests.o:
	$(GPP) -c $(SDIR)/HealthTests.cpp $(CPPFLAGS)

UsbSerialDevice.o:
	$(GPP) -c $(SDIR)/UsbSerialDevice.cpp $(CPPFLAGS)

HmacSha256.o:
	$(GPP) -c $(SDIR)/HmacSha256.cpp $(CPPFLAGS)

RsaCryptor.o:
	$(GPP) -c $(SDIR)/RsaCryptor.cpp $(CPPFLAGS)

RsaKeyRepo.o:
	$(GPP) -c $(SDIR)/RsaKeyRepo.cpp $(CPPFLAGS)

ShaEntropyExtractor.o:
	$(GPP) -c $(SDIR)/ShaEntropyExtractor.cpp $(CPPFLAGS)

AlphaRngApiCWrapper.o:
	$(GPP) -c $(SDIR)/AlphaRngApiCWrapper.cpp $(CPPFLAGS)

Sha256.o:
	$(GPP) -c $(SDIR)/Sha256.cpp $(CPPFLAGS)

AlphaRngApi.o:
	$(GPP) -c $(SDIR)/AlphaRngApi.cpp $(CPPFLAGS)

HmacMD5.o:
	$(GPP) -c $(SDIR)/HmacMD5.cpp $(CPPFLAGS)

AesCryptor.o:
	$(GPP) -c $(SDIR)/AesCryptor.cpp $(CPPFLAGS)

Sha512.o:
	$(GPP) -c $(SDIR)/Sha512.cpp $(CPPFLAGS)

AppArguments.o:
	$(GPP) -c $(SDIR)/AppArguments.cpp $(CPPFLAGS)

RandomRangeSequence.o:
	$(GPP) -c $(SDIR)/RandomRangeSequence.cpp $(CPPFLAGS)

AlphaRandomRangeSequence.o:
	$(GPP) -c $(SDIR)/AlphaRandomRangeSequence.cpp $(CPPFLAGS)

clean:
	rm -fr *.o ; rm -fr $(ALRNGDIAG) $(ALRNG) $(CPPSAMPLE) $(ALPERFTEST) $(CSAMPLE) $(ALSEQGEN) $(BITGRPS)

install:
	install -d $(BINDIR)
	install $(ALRNGDIAG) $(BINDIR)/$(ALRNGDIAG)
	install $(ALRNG) $(BINDIR)/$(ALRNG)
	install $(ALPERFTEST) $(BINDIR)/$(ALPERFTEST)
	install $(ALSEQGEN) $(BINDIR)/$(ALSEQGEN)
	install $(BITGRPS) $(BINDIR)/$(BITGRPS) 
	cp $(ALRNG_PSERVER) $(BINDIR)/$(ALRNG_PSERVER)
	chmod a+x $(BINDIR)/$(ALRNG_PSERVER)

uninstall:
	rm $(BINDIR)/$(ALRNGDIAG)
	rm $(BINDIR)/$(ALRNG)
	rm $(BINDIR)/$(ALPERFTEST)
	rm $(BINDIR)/$(ALSEQGEN)
	rm $(BINDIR)/$(BITGRPS)
	rm $(BINDIR)/$(ALRNG_PSERVER)
