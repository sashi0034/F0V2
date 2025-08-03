#pragma once

namespace TY
{
    template <typename Type>
    class Array : public std::vector<Type>
    {
    public:
        using type = Type;

        using std::vector<Type>::vector;

        void remove_at(size_t index)
        {
            if (index < this->size())
            {
                this->erase(this->begin() + index);
            }
        }

        Array append(const Array& other)
        {
            this->insert(this->end(), other.begin(), other.end());
            return *this;
        }

        template <typename U = Type>
        Array<Array<U>> toColumnVector() const
        {
            Array<Array<U>> result{};
            result.reserve(this->size());
            for (const auto& item : *this)
            {
                result.push_back(Array<U>{item});
            }

            return result;
        }

        template <class Fty, std::enable_if_t<std::is_invocable_r_v<bool, Fty, Type>>* = nullptr>
        [[nodiscard]]
        Array filter(Fty f) const
        {
            Array<Type> result{};
            for (const auto& item : *this)
            {
                if (f(item))
                {
                    result.push_back(item);
                }
            }

            return result;
        }

        template <class Fty, std::enable_if_t<std::is_invocable_v<Fty, Type>>* = nullptr>
        auto map(Fty f) const
        {
            Array<std::invoke_result_t<Fty, Type>> result{};
            result.reserve(this->size());
            for (const auto& item : *this)
            {
                result.push_back(f(item));
            }

            return result;
        }

        int indexOf(const Type& value) const
        {
            for (size_t i = 0; i < this->size(); ++i)
            {
                if ((*this)[i] == value)
                {
                    return static_cast<int>(i);
                }
            }

            return -1;
        }

        template <class Fty, std::enable_if_t<std::is_invocable_r_v<bool, Fty, Type>>* = nullptr>
        int indexOf(Fty f) const
        {
            for (size_t i = 0; i < this->size(); ++i)
            {
                if (f((*this)[i]))
                {
                    return static_cast<int>(i);
                }
            }

            return -1;
        }

        bool contains(const Type& value) const
        {
            return std::find(this->begin(), this->end(), value) != this->end();
        }

        template <class Fty, std::enable_if_t<std::is_invocable_r_v<bool, Fty, Type>>* = nullptr>
        bool contains(Fty f) const
        {
            return std::any_of(this->begin(), this->end(), f);
        }

        bool sequenceEquals(const Array& other) const
        {
            if (this->size() != other.size())
            {
                return false;
            }

            for (size_t i = 0; i < this->size(); ++i)
            {
                if ((*this)[i] != other[i])
                {
                    return false;
                }
            }

            return true;
        }

        bool sequenceAlmostEquals(const Array& other, float threshold = 1e-5f) const
        {
            if (this->size() != other.size())
            {
                return false;
            }

            for (size_t i = 0; i < this->size(); ++i)
            {
                if (std::abs((*this)[i] - other[i]) > threshold)
                {
                    return false;
                }
            }

            return true;
        }

        /// @brief バッファのサイズをバイト単位で取得
        [[nodiscard]]
        std::size_t size_in_bytes() const
        {
            return this->size() * sizeof(Type);
        }
    };
}
