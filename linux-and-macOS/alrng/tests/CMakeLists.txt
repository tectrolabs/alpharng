# Each test source file becomes its own executable and its own CTest test.
set(ALRNG_TESTS sha hmac aes rsa health range_sequence app_arguments)

foreach(name ${ALRNG_TESTS})
  add_executable(test_${name} test_${name}.cpp)
  target_include_directories(test_${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(test_${name} PRIVATE alpharng)
  add_test(NAME ${name} COMMAND test_${name})
endforeach()
