#pragma once
#include <string>
#include <stdint.h>

namespace util
{
/// Wrapper of timestamp.
class Timestamp
{
public:
    Timestamp() : _timestamp(0) {}
    Timestamp(uint64_t timestamp) : _timestamp(timestamp) {}

    /// Timestamp of current timestamp.
    static Timestamp now();

    /// Formatted string of today. eg, 20180805
    std::string date() const;

    /// Formatted string of current second include date and time.
    /// eg, 20180805 14:45:20
    std::string datetime() const;
    std::string datetime2() const;

    /// Formatted string of current second include time only.
    std::string time() const;

    /// Formatted string of current timestamp. eg, 20190701 16:28:30.070981
    std::string formatTimestamp() const;

    /// Current timestamp.
    uint64_t timestamp() const { return _timestamp; }

private:
    uint64_t _timestamp;
    static const uint32_t kMicrosecondOneSecond = 1000000;
};

} // namespace util
