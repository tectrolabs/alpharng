# Each test source file becomes its own executable and its own CTest test.
set(ALRNG_TESTS sha hmac aes rsa health range_sequence app_arguments)

foreach(name ${ALRNG_TESTS})
  add_executable(test_${name} test_${name}.cpp)
  target_include_directories(test_${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(test_${name} PRIVATE alpharng)
  add_test(NAME ${name} COMMAND test_${name})
endforeach()

# Tests of the OpenSSL provider. They load a stand-in build of the provider (a repeatable byte stream instead of a
# device), so they need no hardware. Each test program has its own OpenSSL configuration, so each is separate.
if(ALRNG_BUILD_PROVIDER)
  add_library(alpharng_provider_sim MODULE
    ${CMAKE_CURRENT_SOURCE_DIR}/../provider/alpharng_provider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../provider/settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rng_backend_sim.cpp)
  set_target_properties(alpharng_provider_sim PROPERTIES PREFIX "" OUTPUT_NAME alpharng_sim SUFFIX "${ALRNG_PROVIDER_SUFFIX}")
  target_include_directories(alpharng_provider_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../provider)
  target_link_libraries(alpharng_provider_sim PRIVATE OpenSSL::Crypto)

  set(ALRNG_PROVIDER_TESTS provider_config provider_threads provider_first_call provider_api provider_unset
      provider_read_fail provider_open_fail provider_reentrant provider_options provider_bad_setting)
  foreach(name ${ALRNG_PROVIDER_TESTS})
    add_executable(test_${name} test_${name}.cpp)
    target_include_directories(test_${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_${name} PRIVATE OpenSSL::Crypto Threads::Threads)
    target_compile_definitions(test_${name} PRIVATE ALRNG_SIM_PROVIDER_PATH="$<TARGET_FILE:alpharng_provider_sim>")
    add_dependencies(test_${name} alpharng_provider_sim)
    add_test(NAME ${name} COMMAND test_${name})
    set_tests_properties(${name} PROPERTIES TIMEOUT 60)
  endforeach()

  # The settings parser does not use OpenSSL, so its test is a plain program
  add_executable(test_provider_settings test_provider_settings.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../provider/settings.cpp)
  target_include_directories(test_provider_settings PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../provider)
  add_test(NAME provider_settings COMMAND test_provider_settings)
endif()
