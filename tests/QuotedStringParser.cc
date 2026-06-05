#include <gtest/gtest.h>

#include "QuotedStringParser.h"

class QuotedStringParserTest : public ::testing::Test {
  protected:
    QuotedStringParser parser;
};

TEST_F(QuotedStringParserTest, EmptyString) {
  std::string input{""};
  const auto result = this->parser.feed(input);
  EXPECT_EQ(result.state, FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(result.consumed, input.size());
  EXPECT_EQ(this->parser.get_string(), std::nullopt);
}

TEST_F(QuotedStringParserTest, EmptyQuotedString) {
  std::string input{"\"\""};
  const auto result = this->parser.feed(input);
  EXPECT_EQ(result.state, FeedState::COMPLETE);
  EXPECT_EQ(result.consumed, input.size());
  EXPECT_EQ(this->parser.get_string(), "");
}

TEST_F(QuotedStringParserTest, SplitEmptyQuotedString) {
  std::string input1{"\""};
  const auto result1 = this->parser.feed(input1);
  EXPECT_EQ(result1.state, FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(result1.consumed, input1.size());
  EXPECT_EQ(this->parser.get_string(), std::nullopt);

  std::string input2{"\""};
  const auto result2 = this->parser.feed(input2);
  EXPECT_EQ(result2.state, FeedState::COMPLETE);
  EXPECT_EQ(result2.consumed, input2.size());
  EXPECT_EQ(this->parser.get_string(), "");
}

TEST_F(QuotedStringParserTest, ValidQuotedString) {
  std::string input{"\"Hello, World!\""};
  const auto result = this->parser.feed(input);
  EXPECT_EQ(result.state, FeedState::COMPLETE);
  EXPECT_EQ(result.consumed, input.size());
  EXPECT_EQ(this->parser.get_string(), "Hello, World!");
}

TEST_F(QuotedStringParserTest, SplitValidQuotedString) {
  std::string input1{"\"Hello,"};
  const auto result1 = this->parser.feed(input1);
  EXPECT_EQ(result1.state, FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(result1.consumed, input1.size());
  EXPECT_EQ(this->parser.get_string(), std::nullopt);

  std::string input2{" World!\""};
  const auto result2 = this->parser.feed(input2);
  EXPECT_EQ(result2.state, FeedState::COMPLETE);
  EXPECT_EQ(result2.consumed, input2.size());
  EXPECT_EQ(this->parser.get_string(), "Hello, World!");
}

TEST_F(QuotedStringParserTest, QuotedStringWithEscapedQuote) {
  std::string input{"\"Hello, \\\"World\\\"!\""};
  const auto result = this->parser.feed(input);
  EXPECT_EQ(result.state, FeedState::COMPLETE);
  EXPECT_EQ(result.consumed, input.size());
  EXPECT_EQ(this->parser.get_string(), "Hello, \"World\"!");
}

TEST_F(QuotedStringParserTest, SplitQuotedStringWithEscapedQuote) {
  std::string input1{"\"Hello, \\\"World\\\"!"};
  const auto result1 = this->parser.feed(input1);
  EXPECT_EQ(result1.state, FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(result1.consumed, input1.size());
  EXPECT_EQ(this->parser.get_string(), std::nullopt);

  std::string input2{"\""};
  const auto result2 = this->parser.feed(input2);
  EXPECT_EQ(result2.state, FeedState::COMPLETE);
  EXPECT_EQ(result2.consumed, input2.size());
  EXPECT_EQ(this->parser.get_string(), "Hello, \"World\"!");
}

TEST_F(QuotedStringParserTest, InvalidQuotedString) {
  const auto result = this->parser.feed("Hello, World!");
  EXPECT_EQ(result.state, FeedState::ERROR);
  EXPECT_EQ(result.consumed, 0);
  EXPECT_EQ(this->parser.get_string(), std::nullopt);
}

// When dealing with invalid characters, parsers should consume greedily until
// an invalid character is encountered. Because the parser structure in this
// project is designed to be fed with chunks of input, it does not backtrack to
// the last valid grammatical construct.
TEST_F(QuotedStringParserTest, QuotedStringWithInvalidEscape) {
  const auto result = this->parser.feed("\"Hello, \\\x7fWorld!\"");
  EXPECT_EQ(result.state, FeedState::ERROR);
  EXPECT_EQ(result.consumed, 9); // first backslash is valid, but a backslash can not be the character in a quoted pair
  EXPECT_EQ(this->parser.get_string(), std::nullopt);
}

TEST_F(QuotedStringParserTest, QuotedStringWithInvalidCharacter) {
  const auto result = this->parser.feed("\"Hello, \x01World!\"");
  EXPECT_EQ(result.state, FeedState::ERROR);
  EXPECT_EQ(result.consumed, 8); // up to the invalid byte
  EXPECT_EQ(this->parser.get_string(), std::nullopt);
}

TEST_F(QuotedStringParserTest, FeedAfterSuccess) {
  const auto result1 = this->parser.feed("\"Hello, World!\"");
  EXPECT_EQ(result1.state, FeedState::COMPLETE);
  EXPECT_EQ(result1.consumed, 15);
  EXPECT_EQ(this->parser.get_string(), "Hello, World!");

  const auto result2 = this->parser.feed("\"Another string\"");
  EXPECT_EQ(result2.state, FeedState::COMPLETE);
  EXPECT_EQ(result2.consumed, 0);
  EXPECT_EQ(this->parser.get_string(), "Hello, World!");
}

TEST_F(QuotedStringParserTest, FeedAfterError) {
  const auto result1 = this->parser.feed("Invalid input");
  EXPECT_EQ(result1.state, FeedState::ERROR);
  EXPECT_EQ(result1.consumed, 0);
  EXPECT_EQ(this->parser.get_string(), std::nullopt);

  const auto result2 = this->parser.feed("\"Another string\"");
  EXPECT_EQ(result2.state, FeedState::ERROR);
  EXPECT_EQ(result2.consumed, 0);
  EXPECT_EQ(this->parser.get_string(), std::nullopt);
}

TEST_F(QuotedStringParserTest, ExtraDataAfterSuccess) {
  const auto result1 = this->parser.feed("\"Hello, World!\" Extra data");
  EXPECT_EQ(result1.state, FeedState::COMPLETE);
  EXPECT_EQ(result1.consumed, 15); // only the quoted string is consumed
  EXPECT_EQ(this->parser.get_string(), "Hello, World!");
}

TEST_F(QuotedStringParserTest, ExtraDataAfterError) {
  const auto result1 = this->parser.feed("\"Hello, \x01World!\" Extra data");
  EXPECT_EQ(result1.state, FeedState::ERROR);
  EXPECT_EQ(result1.consumed, 8); // up to the invalid byte
  EXPECT_EQ(this->parser.get_string(), std::nullopt);
}
