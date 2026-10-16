cmake_minimum_required(VERSION 3.10)
project(alrng LANGUAGES C CXX)

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
