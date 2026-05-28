#ifndef STREAM_PROCESSOR_H
#define STREAM_PROCESSOR_H

#include <cstdint>
#include <span>

class StreamProcessor {
  public:
    enum class Status {
      IN_PROGRESS,
      DONE,
      INVALID
    };

    struct FeedResult {
      Status status;
      std::span<const std::byte> remaining;
    };

    virtual ~StreamProcessor() = default;
    virtual FeedResult feed(std::span<const std::byte> span) = 0;
    Status get_status() const;

  protected:
    Status status = Status::IN_PROGRESS;
};

#endif
