#include <gtest/gtest.h>
#include <sstream>

#include "BasicOstreamSink.h"
#include "ChunkedEncoder.h"
#include "ChunkedDecoder.h"

class ChunkedEncoderTest: public ::testing::Test {
  protected:
    std::ostringstream oss;
    ChunkedEncoder encoder{std::make_unique<BasicOstreamSink<char>>(oss)};
};

TEST_F(ChunkedEncoderTest, nullptrZeroSize) {
  // Test that writing nullptr with size 0 does not produce any output
  encoder.write(nullptr, 0);
  encoder.finish();
  EXPECT_EQ(oss.str(), "0\r\n");
};

TEST_F(ChunkedEncoderTest, nullptrNonZeroSize) {
  // Test that writing nullptr with non-zero size throws an exception
  EXPECT_THROW(encoder.write(nullptr, 1), std::invalid_argument);
  EXPECT_THROW(encoder.write(nullptr, 100), std::invalid_argument);
};

TEST_F(ChunkedEncoderTest, EmptyData) {
  // Test that writing empty data does not produce any output
  const char *data = "";
  encoder.write(data, strlen(data));
  encoder.finish();
  EXPECT_EQ(oss.str(), "0\r\n");
};

TEST_F(ChunkedEncoderTest, HelloWorld) {
  const char *data = "Hello, World!";
  encoder.write(data, strlen(data));
  encoder.finish();
  EXPECT_EQ(oss.str(), "D\r\nHello, World!\r\n0\r\n");
};

TEST_F(ChunkedEncoderTest, MultipleWrites) {
  const char *data1 = "Hello, ";
  encoder.write(data1, strlen(data1));
  const char *data2 = "World!";
  encoder.write(data2, strlen(data2));
  encoder.finish();
  EXPECT_EQ(oss.str(), "7\r\nHello, \r\n6\r\nWorld!\r\n0\r\n");
};

TEST_F(ChunkedEncoderTest, WriteZeroWrite) {
  const char *data1 = "Hello";
  encoder.write(data1, strlen(data1));
  encoder.write(nullptr, 0); // null pointers are not handled by ChunkedEncoder, but rather by the caller
  const char *data2 = ", World!";
  encoder.write(data2, strlen(data2));
  encoder.write(nullptr, 0);
  encoder.finish();
  EXPECT_EQ(oss.str(), "5\r\nHello\r\n8\r\n, World!\r\n0\r\n");
};

TEST_F(ChunkedEncoderTest, WriteLessData) {
  const char *data = "Hello, World!";
  encoder.write(data, 5); // Write only "Hello"
  encoder.finish();
  EXPECT_EQ(oss.str(), "5\r\nHello\r\n0\r\n");
};

TEST_F(ChunkedEncoderTest, FinishWithoutWrite) {
  encoder.finish();
  EXPECT_EQ(oss.str(), "0\r\n");
};

TEST_F(ChunkedEncoderTest, WriteAfterFinishThrows) {
  encoder.finish();
  const char *data = "Hello, World!";
  EXPECT_THROW(encoder.write(data, strlen(data)), std::logic_error); // This should not produce any output
};

TEST_F(ChunkedEncoderTest, MultipleFinishes) {
  const char *data = "Hello, World!";
  encoder.write(data, strlen(data));
  encoder.finish();
  encoder.finish(); // This should not produce any additional output
  EXPECT_EQ(oss.str(), "D\r\nHello, World!\r\n0\r\n");
};

TEST_F(ChunkedEncoderTest, WriteLargeData) {
  std::string large_data(1024, 'A'); // 1 KB of 'A'
  encoder.write(large_data.data(), large_data.size());
  encoder.finish();
  std::stringstream expected_ss;
  expected_ss << std::uppercase << std::hex << large_data.size() << "\r\n" << large_data << "\r\n0\r\n";
  EXPECT_EQ(oss.str(), expected_ss.str());
};
