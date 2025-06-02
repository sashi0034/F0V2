#pragma once

namespace ZG
{
    class ITimestamp
    {
    public:
        virtual ~ITimestamp() = default;

        virtual uint64_t timestamp() const = 0;
    };

    constexpr uint64_t InvalidTimestampValue = static_cast<int64_t>(-1);

    class InvalidTimestamp_impl : public ITimestamp
    {
        uint64_t timestamp() const override { return InvalidTimestampValue; }
    };

    inline const auto InvalidTimestamp = std::make_shared<InvalidTimestamp_impl>();
}
