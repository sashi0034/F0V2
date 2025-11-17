#pragma once

namespace Race
{
    class RaceDrawQualityController
    {
    public:
        RaceDrawQualityController();

        void update();

        struct target_type
        {
            float renderScale{};
            bool fsrEnabled{};
        };

        target_type getQualityTarget() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
