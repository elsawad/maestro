#include <gtest/gtest.h>
#include <sstream>

#include "BasicOstreamSink.h"

class ostringstreamSinkTest: public ::testing::Test {
  protected:
    std::ostringstream oss;
    BasicOstreamSink<char> cbos{oss};
};

class wostringstreamSinkTest: public ::testing::Test {
  protected:
    std::wostringstream woss;
    BasicOstreamSink<wchar_t> wcbos{woss};
};

TEST_F(ostringstreamSinkTest, Finish) {
  // Test that finish flushes the stream
  cbos.finish();
  EXPECT_EQ(oss.str(), "");
}

TEST_F(ostringstreamSinkTest, WriteEmptyData) {
  // Test that writing empty data does not change the stream
  const char *data = "";
  cbos.write(data, strlen(data));
  cbos.finish();
  EXPECT_EQ(oss.str(), "");
}

TEST_F(ostringstreamSinkTest, WriteData) {
  // Test that writing empty data followed by finish does not change the stream
  const char *data = "example-data_123";
  cbos.write(data, strlen(data));
  cbos.finish();
  EXPECT_EQ(oss.str(), "example-data_123");
}

TEST_F(ostringstreamSinkTest, WriteLessData) {
  // Test that writing less data than the size parameter works correctly
  const char *data = "example-data_123";
  cbos.write(data, 7); // Write only "example", without the null terminator
  cbos.finish();
  EXPECT_EQ(oss.str(), "example"); // oss.str() adds a null terminator, and string literals have implicit null terminators
}

TEST_F(ostringstreamSinkTest, WriteMultipleTimesAndFinish) {
  // Test that multiple writes followed by finish works correctly
  const char *data1 = "Hello, ";
  cbos.write(data1, strlen(data1));
  const char *data2 = "World!";
  cbos.write(data2, strlen(data2));
  cbos.finish();
  EXPECT_EQ(oss.str(), "Hello, World!");
}

TEST_F(ostringstreamSinkTest, WriteNullData) {
  // Test that writing null data does not change the stream
  const char *data = nullptr;
  cbos.write(data, 0);
  cbos.finish();
  EXPECT_EQ(oss.str(), "");
}

TEST_F(ostringstreamSinkTest, WriteLargeData) {
  // Test that writing large data works correctly
  std::string data(10000, 'A');
  cbos.write(data.c_str(), data.size());
  cbos.finish();
  EXPECT_EQ(oss.str(), data);
}

TEST_F(ostringstreamSinkTest, WriteAfterFinish) {
  // Test that writing after finish throws an exception
  const char *data1 = "Hello, ";
  cbos.write(data1, strlen(data1));
  cbos.finish();
  EXPECT_EQ(oss.str(), "Hello, ");

  const char *data2 = "World!";
  cbos.write(data2, strlen(data2));
  cbos.finish();
  EXPECT_EQ(oss.str(), "Hello, World!"); // writing after finish should work for basic_ostream instances
}

TEST_F(ostringstreamSinkTest, FinishMultipleTimes) {
  // Test that calling finish multiple times does not change the stream
  const char *data = "example-data_123";
  cbos.write(data, strlen(data));
  cbos.finish();
  EXPECT_EQ(oss.str(), "example-data_123");

  EXPECT_NO_THROW(cbos.finish());
  EXPECT_EQ(oss.str(), "example-data_123");
}

TEST_F(wostringstreamSinkTest, WriteData) {
  // Test that writing wide data works correctly
  const wchar_t *data = L"example-wide-data_123";
  wcbos.write(data, wcslen(data));
  wcbos.finish();
  EXPECT_EQ(woss.str(), L"example-wide-data_123");
}
