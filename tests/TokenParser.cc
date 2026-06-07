#include <gtest/gtest.h>

#include "TokenParser.h"

class TokenParserTest : public ::testing::Test {
  protected:
    TokenParser parser;
};

TEST_F(TokenParserTest, EmptyInput) {
  const std::string input{""};
  const FeedResult result{parser.feed(input)};
  EXPECT_EQ(result.state, FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(result.consumed, input.size());

  const std::optional<std::string_view> value{parser.value()};
  EXPECT_FALSE(value.has_value());
}

TEST_F(TokenParserTest, ValidToken) {
  const std::string input1{"token123"};
  const FeedResult result1{parser.feed(input1)};
  EXPECT_EQ(result1.state, FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(result1.consumed, input1.size());

  const std::optional<std::string_view> value1{parser.value()};
  ASSERT_FALSE(value1.has_value());

  const std::string input2{" "}; // space is not a tchar and thus ends the token
  const FeedResult result2{parser.feed(input2)};
  EXPECT_EQ(result2.state, FeedState::COMPLETE);
  EXPECT_EQ(result2.consumed, 0); // space also does not contribute to the token parser byte consumption count

  const std::optional<std::string_view> value2{parser.value()};
  ASSERT_TRUE(value2.has_value());
  EXPECT_EQ(value2.value(), "token123");
}

TEST_F(TokenParserTest, ImmediatelyInvalidToken) {
  const std::string input{"\ninvalid token"};
  const FeedResult result{parser.feed(input)};
  EXPECT_EQ(result.state, FeedState::ERROR); // the first character fed to a parser should be valid
  EXPECT_EQ(result.consumed, 0);

  const std::optional<std::string_view> value{parser.value()};
  EXPECT_FALSE(value.has_value());
}

TEST_F(TokenParserTest, ValidTokenFollowedByExtraData) {
  const std::string input{"validToken extraData"};
  const FeedResult result{parser.feed(input)};
  EXPECT_EQ(result.state, FeedState::COMPLETE);
  EXPECT_EQ(result.consumed, 10); // "validToken" is 10 characters long

  const std::optional<std::string_view> value{parser.value()};
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(value.value(), "validToken");
}

TEST_F(TokenParserTest, SplitValidToken) {
  const std::string input1{"split"};
  const FeedResult result1{parser.feed(input1)};
  EXPECT_EQ(result1.state, FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(result1.consumed, input1.size());

  const std::optional<std::string_view> value1{parser.value()};
  ASSERT_FALSE(value1.has_value());

  const std::string input2{"Token ~~ this is not part of the token"};
  const FeedResult result2{parser.feed(input2)};
  EXPECT_EQ(result2.state, FeedState::COMPLETE);
  EXPECT_EQ(result2.consumed, 5); // size of "Token"

  const std::optional<std::string_view> value2{parser.value()};
  ASSERT_TRUE(value2.has_value());
  EXPECT_EQ(value2.value(), "splitToken");
}

TEST_F(TokenParserTest, MoreDataAfterComplete) {
  const std::string input1{"completeToken "};
  const FeedResult result1{parser.feed(input1)};
  EXPECT_EQ(result1.state, FeedState::COMPLETE);
  EXPECT_EQ(result1.consumed, 13);

  const std::optional<std::string_view> value2{parser.value()};
  ASSERT_TRUE(value2.has_value());
  EXPECT_EQ(value2.value(), "completeToken");

  const std::string input3{"extraData"};
  const FeedResult result3{parser.feed(input3)};
  EXPECT_EQ(result3.state, FeedState::COMPLETE); // feeding more data after completion should not change the state
  EXPECT_EQ(result3.consumed, 0); // feeding more data after completion should not change the byte consumption count

  const std::optional<std::string_view> value3{parser.value()};
  ASSERT_TRUE(value3.has_value());
  EXPECT_EQ(value3.value(), "completeToken"); // feeding more data after completion should not change the value
}

TEST_F(TokenParserTest, MoreDataAfterError) {
  const std::string input1{" invalidToken"};
  const FeedResult result1{parser.feed(input1)};
  EXPECT_EQ(result1.state, FeedState::ERROR);
  EXPECT_EQ(result1.consumed, 0);

  const std::optional<std::string_view> value1{parser.value()};
  EXPECT_FALSE(value1.has_value());

  const std::string input2{"extraData"};
  const FeedResult result2{parser.feed(input2)};
  EXPECT_EQ(result2.state, FeedState::ERROR); // feeding more data after error should not change the state
  EXPECT_EQ(result2.consumed, 0); // feeding more data after error should not change the byte consumption count

  const std::optional<std::string_view> value2{parser.value()};
  EXPECT_FALSE(value2.has_value()); // feeding more data after error should not change the value
}
