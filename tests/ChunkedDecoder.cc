#include <gtest/gtest.h>

#include "ChunkedDecoder.h"

std::vector<std::byte> operator"" _b(const char *str, std::size_t len) {
  std::vector<std::byte> result;
  result.reserve(len);
  for (std::size_t i = 0; i < len; ++i) {
    result.push_back(static_cast<std::byte>(str[i]));
  }
  return result;
}

class ChunkedDecoderTestHandler: public ChunkedDecoder::Handler {
  public:
    void on_chunk(Chunk && chunk) override {
      this->chunks.push_back(chunk);
    }

    void on_error() override {
      this->error = true;
    }

    void on_trailer_section(FieldCollection && field_collection) override {
      this->trailer_section = field_collection;
    }

    void on_finish() override {
      this->finished = true;
    }

    std::vector<Chunk> chunks;
    FieldCollection trailer_section;
    bool finished = false;
    bool error = false;
};

class ChunkedDecoderTest: public ::testing::Test {
  protected:
    ChunkedDecoder decoder;
    ChunkedDecoderTestHandler handler;

    ChunkedDecoderTest() {
      decoder.set_handler(handler);
    }
};

TEST_F(ChunkedDecoderTest, NoInput) {
  EXPECT_EQ(decoder.get_status(), ChunkedDecoder::Status::IN_PROGRESS);
  EXPECT_TRUE(handler.chunks.empty());
  EXPECT_TRUE(handler.trailer_section.empty());
  EXPECT_FALSE(handler.error);
  EXPECT_FALSE(handler.finished);
};

TEST_F(ChunkedDecoderTest, EmptyInput) {
  std::vector<std::byte> input{""_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::IN_PROGRESS);
  EXPECT_TRUE(result.remaining.empty());
  EXPECT_TRUE(handler.chunks.empty());
  EXPECT_TRUE(handler.trailer_section.empty());
  EXPECT_FALSE(handler.finished);
  EXPECT_FALSE(handler.error);
};

TEST_F(ChunkedDecoderTest, RepeatedEmptyInput) {
  std::vector<std::byte> input{""_b};
  for (int i = 0; i < 10; ++i) {
    ChunkedDecoder::FeedResult result{decoder.feed(input)};
    EXPECT_EQ(result.status, ChunkedDecoder::Status::IN_PROGRESS);
    EXPECT_TRUE(result.remaining.empty());
    EXPECT_TRUE(handler.chunks.empty());
    EXPECT_TRUE(handler.trailer_section.empty());
    EXPECT_FALSE(handler.finished);
    EXPECT_FALSE(handler.error);
  }
};

TEST_F(ChunkedDecoderTest, ImmediatelyInvalidInput) {
  std::vector<std::byte> input{"this is a bad input"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::INVALID);
  EXPECT_TRUE(std::equal(result.remaining.begin(), result.remaining.end(), input.cbegin(), input.cend()));
  EXPECT_TRUE(handler.chunks.empty());
  EXPECT_TRUE(handler.trailer_section.empty());
  EXPECT_FALSE(handler.finished);
  EXPECT_TRUE(handler.error);
};

TEST_F(ChunkedDecoderTest, NoData) {
  std::vector<std::byte> input{"0\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result.remaining.empty());
  ASSERT_EQ(handler.chunks.size(), 1);
  const Chunk * chunk{&handler.chunks[0]};
  EXPECT_TRUE(chunk->extensions.empty()); // extensions are not yet implemented
  EXPECT_TRUE(chunk->data.empty());
};

TEST_F(ChunkedDecoderTest, NoDataAndRepeatedEmptyData) {
  std::vector<std::byte> empty{""_b};
  for (int i = 0; i < 10; ++i) {
    ChunkedDecoder::FeedResult result{decoder.feed(empty)};
    EXPECT_EQ(result.status, ChunkedDecoder::Status::IN_PROGRESS);
    EXPECT_TRUE(result.remaining.empty());
    EXPECT_TRUE(handler.chunks.empty());
  }

  EXPECT_FALSE(handler.error);
  EXPECT_FALSE(handler.finished);

  std::vector<std::byte> input{"0\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result.remaining.empty());
  ASSERT_EQ(handler.chunks.size(), 1);
  const auto chunk{&handler.chunks[0]};
  EXPECT_TRUE(chunk->data.empty());
  EXPECT_TRUE(chunk->extensions.empty());

  EXPECT_FALSE(handler.error);
  EXPECT_TRUE(handler.finished);

  for (int i = 0; i < 10; ++i) {
    ChunkedDecoder::FeedResult result{decoder.feed(empty)};
    EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
    EXPECT_TRUE(result.remaining.empty());
    EXPECT_EQ(handler.chunks.size(), 1);
  }

  EXPECT_FALSE(handler.error);
  EXPECT_TRUE(handler.finished);
};

TEST_F(ChunkedDecoderTest, SplitEmptyData) {
  std::vector<std::byte> input1{"0\r"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input1)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::IN_PROGRESS);
  EXPECT_TRUE(result.remaining.empty());
  EXPECT_TRUE(handler.chunks.empty());
  EXPECT_FALSE(handler.finished);
  EXPECT_FALSE(handler.error);

  std::vector<std::byte> input2{"\n\r\n"_b};
  result = decoder.feed(input2);
  EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result.remaining.empty());

  ASSERT_EQ(handler.chunks.size(), 1);
  const Chunk * const chunk{&handler.chunks[0]};
  EXPECT_TRUE(chunk->data.empty());
  EXPECT_TRUE(chunk->extensions.empty());

  EXPECT_TRUE(handler.finished);
  EXPECT_FALSE(handler.error);
}

TEST_F(ChunkedDecoderTest, SingleChunk) {
  std::vector<std::byte> input{"4\r\nWiki\r\n0\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result.remaining.empty());
  ASSERT_EQ(handler.chunks.size(), 2);
  const Chunk * const chunk1{&handler.chunks[0]};
  EXPECT_EQ(chunk1->data, "Wiki"_b);
  EXPECT_TRUE(chunk1->extensions.empty());
  const Chunk * const chunk2{&handler.chunks[1]};
  EXPECT_EQ(chunk2->data, ""_b);
  EXPECT_TRUE(chunk2->extensions.empty());

  EXPECT_TRUE(handler.finished);
  EXPECT_FALSE(handler.error);
};

TEST_F(ChunkedDecoderTest, MultipleChunks) {
  std::vector<std::byte> input{"4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result.remaining.empty());

  ASSERT_EQ(handler.chunks.size(), 3);

  const Chunk * const chunk1{&handler.chunks[0]};
  EXPECT_EQ(chunk1->data, "Wiki"_b);
  EXPECT_TRUE(chunk1->extensions.empty());

  const Chunk * const chunk2{&handler.chunks[1]};
  EXPECT_EQ(chunk2->data, "pedia"_b);
  EXPECT_TRUE(chunk2->extensions.empty());

  const Chunk * const chunk3{&handler.chunks[2]};
  EXPECT_EQ(chunk3->data, ""_b);
  EXPECT_TRUE(chunk3->extensions.empty());

  EXPECT_TRUE(handler.finished);
  EXPECT_FALSE(handler.error);
};

TEST_F(ChunkedDecoderTest, EmptyChunkWithExtensions) {
  std::vector<std::byte> input{"0;ext1=value1;ext2=\"value2\"\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result.remaining.empty());

  ASSERT_EQ(handler.chunks.size(), 1);

  const Chunk * const chunk{&handler.chunks[0]};
  EXPECT_TRUE(chunk->data.empty());
  EXPECT_TRUE(chunk->extensions.empty()); // extensions are not yet implemented so they are skipped

  EXPECT_TRUE(handler.finished);
  EXPECT_FALSE(handler.error);
}

TEST_F(ChunkedDecoderTest, LowercaseChunkSize) {
  // The HTTP/1.1 spec requires chunk sizes to use uppercase lettering.
  std::vector<std::byte> input{"a\r\nWrong case\r\n0\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::INVALID);
  EXPECT_TRUE(std::equal(result.remaining.begin(), result.remaining.end(), input.cbegin(), input.cend()));

  EXPECT_EQ(handler.chunks.size(), 0);
  EXPECT_FALSE(handler.finished);
  EXPECT_TRUE(handler.error);
}

TEST_F(ChunkedDecoderTest, StrangeBodyCharacters) {
  // The chunked encoding should be able to handle any binary data within the chunk size
  std::vector<std::byte> input{"F\r\n\r\n\0\xffweird\\\"\a_b;\r\n0\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result.remaining.empty());

  EXPECT_TRUE(handler.finished);
  EXPECT_FALSE(handler.error);

  ASSERT_EQ(handler.chunks.size(), 2);

  const Chunk * const chunk1{&handler.chunks[0]};
  EXPECT_EQ(chunk1->data, "\r\n\0\xffweird\\\"\a_b;"_b);
  EXPECT_TRUE(chunk1->extensions.empty());

  const Chunk * const chunk2{&handler.chunks[1]};
  EXPECT_TRUE(chunk2->data.empty());
  EXPECT_TRUE(chunk2->extensions.empty());
}

TEST_F(ChunkedDecoderTest, SplitChunkSize) {
  std::vector<std::byte> input1{"1"_b};
  ChunkedDecoder::FeedResult result1{decoder.feed(input1)};
  EXPECT_EQ(result1.status, ChunkedDecoder::Status::IN_PROGRESS);
  EXPECT_TRUE(result1.remaining.empty());
  EXPECT_TRUE(handler.chunks.empty());
  EXPECT_FALSE(handler.error);
  EXPECT_FALSE(handler.finished);

  std::vector<std::byte> input2{"0\r\nabcdefghijklmnop\r\n0\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result2{decoder.feed(input2)};
  EXPECT_EQ(result2.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result2.remaining.empty());
  EXPECT_FALSE(handler.error);
  EXPECT_TRUE(handler.finished);

  ASSERT_EQ(handler.chunks.size(), 2);
  const Chunk * const chunk1{&handler.chunks[0]};
  EXPECT_EQ(chunk1->data, "abcdefghijklmnop"_b);
  EXPECT_TRUE(chunk1->extensions.empty());

  const Chunk * const chunk2{&handler.chunks[1]};
  EXPECT_EQ(chunk2->data, ""_b);
  EXPECT_TRUE(chunk1->extensions.empty());
}
