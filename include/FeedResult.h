#ifndef FEED_RESULT_H
#define FEED_RESULT_H

#include <cstddef>

enum class FeedState {
  NEED_MORE_INPUT,
  COMPLETE,
  ERROR
};

struct FeedResult {
  FeedState state;
  std::size_t consumed;
};

#endif
