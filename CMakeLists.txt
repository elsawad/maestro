cmake_minimum_required(VERSION 3.28)

project(maestro)

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(maestro)

target_sources(maestro
  PRIVATE
    src/ABNF.cc
    src/CharacterClass.cc
    src/ChunkedDecoder.cc
    src/ChunkedDecoderSink.cc
    src/ChunkedEncoder.cc
    src/Encoder.cc
    src/Field.cc
    src/FieldCollection.cc
    src/FieldCollectionParser.cc
    src/HTTPMessageBody.cc
    src/HTTPRequest.cc
    src/HTTPResponse.cc
    src/QuotedStringParser.cc
    src/Server.cc
    src/ServerConfig.cc
    src/ServerConfigBuilder.cc
    src/ServerConfigCLIParser.cc
    src/TokenParser.cc
)

target_include_directories(maestro
  PUBLIC include
  PRIVATE src
)

enable_testing()

add_subdirectory(tests)
add_subdirectory(examples)
