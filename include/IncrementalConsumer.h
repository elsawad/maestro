#ifndef INCREMENTAL_CONSUMER_H
#define INCREMENTAL_CONSUMER_H

#include <concepts>
#include <span>

using ByteView = std::span<const std::byte>;

enum class FeedState {
  NEED_MORE_INPUT,
  COMPLETE,
  ERROR
};

template<typename T>
concept IncrementalConsumer = requires(T t, ByteView bv) {
  { t.feed(bv) } -> std::unsigned_integral;
  { t.state() } -> std::same_as<FeedState>;
};

#endif
