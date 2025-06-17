#include "pch.h"
#include "Random.h"

namespace
{
    thread_local std::mt19937 s_engine{std::random_device{}()};
}

namespace TY
{
    int Random::Int(int min, int max)
    {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(s_engine);
    }

    float Random::Float(float min, float max)
    {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(s_engine);
    }

    bool Random::Bool()
    {
        std::uniform_int_distribution<int> dist(0, 1);
        return dist(s_engine) == 1;
    }
}
