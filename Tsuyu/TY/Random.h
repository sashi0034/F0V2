#pragma once

namespace TY
{
    namespace Random
    {
        int Int(int min, int max);

        float Float(float min, float max);

        bool Bool();

        template <typename Container>
        void Shuffle(Container& array)
        {
            for (size_t i = array.size() - 1; i > 0; --i)
            {
                size_t j = Int(0, static_cast<int>(i));
                std::swap(array[i], array[j]);
            }
        }
    }
}
