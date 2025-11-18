#pragma once

namespace Race
{
    class RaceDrawQualityController
    {
    public:
        RaceDrawQualityController();

        void update();

        struct data_type
        {
            float renderScale{};
            bool fsrEnabled{};
        };

        data_type getQualityData() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
