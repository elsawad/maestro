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
      this->decoder.set_handler(handler);
    }
};

TEST_F(ChunkedDecoderTest, NoInput) {
  EXPECT_EQ(decoder.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_TRUE(handler.chunks.empty());
  EXPECT_TRUE(handler.trailer_section.empty());
  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 0);
};

TEST_F(ChunkedDecoderTest, EmptyInput) {
  const std::vector<std::byte> input{""_b};
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed, input.size());
  EXPECT_TRUE(handler.chunks.empty());
  EXPECT_TRUE(handler.trailer_section.empty());
  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_EQ(handler.on_error_call_count, 0);
};

TEST_F(ChunkedDecoderTest, RepeatedEmptyInput) {
  const std::vector<std::byte> input{""_b};
  for (int i = 0; i < 10; ++i) {
    const std::size_t consumed{decoder.feed(input)};
    EXPECT_EQ(decoder.state(), FeedState::NEED_MORE_INPUT);
    EXPECT_EQ(consumed, input.size());
    EXPECT_TRUE(handler.chunks.empty());
    EXPECT_TRUE(handler.trailer_section.empty());
    EXPECT_EQ(handler.on_finish_call_count, 0);
    EXPECT_EQ(handler.on_error_call_count, 0);
  }
};

TEST_F(ChunkedDecoderTest, ImmediatelyInvalidInput) {
  const std::vector<std::byte> input{"this is a bad input"_b};
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::ERROR);
  EXPECT_EQ(consumed, 0);
  EXPECT_TRUE(handler.chunks.empty());
  EXPECT_TRUE(handler.trailer_section.empty());
  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_EQ(handler.on_error_call_count, 1);
};

TEST_F(ChunkedDecoderTest, NoData) {
  const std::vector<std::byte> input{"0\r\n\r\n"_b};
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, input.size());
  ASSERT_EQ(handler.chunks.size(), 1);
  const Chunk * chunk{&handler.chunks[0]};
  EXPECT_TRUE(chunk->extensions.empty());
  EXPECT_TRUE(chunk->data.empty());
};

TEST_F(ChunkedDecoderTest, NoDataAndRepeatedEmptyData) {
  const std::vector<std::byte> empty{""_b};
  for (int i = 0; i < 10; ++i) {
    const std::size_t consumed{decoder.feed(empty)};
    EXPECT_EQ(decoder.state(), FeedState::NEED_MORE_INPUT);
    EXPECT_EQ(consumed, 0);
    EXPECT_TRUE(handler.chunks.empty());
  }

  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 0);

  const std::vector<std::byte> input{"0\r\n\r\n"_b};
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, 5);
  ASSERT_EQ(handler.chunks.size(), 1);
  const Chunk * const chunk{&handler.chunks[0]};
  EXPECT_TRUE(chunk->data.empty());
  EXPECT_TRUE(chunk->extensions.empty());

  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 1);

  for (int i = 0; i < 10; ++i) {
    const std::size_t consumed{decoder.feed(empty)};
    EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
    EXPECT_EQ(consumed, 0);
    EXPECT_EQ(handler.chunks.size(), 1);
  }

  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 1);
};

TEST_F(ChunkedDecoderTest, SplitEmptyData) {
  const std::vector<std::byte> input1{"0\r"_b};
  const std::size_t consumed1{decoder.feed(input1)};
  EXPECT_EQ(decoder.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed1, input1.size());
  EXPECT_TRUE(handler.chunks.empty());
  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_EQ(handler.on_error_call_count, 0);

  const std::vector<std::byte> input2{"\n\r\n"_b};
  const std::size_t consumed2{decoder.feed(input2)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed2, input2.size());

  ASSERT_EQ(handler.chunks.size(), 1);
  const Chunk * const chunk{&handler.chunks[0]};
  EXPECT_TRUE(chunk->data.empty());
  EXPECT_TRUE(chunk->extensions.empty());

  EXPECT_EQ(handler.on_finish_call_count, 1);
  EXPECT_EQ(handler.on_error_call_count, 0);
}

TEST_F(ChunkedDecoderTest, ExtraData) {
  const std::vector<std::byte> input{"0\r\n\r\nthis should not be read by the decoder"_b};
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, 5);
}

TEST_F(ChunkedDecoderTest, SingleChunk) {
  const std::vector<std::byte> input{"4\r\nWiki\r\n0\r\n\r\n"_b};
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, input.size());
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
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, input.size());

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
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, input.size());

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
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, input.size());

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
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::ERROR);
  EXPECT_EQ(consumed, 2);

  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_EQ(handler.on_error_call_count, 1);
}

TEST_F(ChunkedDecoderTest, ChunkDataStartAfterExtName) {
  const std::vector<std::byte> input{"0;ext\r\n\r\n"_b};
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, input.size());

  ASSERT_EQ(handler.chunks.size(), 1);
  ASSERT_EQ(handler.chunks[0].extensions.size(), 1);
  EXPECT_EQ(handler.chunks[0].extensions[0].name, "ext");
  EXPECT_EQ(handler.chunks[0].extensions[0].val, std::nullopt);

  EXPECT_EQ(handler.on_finish_call_count, 1);
  EXPECT_EQ(handler.on_error_call_count, 0);
}

TEST_F(ChunkedDecoderTest, NoValueExtensionThenValueExtension) {
  const std::vector<std::byte> input{"0;ext1;ext2=\"val2\"\r\n\r\n"_b};
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, input.size());

  ASSERT_EQ(handler.chunks.size(), 1);
  const Chunk * const chunk{&handler.chunks[0]};
  EXPECT_TRUE(chunk->data.empty());

  ASSERT_EQ(handler.chunks[0].extensions.size(), 2);

  const ChunkExtension * const ext1{&chunk->extensions[0]};
  EXPECT_EQ(ext1->name, "ext1");
  EXPECT_EQ(ext1->val, std::nullopt);

  const ChunkExtension * const ext2{&chunk->extensions[1]};
  EXPECT_EQ(ext2->name, "ext2");
  EXPECT_EQ(ext2->val, "val2");
}

TEST_F(ChunkedDecoderTest, TwoNoValueExtensions) {
  const std::vector<std::byte> input{"0;ext1;ext2\r\n\r\n"_b};
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, input.size());

  ASSERT_EQ(handler.chunks.size(), 1);
  const Chunk * const chunk{&handler.chunks[0]};
  EXPECT_TRUE(chunk->data.empty());

  ASSERT_EQ(handler.chunks[0].extensions.size(), 2);

  const ChunkExtension * const ext1{&chunk->extensions[0]};
  EXPECT_EQ(ext1->name, "ext1");
  EXPECT_EQ(ext1->val, std::nullopt);

  const ChunkExtension * const ext2{&chunk->extensions[1]};
  EXPECT_EQ(ext2->name, "ext2");
  EXPECT_EQ(ext2->val, std::nullopt);
}

TEST_F(ChunkedDecoderTest, ChunkDataStartAfterExtEquals) {
  const std::vector<std::byte> input{"0;ext=\r\n\r\n"_b};
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::ERROR);
  EXPECT_EQ(consumed, 6);

  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_EQ(handler.on_error_call_count, 1);
}

TEST_F(ChunkedDecoderTest, LowercaseChunkSize) {
  // The HTTP/1.1 spec requires chunk sizes to use uppercase lettering.
  const std::vector<std::byte> input{"a\r\nWrong case\r\n0\r\n\r\n"_b};
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::ERROR);
  EXPECT_EQ(consumed, 0);

  EXPECT_EQ(handler.chunks.size(), 0);
  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_EQ(handler.on_error_call_count, 1);
}

TEST_F(ChunkedDecoderTest, StrangeBodyCharacters) {
  // The chunked encoding should be able to handle any binary data within the chunk size
  const std::vector<std::byte> input{"F\r\n\r\n\0\xffweird\\\"\a_b;\r\n0\r\n\r\n"_b};
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, input.size());

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
  const std::size_t consumed1{decoder.feed(input1)};
  EXPECT_EQ(decoder.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed1, input1.size());
  EXPECT_TRUE(handler.chunks.empty());
  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 0);

  const std::vector<std::byte> input2{"0\r\nabcdefghijklmnop\r\n0\r\n\r\n"_b};
  const std::size_t consumed2{decoder.feed(input2)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed2, input2.size());
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
  const std::size_t consumed1{decoder.feed(input1)};
  EXPECT_EQ(decoder.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed1, input1.size());
  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_TRUE(handler.chunks.empty());

  const std::vector<std::byte> input2{"1\r\n0\r\n\r\n"_b};
  const std::size_t consumed2{decoder.feed(input2)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed2, input2.size());
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
  const std::size_t consumed1{decoder.feed(input1)};
  EXPECT_EQ(decoder.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed1, input1.size());
  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 0);
  EXPECT_TRUE(handler.chunks.empty());

  const std::vector<std::byte> input2{""_b};
  const std::size_t consumed2{decoder.feed(input2)};
  EXPECT_EQ(decoder.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed2, input2.size());
  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 0);

  const std::vector<std::byte> input3{"1\r\n0\r\n\r\n"_b};
  const std::size_t consumed3{decoder.feed(input3)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed3, input3.size());
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

TEST_F(ChunkedDecoderTest, OneFieldLineTrailerSection) {
  const std::vector<std::byte> input{"0\r\nName: Value\r\n\r\n"_b};
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, input.size());

  ASSERT_EQ(handler.chunks.size(), 1);
  EXPECT_TRUE(handler.chunks.at(0).data.empty());
  EXPECT_TRUE(handler.chunks.at(0).extensions.empty());

  ASSERT_EQ(handler.trailer_section.size(), 1);
  EXPECT_EQ(handler.trailer_section.at(0).get_name(), "Name");
  EXPECT_EQ(handler.trailer_section.at(0).get_value(), "Value");

  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 1);
}

TEST_F(ChunkedDecoderTest, TwoFieldLinesTrailerSection) {
  const std::vector<std::byte> input{"0\r\nName1: Value1\r\nName2: \"Value2\"\r\n\r\n"_b};
  const std::size_t consumed{decoder.feed(input)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, input.size());

  ASSERT_EQ(handler.chunks.size(), 1);
  EXPECT_TRUE(handler.chunks.at(0).data.empty());
  EXPECT_TRUE(handler.chunks.at(0).extensions.empty());

  ASSERT_EQ(handler.trailer_section.size(), 2);
  EXPECT_EQ(handler.trailer_section.at(0).get_name(), "Name1");
  EXPECT_EQ(handler.trailer_section.at(0).get_value(), "Value1");
  EXPECT_EQ(handler.trailer_section.at(1).get_name(), "Name2");
  EXPECT_EQ(handler.trailer_section.at(1).get_value(), "\"Value2\"");

  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 1);
}

TEST_F(ChunkedDecoderTest, SplitTrailerSection) {
  const std::vector<std::byte> input1{"0\r\nName: "_b};
  const std::size_t consumed1{decoder.feed(input1)};
  EXPECT_EQ(decoder.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed1, input1.size());

  const std::vector<std::byte> input2{"Value\r\n\r\n"_b};
  const std::size_t consumed2{decoder.feed(input2)};
  EXPECT_EQ(decoder.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed2, input2.size());

  ASSERT_EQ(handler.chunks.size(), 1);
  EXPECT_TRUE(handler.chunks.at(0).data.empty());
  EXPECT_TRUE(handler.chunks.at(0).extensions.empty());

  ASSERT_EQ(handler.trailer_section.size(), 1);
  EXPECT_EQ(handler.trailer_section.at(0).get_name(), "Name");
  EXPECT_EQ(handler.trailer_section.at(0).get_value(), "Value");

  EXPECT_EQ(handler.on_error_call_count, 0);
  EXPECT_EQ(handler.on_finish_call_count, 1);
}
