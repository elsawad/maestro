#include <gtest/gtest.h>

#include "FieldCollectionParser.h"
#include "utils/byte_literals.h"

class FieldCollectionParserTest: public ::testing::Test {
  protected:
    FieldCollectionParser parser;
};

TEST_F(FieldCollectionParserTest, NoData) {
  const std::vector<std::byte> input{""_b};
  const std::size_t consumed{parser.feed(input)};
  EXPECT_EQ(parser.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed, input.size());
}

TEST_F(FieldCollectionParserTest, SimpleFieldUnterminated) {
  const std::vector<std::byte> input{"Field: Value\r\n"_b};
  const std::size_t consumed{parser.feed(input)};
  EXPECT_EQ(parser.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed, input.size());
  EXPECT_EQ(parser.value().size(), 1);
}

// FieldCollectionParser stops parsing when a non-tchar is encountered where a tchar is expected. This allows for empty FieldCollection instances.
TEST_F(FieldCollectionParserTest, SimpleFieldTerminatedWithCR) {
  const std::vector<std::byte> input{"Field: Value\r\n\r"_b};
  const std::size_t consumed{parser.feed(input)};
  EXPECT_EQ(parser.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, 14);
  EXPECT_EQ(parser.value().size(), 1);
}

TEST_F(FieldCollectionParserTest, SimpleFieldTerminatedWithNull) {
  const std::vector<std::byte> input{"Field: Value\r\n\0"_b};
  const std::size_t consumed{parser.feed(input)};
  EXPECT_EQ(parser.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, 14);
  EXPECT_EQ(parser.value().size(), 1);
}

TEST_F(FieldCollectionParserTest, SimpleFieldWithValueWhitespace) {
  const std::vector<std::byte> input{"Field: \t\t\t\t    Value\t  \t\t   \r\n\r"_b};
  const std::size_t consumed{parser.feed(input)};
  EXPECT_EQ(parser.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed, input.size() - 1);
  EXPECT_EQ(parser.value().size(), 1);
}

TEST_F(FieldCollectionParserTest, SplitFieldName) {
  const std::vector<std::byte> input1{"Fie"_b};
  const std::size_t consumed1{parser.feed(input1)};
  EXPECT_EQ(parser.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed1, input1.size());

  const std::vector<std::byte> input2{"ld: Value\r\n\r"_b};
  const std::size_t consumed2{parser.feed(input2)};
  EXPECT_EQ(parser.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed2, input2.size() - 1);

  ASSERT_EQ(parser.value().size(), 1);
  EXPECT_EQ(parser.value().at(0).get_name(), "Field");
  EXPECT_EQ(parser.value().at(0).get_value(), "Value");
}

TEST_F(FieldCollectionParserTest, IncorrectNameValueDelimiter) {
  const std::vector<std::byte> input{"Field; Value\r\n\r"_b};
  const std::size_t consumed{parser.feed(input)};
  EXPECT_EQ(parser.state(), FeedState::ERROR);
  EXPECT_EQ(consumed, 5);
}

TEST_F(FieldCollectionParserTest, SplitOWSBeforeAndAfterFieldValue) {
  const std::vector<std::byte> input1{"Field:  \t\t  \t   "_b};
  const std::size_t consumed1{parser.feed(input1)};
  EXPECT_EQ(parser.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed1, input1.size());

  const std::vector<std::byte> input2{" \t\t\tValue\t            "_b};
  const std::size_t consumed2{parser.feed(input2)};
  EXPECT_EQ(parser.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed2, input2.size());

  const std::vector<std::byte> input3{"\t  \t  \t  \t  \r\n\r"_b};
  const std::size_t consumed3{parser.feed(input3)};
  EXPECT_EQ(parser.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed3, input3.size() - 1);

  ASSERT_EQ(parser.value().size(), 1);
  EXPECT_EQ(parser.value().at(0).get_name(), "Field");
  EXPECT_EQ(parser.value().at(0).get_value(), "Value");
}

TEST_F(FieldCollectionParserTest, SplitFieldValue) {
  const std::vector<std::byte> input1{"Field: Val"_b};
  const std::size_t consumed1{parser.feed(input1)};
  EXPECT_EQ(parser.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed1, input1.size());

  const std::vector<std::byte> input2{"ue\r\n\r"_b};
  const std::size_t consumed2{parser.feed(input2)};
  EXPECT_EQ(parser.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed2, input2.size() - 1);

  ASSERT_EQ(parser.value().size(), 1);
  EXPECT_EQ(parser.value().at(0).get_name(), "Field");
  EXPECT_EQ(parser.value().at(0).get_value(), "Value");
}

TEST_F(FieldCollectionParserTest, NoLFAfterCR) {
  const std::vector<std::byte> input{"Field; Value\r\r"_b};
  const std::size_t consumed{parser.feed(input)};
  EXPECT_EQ(parser.state(), FeedState::ERROR);
  const std::vector<std::byte> expected_remaining{"\r"_b};
  EXPECT_TRUE(std::equal(input.cbegin() + 13, input.cend(), expected_remaining.cbegin(), expected_remaining.cend()));
}

TEST_F(FieldCollectionParserTest, SplitFieldCRLF) {
  const std::vector<std::byte> input1{"Field: Value\r"_b};
  const std::size_t consumed1{parser.feed(input1)};
  EXPECT_EQ(parser.state(), FeedState::NEED_MORE_INPUT);
  EXPECT_EQ(consumed1, input1.size());

  const std::vector<std::byte> input2{"\n\r"_b};
  const std::size_t consumed2{parser.feed(input2)};
  EXPECT_EQ(parser.state(), FeedState::COMPLETE);
  EXPECT_EQ(consumed2, input2.size() - 1);

  ASSERT_EQ(parser.value().size(), 1);
  EXPECT_EQ(parser.value().at(0).get_name(), "Field");
  EXPECT_EQ(parser.value().at(0).get_value(), "Value");
}
