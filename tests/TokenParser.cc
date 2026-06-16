#include <gtest/gtest.h>

#include "TokenParser.h"
#include "utils/byte_literals.h"

class TokenParserTest : public ::testing::Test {
  protected:
    TokenParser parser;
};

TEST_F(TokenParserTest, EmptyInput) {
  std::vector<std::byte> input{""_b};
  const std::size_t consumed{parser.feed(input)};
  EXPECT_EQ(parser.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed, input.size());

  const std::optional<std::string_view> value{parser.value()};
  EXPECT_FALSE(value.has_value());
}

TEST_F(TokenParserTest, ValidToken) {
  std::vector<std::byte> input1{"token123"_b};
  const std::size_t consumed1{parser.feed(input1)};
  EXPECT_EQ(parser.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed1, input1.size());

  const std::optional<std::string_view> value1{parser.value()};
  ASSERT_FALSE(value1.has_value());

  std::vector<std::byte> input2{" "_b}; // space is not a tchar and thus ends the token
  const std::size_t consumed2{parser.feed(input2)};
  EXPECT_EQ(parser.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed2, 0); // space also does not contribute to the token parser byte consumption count

  const std::optional<std::string_view> value2{parser.value()};
  ASSERT_TRUE(value2.has_value());
  EXPECT_EQ(value2.value(), "token123");
}

TEST_F(TokenParserTest, ImmediatelyInvalidToken) {
  std::vector<std::byte> input{"\ninvalid token"_b};
  const std::size_t consumed{parser.feed(input)};
  EXPECT_EQ(parser.state(), FeedState::ERROR); // the first character fed to a parser should be valid
  EXPECT_EQ(consumed, 0);

  const std::optional<std::string_view> value{parser.value()};
  EXPECT_FALSE(value.has_value());
}

TEST_F(TokenParserTest, ValidTokenFollowedByExtraData) {
  std::vector<std::byte> input{"validToken extraData"_b};
  const std::size_t consumed{parser.feed(input)};
  EXPECT_EQ(parser.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, 10); // "validToken" is 10 characters long

  const std::optional<std::string_view> value{parser.value()};
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(value.value(), "validToken");
}

TEST_F(TokenParserTest, SplitValidToken) {
  std::vector<std::byte> input1{"split"_b};
  const std::size_t consumed1{parser.feed(input1)};
  EXPECT_EQ(parser.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed1, input1.size());

  const std::optional<std::string_view> value1{parser.value()};
  ASSERT_FALSE(value1.has_value());

  std::vector<std::byte> input2{"Token ~~ this is not part of the token"_b};
  const std::size_t consumed2{parser.feed(input2)};
  EXPECT_EQ(parser.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed2, 5); // size of "Token"

  const std::optional<std::string_view> value2{parser.value()};
  ASSERT_TRUE(value2.has_value());
  EXPECT_EQ(value2.value(), "splitToken");
}

TEST_F(TokenParserTest, MoreDataAfterComplete) {
  std::vector<std::byte> input1{"completeToken "_b};
  const std::size_t consumed1{parser.feed(input1)};
  EXPECT_EQ(parser.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed1, 13);

  const std::optional<std::string_view> value2{parser.value()};
  ASSERT_TRUE(value2.has_value());
  EXPECT_EQ(value2.value(), "completeToken");

  std::vector<std::byte> input3{"extraData"_b};
  const std::size_t consumed3{parser.feed(input3)};
  EXPECT_EQ(parser.state(), FeedState::COMPLETE); // feeding more data after completion should not change the state
  EXPECT_EQ(consumed3, 0); // feeding more data after completion should not change the byte consumption count

  const std::optional<std::string_view> value3{parser.value()};
  ASSERT_TRUE(value3.has_value());
  EXPECT_EQ(value3.value(), "completeToken"); // feeding more data after completion should not change the value
}

TEST_F(TokenParserTest, MoreDataAfterError) {
  std::vector<std::byte> input1{" invalidToken"_b};
  const std::size_t consumed1{parser.feed(input1)};
  EXPECT_EQ(parser.state(), FeedState::ERROR);
  EXPECT_EQ(consumed1, 0);

  const std::optional<std::string_view> value1{parser.value()};
  EXPECT_FALSE(value1.has_value());

  std::vector<std::byte> input2{"extraData"_b};
  const std::size_t consumed2{parser.feed(input2)};
  EXPECT_EQ(parser.state(), FeedState::ERROR); // feeding more data after error should not change the state
  EXPECT_EQ(consumed2, 0); // feeding more data after error should not change the byte consumption count

  const std::optional<std::string_view> value2{parser.value()};
  EXPECT_FALSE(value2.has_value()); // feeding more data after error should not change the value
}
