#include <gtest/gtest.h>

#include "ChunkedDecoder.h"
#include "utils/byte_literals.h"

class ChunkedDecoderTestHandler: public ChunkedDecoder::Handler {
  public:
    void on_chunk(Chunk && chunk) override {
      this->chunks.push_back(chunk);
    }

    void on_error() override {
      ++this->on_error_call_count;
    }

    void on_trailer_section(FieldCollection && field_collection) override {
      this->trailer_section = field_collection;
    }

    void on_finish() override {
      ++this->on_finish_call_count;
    }

    std::vector<Chunk> chunks;
    FieldCollection trailer_section;

    int on_error_call_count{0};
    int on_finish_call_count{0};
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
  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 0);
};

TEST_F(ChunkedDecoderTest, EmptyInput) {
  const std::vector<std::byte> input{""_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::IN_PROGRESS);
  EXPECT_TRUE(result.remaining.empty());
  EXPECT_TRUE(handler.chunks.empty());
  EXPECT_TRUE(handler.trailer_section.empty());
  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_EQ(handler.on_error_call_count, 0);
};

TEST_F(ChunkedDecoderTest, RepeatedEmptyInput) {
  const std::vector<std::byte> input{""_b};
  for (int i = 0; i < 10; ++i) {
    ChunkedDecoder::FeedResult result{decoder.feed(input)};
    EXPECT_EQ(result.status, ChunkedDecoder::Status::IN_PROGRESS);
    EXPECT_TRUE(result.remaining.empty());
    EXPECT_TRUE(handler.chunks.empty());
    EXPECT_TRUE(handler.trailer_section.empty());
    EXPECT_EQ(handler.on_finish_call_count, 0);
    EXPECT_EQ(handler.on_error_call_count, 0);
  }
};

TEST_F(ChunkedDecoderTest, ImmediatelyInvalidInput) {
  const std::vector<std::byte> input{"this is a bad input"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::INVALID);
  EXPECT_TRUE(std::equal(result.remaining.begin(), result.remaining.end(), input.cbegin(), input.cend()));
  EXPECT_TRUE(handler.chunks.empty());
  EXPECT_TRUE(handler.trailer_section.empty());
  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_EQ(handler.on_error_call_count, 1);
};

TEST_F(ChunkedDecoderTest, NoData) {
  const std::vector<std::byte> input{"0\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result.remaining.empty());
  ASSERT_EQ(handler.chunks.size(), 1);
  const Chunk * chunk{&handler.chunks[0]};
  EXPECT_TRUE(chunk->extensions.empty());
  EXPECT_TRUE(chunk->data.empty());
};

TEST_F(ChunkedDecoderTest, NoDataAndRepeatedEmptyData) {
  const std::vector<std::byte> empty{""_b};
  for (int i = 0; i < 10; ++i) {
    ChunkedDecoder::FeedResult result{decoder.feed(empty)};
    EXPECT_EQ(result.status, ChunkedDecoder::Status::IN_PROGRESS);
    EXPECT_TRUE(result.remaining.empty());
    EXPECT_TRUE(handler.chunks.empty());
  }

  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 0);

  const std::vector<std::byte> input{"0\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result.remaining.empty());
  ASSERT_EQ(handler.chunks.size(), 1);
  const Chunk * const chunk{&handler.chunks[0]};
  EXPECT_TRUE(chunk->data.empty());
  EXPECT_TRUE(chunk->extensions.empty());

  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 1);

  for (int i = 0; i < 10; ++i) {
    ChunkedDecoder::FeedResult result{decoder.feed(empty)};
    EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
    EXPECT_TRUE(result.remaining.empty());
    EXPECT_EQ(handler.chunks.size(), 1);
  }

  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 1);
};

TEST_F(ChunkedDecoderTest, SplitEmptyData) {
  const std::vector<std::byte> input1{"0\r"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input1)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::IN_PROGRESS);
  EXPECT_TRUE(result.remaining.empty());
  EXPECT_TRUE(handler.chunks.empty());
  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_EQ(handler.on_error_call_count, 0);

  const std::vector<std::byte> input2{"\n\r\n"_b};
  result = decoder.feed(input2);
  EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result.remaining.empty());

  ASSERT_EQ(handler.chunks.size(), 1);
  const Chunk * const chunk{&handler.chunks[0]};
  EXPECT_TRUE(chunk->data.empty());
  EXPECT_TRUE(chunk->extensions.empty());

  EXPECT_EQ(handler.on_finish_call_count, 1);
  EXPECT_EQ(handler.on_error_call_count, 0);
}

TEST_F(ChunkedDecoderTest, ExtraData) {
  const std::vector<std::byte> input{"0\r\n\r\nthis should not be read by the decoder"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
  const std::vector<std::byte> expected_remaining{"this should not be read by the decoder"_b};
  EXPECT_TRUE(std::equal(result.remaining.begin(), result.remaining.end(), expected_remaining.cbegin(), expected_remaining.cend()));
}

TEST_F(ChunkedDecoderTest, SingleChunk) {
  const std::vector<std::byte> input{"4\r\nWiki\r\n0\r\n\r\n"_b};
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

  EXPECT_EQ(handler.on_finish_call_count, 1);
  EXPECT_EQ(handler.on_error_call_count, 0);
};

TEST_F(ChunkedDecoderTest, MultipleChunks) {
  const std::vector<std::byte> input{"4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n"_b};
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

  EXPECT_EQ(handler.on_finish_call_count, 1);
  EXPECT_EQ(handler.on_error_call_count, 0);
};

TEST_F(ChunkedDecoderTest, EmptyChunkWithExtensions) {
  const std::vector<std::byte> input{"0;ext1=value1;ext2=\"value2\"\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result.remaining.empty());

  ASSERT_EQ(handler.chunks.size(), 1);

  const Chunk * const chunk{&handler.chunks[0]};
  EXPECT_TRUE(chunk->data.empty());
  ASSERT_EQ(chunk->extensions.size(), 2);
  EXPECT_EQ(chunk->extensions[0].name, "ext1");
  EXPECT_EQ(chunk->extensions[0].val, "value1");
  EXPECT_EQ(chunk->extensions[1].name, "ext2");
  EXPECT_EQ(chunk->extensions[1].val, "value2");

  EXPECT_EQ(handler.on_finish_call_count, 1);
  EXPECT_EQ(handler.on_error_call_count, 0);
}

TEST_F(ChunkedDecoderTest, EmptyChunkWithBWS) {
  const std::vector<std::byte> input{"0 ;  ext1   =    val1\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result.remaining.empty());

  ASSERT_EQ(handler.chunks.size(), 1);

  const Chunk * const chunk{&handler.chunks[0]};
  EXPECT_TRUE(chunk->data.empty());
  ASSERT_EQ(chunk->extensions.size(), 1);
  EXPECT_EQ(chunk->extensions[0].name, "ext1");
  EXPECT_EQ(chunk->extensions[0].val, "val1");

  EXPECT_EQ(handler.on_finish_call_count, 1);
  EXPECT_EQ(handler.on_error_call_count, 0);
}

TEST_F(ChunkedDecoderTest, ChunkDataStartAfterExtSemicolon) {
  const std::vector<std::byte> input{"0;\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::INVALID);

  const std::vector<std::byte> expected_remaining{"\r\n\r\n"_b};
  EXPECT_TRUE(std::equal(input.cbegin() + 2, input.cend(), expected_remaining.cbegin(), expected_remaining.cend()));

  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_EQ(handler.on_error_call_count, 1);
}

TEST_F(ChunkedDecoderTest, ChunkDataStartAfterExtEquals) {
  const std::vector<std::byte> input{"0;ext=\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::INVALID);

  const std::vector<std::byte> expected_remaining{"\r\n\r\n"_b};
  EXPECT_TRUE(std::equal(input.cbegin() + 6, input.cend(), expected_remaining.cbegin(), expected_remaining.cend()));

  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_EQ(handler.on_error_call_count, 1);
}

TEST_F(ChunkedDecoderTest, LowercaseChunkSize) {
  // The HTTP/1.1 spec requires chunk sizes to use uppercase lettering.
  const std::vector<std::byte> input{"a\r\nWrong case\r\n0\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::INVALID);
  EXPECT_TRUE(std::equal(result.remaining.begin(), result.remaining.end(), input.cbegin(), input.cend()));

  EXPECT_EQ(handler.chunks.size(), 0);
  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_EQ(handler.on_error_call_count, 1);
}

TEST_F(ChunkedDecoderTest, StrangeBodyCharacters) {
  // The chunked encoding should be able to handle any binary data within the chunk size
  const std::vector<std::byte> input{"F\r\n\r\n\0\xffweird\\\"\a_b;\r\n0\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result{decoder.feed(input)};
  EXPECT_EQ(result.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result.remaining.empty());

  EXPECT_EQ(handler.on_finish_call_count, 1);
  EXPECT_EQ(handler.on_error_call_count, 0);

  ASSERT_EQ(handler.chunks.size(), 2);

  const Chunk * const chunk1{&handler.chunks[0]};
  EXPECT_EQ(chunk1->data, "\r\n\0\xffweird\\\"\a_b;"_b);
  EXPECT_TRUE(chunk1->extensions.empty());

  const Chunk * const chunk2{&handler.chunks[1]};
  EXPECT_TRUE(chunk2->data.empty());
  EXPECT_TRUE(chunk2->extensions.empty());
}

TEST_F(ChunkedDecoderTest, SplitChunkSize) {
  const std::vector<std::byte> input1{"1"_b};
  ChunkedDecoder::FeedResult result1{decoder.feed(input1)};
  EXPECT_EQ(result1.status, ChunkedDecoder::Status::IN_PROGRESS);
  EXPECT_TRUE(result1.remaining.empty());
  EXPECT_TRUE(handler.chunks.empty());
  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 0);

  const std::vector<std::byte> input2{"0\r\nabcdefghijklmnop\r\n0\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result2{decoder.feed(input2)};
  EXPECT_EQ(result2.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result2.remaining.empty());
  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 1);

  ASSERT_EQ(handler.chunks.size(), 2);
  const Chunk * const chunk1{&handler.chunks[0]};
  EXPECT_EQ(chunk1->data, "abcdefghijklmnop"_b);
  EXPECT_TRUE(chunk1->extensions.empty());

  const Chunk * const chunk2{&handler.chunks[1]};
  EXPECT_EQ(chunk2->data, ""_b);
  EXPECT_TRUE(chunk1->extensions.empty());
}

TEST_F(ChunkedDecoderTest, SplitChunkBody) {
  const std::vector<std::byte> input1{"2\r\n0"_b};
  ChunkedDecoder::FeedResult result1{decoder.feed(input1)};
  EXPECT_EQ(result1.status, ChunkedDecoder::Status::IN_PROGRESS);
  EXPECT_TRUE(result1.remaining.empty());
  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_TRUE(handler.chunks.empty());

  const std::vector<std::byte> input2{"1\r\n0\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result2{decoder.feed(input2)};
  EXPECT_EQ(result2.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result2.remaining.empty());
  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 1);

  ASSERT_EQ(handler.chunks.size(), 2);
  const Chunk * const chunk1{&handler.chunks[0]};
  EXPECT_EQ(chunk1->data, "01"_b);
  EXPECT_TRUE(chunk1->extensions.empty());

  const Chunk * const chunk2{&handler.chunks[1]};
  EXPECT_TRUE(chunk2->data.empty());
  EXPECT_TRUE(chunk2->extensions.empty());
}

TEST_F(ChunkedDecoderTest, SplitChunkBodyWithEmptyFeed) {
  const std::vector<std::byte> input1{"2\r\n0"_b};
  ChunkedDecoder::FeedResult result1{decoder.feed(input1)};
  EXPECT_EQ(result1.status, ChunkedDecoder::Status::IN_PROGRESS);
  EXPECT_TRUE(result1.remaining.empty());
  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_TRUE(handler.chunks.empty());

  const std::vector<std::byte> input2{""_b};
  ChunkedDecoder::FeedResult result2{decoder.feed(input2)};
  EXPECT_EQ(result2.status, ChunkedDecoder::Status::IN_PROGRESS);
  EXPECT_TRUE(result2.remaining.empty());
  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 0);

  const std::vector<std::byte> input3{"1\r\n0\r\n\r\n"_b};
  ChunkedDecoder::FeedResult result3{decoder.feed(input3)};
  EXPECT_EQ(result3.status, ChunkedDecoder::Status::DONE);
  EXPECT_TRUE(result3.remaining.empty());
  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 1);

  ASSERT_EQ(handler.chunks.size(), 2);
  const Chunk * const chunk1{&handler.chunks[0]};
  EXPECT_EQ(chunk1->data, "01"_b);
  EXPECT_TRUE(chunk1->extensions.empty());

  const Chunk * const chunk2{&handler.chunks[1]};
  EXPECT_TRUE(chunk2->data.empty());
  EXPECT_TRUE(chunk2->extensions.empty());
}
