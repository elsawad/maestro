#include <gtest/gtest.h>

#include "QuotedStringParser.h"
#include "utils/byte_literals.h"

class QuotedStringParserTest : public ::testing::Test {
  protected:
    QuotedStringParser parser;
};

TEST_F(QuotedStringParserTest, EmptyString) {
  std::vector<std::byte> input{""_b};
  const auto result = this->parser.feed(input);
  EXPECT_EQ(result.state, FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(result.consumed, input.size());
  EXPECT_EQ(this->parser.value(), std::nullopt);
}

TEST_F(QuotedStringParserTest, EmptyQuotedString) {
  std::vector<std::byte> input{"\"\""_b};
  const auto result = this->parser.feed(input);
  EXPECT_EQ(result.state, FeedState::COMPLETE);
  EXPECT_EQ(result.consumed, input.size());
  EXPECT_EQ(this->parser.value(), "");
}

TEST_F(QuotedStringParserTest, SplitEmptyQuotedString) {
  std::vector<std::byte> input1{"\""_b};
  const auto result1 = this->parser.feed(input1);
  EXPECT_EQ(result1.state, FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(result1.consumed, input1.size());
  EXPECT_EQ(this->parser.value(), std::nullopt);

  std::vector<std::byte> input2{"\""_b};
  const auto result2 = this->parser.feed(input2);
  EXPECT_EQ(result2.state, FeedState::COMPLETE);
  EXPECT_EQ(result2.consumed, input2.size());
  EXPECT_EQ(this->parser.value(), "");
}

TEST_F(QuotedStringParserTest, ValidQuotedString) {
  std::vector<std::byte> input{"\"Hello, World!\""_b};
  const auto result = this->parser.feed(input);
  EXPECT_EQ(result.state, FeedState::COMPLETE);
  EXPECT_EQ(result.consumed, input.size());
  EXPECT_EQ(this->parser.value(), "Hello, World!");
}

TEST_F(QuotedStringParserTest, SplitValidQuotedString) {
  std::vector<std::byte> input1{"\"Hello,"_b};
  const auto result1 = this->parser.feed(input1);
  EXPECT_EQ(result1.state, FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(result1.consumed, input1.size());
  EXPECT_EQ(this->parser.value(), std::nullopt);

  std::vector<std::byte> input2{" World!\""_b};
  const auto result2 = this->parser.feed(input2);
  EXPECT_EQ(result2.state, FeedState::COMPLETE);
  EXPECT_EQ(result2.consumed, input2.size());
  EXPECT_EQ(this->parser.value(), "Hello, World!");
}

TEST_F(QuotedStringParserTest, QuotedStringWithEscapedQuote) {
  std::vector<std::byte> input{"\"Hello, \\\"World\\\"!\""_b};
  const auto result = this->parser.feed(input);
  EXPECT_EQ(result.state, FeedState::COMPLETE);
  EXPECT_EQ(result.consumed, input.size());
  EXPECT_EQ(this->parser.value(), "Hello, \"World\"!");
}

TEST_F(QuotedStringParserTest, SplitQuotedStringWithEscapedQuote) {
  std::vector<std::byte> input1{"\"Hello, \\\"World\\\"!"_b};
  const auto result1 = this->parser.feed(input1);
  EXPECT_EQ(result1.state, FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(result1.consumed, input1.size());
  EXPECT_EQ(this->parser.value(), std::nullopt);

  std::vector<std::byte> input2{"\""_b};
  const auto result2 = this->parser.feed(input2);
  EXPECT_EQ(result2.state, FeedState::COMPLETE);
  EXPECT_EQ(result2.consumed, input2.size());
  EXPECT_EQ(this->parser.value(), "Hello, \"World\"!");
}

TEST_F(QuotedStringParserTest, InvalidQuotedString) {
  std::vector<std::byte> input{"Hello, World!"_b};
  const auto result = this->parser.feed(input);
  EXPECT_EQ(result.state, FeedState::ERROR);
  EXPECT_EQ(result.consumed, 0);
  EXPECT_EQ(this->parser.value(), std::nullopt);
}

// When dealing with invalid characters, parsers should consume greedily until
// an invalid character is encountered. Because the parser structure in this
// project is designed to be fed with chunks of input, it does not backtrack to
// the last valid grammatical construct.
TEST_F(QuotedStringParserTest, QuotedStringWithInvalidEscape) {
  std::vector<std::byte> input{"\"Hello, \\\x7fWorld!\""_b};
  const auto result = this->parser.feed(input);
  EXPECT_EQ(result.state, FeedState::ERROR);
  EXPECT_EQ(result.consumed, 9); // first backslash is valid, but a backslash can not be the character in a quoted pair
  EXPECT_EQ(this->parser.value(), std::nullopt);
}

TEST_F(QuotedStringParserTest, QuotedStringWithInvalidCharacter) {
  std::vector<std::byte> input{"\"Hello, \x01World!\""_b};
  const auto result = this->parser.feed(input);
  EXPECT_EQ(result.state, FeedState::ERROR);
  EXPECT_EQ(result.consumed, 8); // up to the invalid byte
  EXPECT_EQ(this->parser.value(), std::nullopt);
}

TEST_F(QuotedStringParserTest, FeedAfterSuccess) {
  std::vector<std::byte> input1{"\"Hello, World!\""_b};
  const auto result1 = this->parser.feed(input1);
  EXPECT_EQ(result1.state, FeedState::COMPLETE);
  EXPECT_EQ(result1.consumed, 15);
  EXPECT_EQ(this->parser.value(), "Hello, World!");

  std::vector<std::byte> input2{"\"Another string\""_b};
  const auto result2 = this->parser.feed(input2);
  EXPECT_EQ(result2.state, FeedState::COMPLETE);
  EXPECT_EQ(result2.consumed, 0);
  EXPECT_EQ(this->parser.value(), "Hello, World!");
}

TEST_F(QuotedStringParserTest, FeedAfterError) {
  std::vector<std::byte> input1{"Invalid input"_b};
  const auto result1 = this->parser.feed(input1);
  EXPECT_EQ(result1.state, FeedState::ERROR);
  EXPECT_EQ(result1.consumed, 0);
  EXPECT_EQ(this->parser.value(), std::nullopt);

  std::vector<std::byte> input2{"\"Another string\""_b};
  const auto result2 = this->parser.feed(input2);
  EXPECT_EQ(result2.state, FeedState::ERROR);
  EXPECT_EQ(result2.consumed, 0);
  EXPECT_EQ(this->parser.value(), std::nullopt);
}

TEST_F(QuotedStringParserTest, ExtraDataAfterSuccess) {
  std::vector<std::byte> input{"\"Hello, World!\" Extra data"_b};
  const auto result = this->parser.feed(input);
  EXPECT_EQ(result.state, FeedState::COMPLETE);
  EXPECT_EQ(result.consumed, 15); // only the quoted string is consumed
  EXPECT_EQ(this->parser.value(), "Hello, World!");
}

TEST_F(QuotedStringParserTest, ExtraDataAfterError) {
  std::vector<std::byte> input{"\"Hello, \x01World!\" Extra data"_b};
  const auto result1 = this->parser.feed(input);
  EXPECT_EQ(result1.state, FeedState::ERROR);
  EXPECT_EQ(result1.consumed, 8); // up to the invalid byte
  EXPECT_EQ(this->parser.value(), std::nullopt);
}
