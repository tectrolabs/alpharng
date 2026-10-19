cmake_minimum_required(VERSION 3.10)
project(alrng LANGUAGES C CXX)

# Match the Makefile: Release builds use plain -O2, with no debug info and no -DNDEBUG.
# Replace CMake's default Release flags (-O3 -DNDEBUG), but leave any value the user set.
if(NOT WIN32)
  foreach(lang C CXX)
    if(CMAKE_${lang}_FLAGS_RELEASE STREQUAL "-O3 -DNDEBUG")
      set(CMAKE_${lang}_FLAGS_RELEASE "-O2" CACHE STRING "Flags used by the ${lang} compiler for Release builds" FORCE)
    endif()
  endforeach()
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Homebrew's OpenSSL is keg-only on macOS; probe the usual locations unless the user chose one.
if(APPLE AND NOT OPENSSL_ROOT_DIR)
  foreach(dir /opt/homebrew/opt/openssl /usr/local/opt/openssl)
    if(IS_DIRECTORY ${dir})
      set(OPENSSL_ROOT_DIR ${dir})
      break()
    endif()
  endforeach()
endif()
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Core library shared by all utilities
add_library(alpharng STATIC
  api-src/AesCryptor.cpp
  api-src/AlphaRandomRangeSequence.cpp
  api-src/AlphaRngApi.cpp
  api-src/AlphaRngApiCWrapper.cpp
  api-src/AppArguments.cpp
  api-src/HealthTests.cpp
  api-src/HmacMD5.cpp
  api-src/HmacSha1.cpp
  api-src/HmacSha256.cpp
  api-src/RandomRangeSequence.cpp
  api-src/RsaCryptor.cpp
  api-src/RsaKeyRepo.cpp
  api-src/Sha256.cpp
  api-src/Sha512.cpp
  api-src/ShaEntropyExtractor.cpp
  api-src/UsbSerialDevice.cpp
)
target_include_directories(alpharng PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/api-inc)
target_link_libraries(alpharng PUBLIC OpenSSL::Crypto Threads::Threads)
if(MSVC)
  target_compile_options(alpharng PRIVATE /W3)
else()
  target_compile_options(alpharng PUBLIC -Wall -Wextra)
endif()

# Utilities
set(ALRNG_TOOLS alrng alrngdiag alperftest alseqgen bitgrps)
foreach(tool ${ALRNG_TOOLS} sample)
  add_executable(${tool} ${tool}.cpp)
  target_link_libraries(${tool} PRIVATE alpharng)
endforeach()

# Plain C sample; linked through the C++ library, so force the C++ linker.
add_executable(sample_c sample_c.c)
target_link_libraries(sample_c PRIVATE alpharng)
set_target_properties(sample_c PROPERTIES LINKER_LANGUAGE CXX)

# Install / uninstall
include(GNUInstallDirs)
install(TARGETS ${ALRNG_TOOLS} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS run-alrng-pserver.sh DESTINATION ${CMAKE_INSTALL_BINDIR})

if(NOT TARGET uninstall)
  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -DMANIFEST=${CMAKE_BINARY_DIR}/install_manifest.txt
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake)
endif()

# OpenSSL provider that makes RAND_bytes() return bytes from an AlphaRNG device (for OpenSSL 4.0, which has no engines).
# Needs OpenSSL 4.0 or a 3.5.x release with OSSL_FUNC_PROVIDER_RANDOM_BYTES. Opt-in: -DALRNG_BUILD_PROVIDER=ON
option(ALRNG_BUILD_PROVIDER "Build the OpenSSL provider (alpharng.so)" OFF)
if(ALRNG_BUILD_PROVIDER)
  include(CheckSymbolExists)
  set(CMAKE_REQUIRED_INCLUDES ${OPENSSL_INCLUDE_DIR})
  check_symbol_exists(OSSL_FUNC_PROVIDER_RANDOM_BYTES openssl/core_dispatch.h ALRNG_OPENSSL_HAS_RANDOM_BYTES_HOOK)
  unset(CMAKE_REQUIRED_INCLUDES)
  if(NOT ALRNG_OPENSSL_HAS_RANDOM_BYTES_HOOK)
    message(FATAL_ERROR "The OpenSSL found (${OPENSSL_VERSION}) has no provider random bytes support. "
            "The provider needs OpenSSL 4.0 or a 3.5.x release that has OSSL_FUNC_PROVIDER_RANDOM_BYTES. "
            "Use -DOPENSSL_ROOT_DIR=<path> to choose another OpenSSL.")
  endif()
  # The provider is a shared module that includes the (static) alpharng library
  set_target_properties(alpharng PROPERTIES POSITION_INDEPENDENT_CODE ON)
  # OpenSSL looks for provider modules with the platform suffix when they are loaded by name
  if(APPLE)
    set(ALRNG_PROVIDER_SUFFIX ".dylib")
  else()
    set(ALRNG_PROVIDER_SUFFIX ".so")
  endif()
  add_library(alpharng_provider MODULE provider/alpharng_provider.cpp provider/rng_backend_device.cpp provider/settings.cpp)
  set_target_properties(alpharng_provider PROPERTIES PREFIX "" OUTPUT_NAME alpharng SUFFIX "${ALRNG_PROVIDER_SUFFIX}")
  target_link_libraries(alpharng_provider PRIVATE alpharng OpenSSL::Crypto)
endif()

# Unit tests, opt-in: cmake -S . -B build -DALRNG_BUILD_TESTS=ON && cmake --build build && (cd build && ctest --output-on-failure)
option(ALRNG_BUILD_TESTS "Build the unit tests" OFF)
if(ALRNG_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
