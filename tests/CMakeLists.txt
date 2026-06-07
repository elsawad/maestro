include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)

add_executable(tests
  BasicOstreamSink.cc
  ChunkedDecoder.cc
  ChunkedEncoder.cc
  TokenParser.cc
  QuotedStringParser.cc
)

target_link_libraries(tests
  PRIVATE
    maestro
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)
