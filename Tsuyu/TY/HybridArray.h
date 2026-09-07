#pragma once

#include <algorithm>
#include <cassert>
#include <compare>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace TY
{
    template <class T, std::size_t InlineCapacity, class Allocator = std::allocator<T>>
    class HybridArray
    {
        static_assert(InlineCapacity > 0, "HybridArray requires InlineCapacity > 0");

    public:
        using value_type = T;
        using allocator_type = Allocator;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = typename std::allocator_traits<allocator_type>::pointer;
        using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;
        using iterator = value_type*;
        using const_iterator = const value_type*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    private:
        using AllocTraits = std::allocator_traits<allocator_type>;

        static_assert(std::is_same_v<pointer, value_type*>,
                      "HybridArray currently requires an allocator whose pointer type is T*");
        static_assert(std::is_same_v<const_pointer, const value_type*>,
                      "HybridArray currently requires an allocator whose const_pointer type is const T*");

        union Storage
        {
            alignas(value_type) std::byte inlineData[sizeof(value_type) * InlineCapacity];
            value_type* heapData;

            Storage() noexcept
            {
            }

            ~Storage()
            {
            }
        };

    public:
        HybridArray() noexcept(std::is_nothrow_default_constructible_v<allocator_type>) = default;

        explicit HybridArray(const allocator_type& alloc) noexcept(std::is_nothrow_copy_constructible_v<allocator_type>)
            : m_allocator(alloc)
        {
        }

        explicit HybridArray(size_type count, const allocator_type& alloc = allocator_type())
            : m_allocator(alloc)
        {
            resize(count);
        }

        HybridArray(size_type count, const value_type& value,
                    const allocator_type& alloc = allocator_type())
            : m_allocator(alloc)
        {
            assign(count, value);
        }

        template <class InputIt,
                  std::enable_if_t<!std::is_integral_v<InputIt>, int> = 0>
        HybridArray(InputIt first, InputIt last,
                    const allocator_type& alloc = allocator_type())
            : m_allocator(alloc)
        {
            appendRange(first, last);
        }

        HybridArray(const HybridArray& other)
            : m_allocator(AllocTraits::select_on_container_copy_construction(other.m_allocator))
        {
            appendRange(other.begin(), other.end());
        }

        HybridArray(const HybridArray& other, const allocator_type& alloc)
            : m_allocator(alloc)
        {
            appendRange(other.begin(), other.end());
        }

        HybridArray(HybridArray&& other) noexcept(
            std::is_nothrow_move_constructible_v<allocator_type> &&
            std::is_nothrow_move_constructible_v<value_type>)
            : m_allocator(std::move(other.m_allocator))
        {
            moveConstructFrom(std::move(other));
        }

        HybridArray(HybridArray&& other, const allocator_type& alloc)
            : m_allocator(alloc)
        {
            if (m_allocator == other.m_allocator && other.isHeap())
            {
                stealHeapFrom(other);
            }
            else
            {
                reserve(other.size());
                for (auto& value : other)
                    emplace_back(std::move(value));
                other.clear();
            }
        }

        HybridArray(std::initializer_list<value_type> init,
                    const allocator_type& alloc = allocator_type())
            : m_allocator(alloc)
        {
            appendRange(init.begin(), init.end());
        }

        ~HybridArray()
        {
            destroyElements();
            releaseHeap();
        }

        HybridArray& operator=(const HybridArray& other)
        {
            if (this == &other)
                return *this;

            if constexpr (AllocTraits::propagate_on_container_copy_assignment::value)
            {
                if (m_allocator != other.m_allocator)
                {
                    destroyElements();
                    releaseHeap();
                    resetToInline();
                }
                m_allocator = other.m_allocator;
            }

            assign(other.begin(), other.end());
            return *this;
        }

        HybridArray& operator=(HybridArray&& other) noexcept(
            (AllocTraits::propagate_on_container_move_assignment::value ||
                AllocTraits::is_always_equal::value) &&
            std::is_nothrow_move_assignable_v<allocator_type> &&
            std::is_nothrow_move_constructible_v<value_type>)
        {
            if (this == &other)
                return *this;

            if constexpr (AllocTraits::propagate_on_container_move_assignment::value)
            {
                destroyElements();
                releaseHeap();
                resetToInline();
                m_allocator = std::move(other.m_allocator);
                moveConstructFrom(std::move(other));
            }
            else if (m_allocator == other.m_allocator)
            {
                destroyElements();
                releaseHeap();
                resetToInline();
                moveConstructFrom(std::move(other));
            }
            else
            {
                clear();
                reserve(other.size());
                for (auto& value : other)
                    emplace_back(std::move(value));
                other.clear();
            }

            return *this;
        }

        HybridArray& operator=(std::initializer_list<value_type> init)
        {
            assign(init);
            return *this;
        }

        void assign(size_type count, const value_type& value)
        {
            value_type valueCopy = value; // handles aliasing with an existing element
            clear();
            if (count > capacity())
                reserve(count);

            size_type constructed = 0;
            try
            {
                for (; constructed < count; ++constructed)
                    AllocTraits::construct(m_allocator, data() + constructed, valueCopy);
            }
            catch (...)
            {
                destroyRange(data(), data() + constructed);
                throw;
            }
            m_size = count;
        }

        template <class InputIt,
                  std::enable_if_t<!std::is_integral_v<InputIt>, int> = 0>
        void assign(InputIt first, InputIt last)
        {
            HybridArray temp(first, last, m_allocator);
            replaceWith(std::move(temp));
        }

        void assign(std::initializer_list<value_type> init)
        {
            assign(init.begin(), init.end());
        }

        [[nodiscard]] allocator_type get_allocator() const noexcept
        {
            return m_allocator;
        }

        [[nodiscard]] reference at(size_type pos)
        {
            if (pos >= m_size)
                throw std::out_of_range("TY::HybridArray::at");
            return data()[pos];
        }

        [[nodiscard]] const_reference at(size_type pos) const
        {
            if (pos >= m_size)
                throw std::out_of_range("TY::HybridArray::at");
            return data()[pos];
        }

        [[nodiscard]] reference operator[](size_type pos) noexcept { return data()[pos]; }
        [[nodiscard]] const_reference operator[](size_type pos) const noexcept { return data()[pos]; }

        [[nodiscard]] reference front() noexcept { return *begin(); }
        [[nodiscard]] const_reference front() const noexcept { return *begin(); }
        [[nodiscard]] reference back() noexcept { return *(end() - 1); }
        [[nodiscard]] const_reference back() const noexcept { return *(end() - 1); }

        [[nodiscard]] value_type* data() noexcept
        {
            return isHeap() ? m_storage.heapData : inlineData();
        }

        [[nodiscard]] const value_type* data() const noexcept
        {
            return isHeap() ? m_storage.heapData : inlineData();
        }

        [[nodiscard]] iterator begin() noexcept { return data(); }
        [[nodiscard]] const_iterator begin() const noexcept { return data(); }
        [[nodiscard]] const_iterator cbegin() const noexcept { return data(); }

        [[nodiscard]] iterator end() noexcept { return data() + m_size; }
        [[nodiscard]] const_iterator end() const noexcept { return data() + m_size; }
        [[nodiscard]] const_iterator cend() const noexcept { return data() + m_size; }

        [[nodiscard]] reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        [[nodiscard]] const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        [[nodiscard]] const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

        [[nodiscard]] reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        [[nodiscard]] const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        [[nodiscard]] const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

        [[nodiscard]] bool empty() const noexcept { return m_size == 0; }
        [[nodiscard]] size_type size() const noexcept { return m_size; }
        [[nodiscard]] size_type capacity() const noexcept { return m_capacity; }

        [[nodiscard]] size_type max_size() const noexcept
        {
            return std::min<size_type>(
                AllocTraits::max_size(m_allocator),
                static_cast<size_type>(std::numeric_limits<difference_type>::max()));
        }

        void reserve(size_type newCapacity)
        {
            if (newCapacity <= m_capacity)
                return;
            if (newCapacity > max_size())
                throw std::length_error("TY::HybridArray::reserve");

            reallocate(newCapacity);
        }

        void shrink_to_fit()
        {
            if (!isHeap())
                return;

            if (m_size <= InlineCapacity)
            {
                moveHeapToInline();
            }
            else if (m_size < m_capacity)
            {
                reallocate(m_size);
            }
        }

        void clear() noexcept
        {
            destroyElements();
            m_size = 0;
        }

        iterator insert(const_iterator pos, const value_type& value)
        {
            value_type valueCopy = value; // handles self-aliasing
            return emplace(pos, std::move(valueCopy));
        }

        iterator insert(const_iterator pos, value_type&& value)
        {
            const size_type index = indexOf(pos);
            // If value aliases this container, moving after reallocation would be invalid.
            if (containsAddress(std::addressof(value)))
            {
                value_type temp(std::move(value));
                return emplace(begin() + index, std::move(temp));
            }
            return emplace(begin() + index, std::move(value));
        }

        iterator insert(const_iterator pos, size_type count, const value_type& value)
        {
            const size_type index = indexOf(pos);
            if (count == 0)
                return begin() + index;

            value_type valueCopy = value;
            HybridArray temp(count, valueCopy, m_allocator);
            return insertRangeFromTemp(index, temp);
        }

        template <class InputIt,
                  std::enable_if_t<!std::is_integral_v<InputIt>, int> = 0>
        iterator insert(const_iterator pos, InputIt first, InputIt last)
        {
            const size_type index = indexOf(pos);
            HybridArray temp(first, last, m_allocator);
            return insertRangeFromTemp(index, temp);
        }

        iterator insert(const_iterator pos, std::initializer_list<value_type> init)
        {
            return insert(pos, init.begin(), init.end());
        }

        template <class... Args>
        iterator emplace(const_iterator pos, Args&&... args)
        {
            const size_type index = indexOf(pos);

            // Construct first so args may safely refer to an element in *this.
            value_type newValue(std::forward<Args>(args)...);

            if (m_size == m_capacity)
            {
                const size_type newCapacity = growthCapacity(m_size + 1);
                value_type* oldData = data();
                const bool oldHeap = isHeap();
                const size_type oldCapacity = m_capacity;

                value_type* newData = AllocTraits::allocate(m_allocator, newCapacity);
                size_type constructed = 0;
                try
                {
                    for (size_type i = 0; i < index; ++i, ++constructed)
                        constructRelocated(newData + constructed, oldData[i]);

                    AllocTraits::construct(m_allocator, newData + constructed,
                                           std::move(newValue));
                    ++constructed;

                    for (size_type i = index; i < m_size; ++i, ++constructed)
                        constructRelocated(newData + constructed, oldData[i]);
                }
                catch (...)
                {
                    destroyRange(newData, newData + constructed);
                    AllocTraits::deallocate(m_allocator, newData, newCapacity);
                    throw;
                }

                destroyRange(oldData, oldData + m_size);
                if (oldHeap)
                    AllocTraits::deallocate(m_allocator, oldData, oldCapacity);

                activateHeap(newData);
                m_capacity = newCapacity;
                ++m_size;
                return data() + index;
            }

            value_type* d = data();
            if (index == m_size)
            {
                AllocTraits::construct(m_allocator, d + m_size, std::move(newValue));
                ++m_size;
                return d + index;
            }

            // Open one slot at [index]. Basic guarantee if T's move assignment throws.
            AllocTraits::construct(m_allocator, d + m_size,
                                   std::move_if_noexcept(d[m_size - 1]));
            ++m_size;
            std::move_backward(d + index, d + (m_size - 2), d + (m_size - 1));
            d[index] = std::move(newValue);
            return d + index;
        }

        iterator erase(const_iterator pos)
        {
            const size_type index = indexOf(pos);
            return erase(begin() + index, begin() + index + 1);
        }

        iterator erase(const_iterator first, const_iterator last)
        {
            const size_type firstIndex = indexOf(first);
            const size_type lastIndex = indexOf(last);
            assert(firstIndex <= lastIndex);

            const size_type count = lastIndex - firstIndex;
            if (count == 0)
                return begin() + firstIndex;

            value_type* d = data();
            std::move(d + lastIndex, d + m_size, d + firstIndex);
            destroyRange(d + (m_size - count), d + m_size);
            m_size -= count;
            return d + firstIndex;
        }

        void push_back(const value_type& value)
        {
            emplace_back(value);
        }

        void push_back(value_type&& value)
        {
            emplace_back(std::move(value));
        }

        template <class... Args>
        reference emplace_back(Args&&... args)
        {
            // Construct temporary before reserve because args may alias this container.
            if (m_size == m_capacity)
            {
                value_type value(std::forward<Args>(args)...);
                reserve(growthCapacity(m_size + 1));
                AllocTraits::construct(m_allocator, data() + m_size, std::move(value));
            }
            else
            {
                AllocTraits::construct(m_allocator, data() + m_size,
                                       std::forward<Args>(args)...);
            }
            ++m_size;
            return back();
        }

        void pop_back()
        {
            assert(m_size != 0);
            --m_size;
            AllocTraits::destroy(m_allocator, data() + m_size);
        }

        void resize(size_type count)
        {
            if (count < m_size)
            {
                destroyRange(data() + count, data() + m_size);
                m_size = count;
                return;
            }

            if (count == m_size)
                return;

            reserveForSize(count);
            size_type constructed = m_size;
            try
            {
                for (; constructed < count; ++constructed)
                    AllocTraits::construct(m_allocator, data() + constructed);
            }
            catch (...)
            {
                destroyRange(data() + m_size, data() + constructed);
                throw;
            }
            m_size = count;
        }

        void resize(size_type count, const value_type& value)
        {
            if (count < m_size)
            {
                destroyRange(data() + count, data() + m_size);
                m_size = count;
                return;
            }

            if (count == m_size)
                return;

            value_type valueCopy = value;
            reserveForSize(count);
            size_type constructed = m_size;
            try
            {
                for (; constructed < count; ++constructed)
                    AllocTraits::construct(m_allocator, data() + constructed, valueCopy);
            }
            catch (...)
            {
                destroyRange(data() + m_size, data() + constructed);
                throw;
            }
            m_size = count;
        }

        void swap(HybridArray& other)
        {
            if (this == &other)
                return;

            if constexpr (AllocTraits::propagate_on_container_swap::value)
            {
                HybridArray temp(std::move(*this));
                *this = std::move(other);
                other = std::move(temp);
            }
            else
            {
                assert(m_allocator == other.m_allocator &&
                    "swapping HybridArrays with unequal non-propagating allocators is undefined");

                HybridArray temp(std::move(*this), m_allocator);
                replaceWith(std::move(other));
                other.replaceWith(std::move(temp));
            }
        }

    private:
        [[nodiscard]] bool isHeap() const noexcept
        {
            return m_capacity > InlineCapacity;
        }

        [[nodiscard]] value_type* inlineData() noexcept
        {
            return reinterpret_cast<value_type*>(m_storage.inlineData);
        }

        [[nodiscard]] const value_type* inlineData() const noexcept
        {
            return reinterpret_cast<const value_type*>(m_storage.inlineData);
        }

        void activateHeap(value_type* ptr) noexcept
        {
            m_storage.heapData = ptr;
        }

        void resetToInline() noexcept
        {
            std::destroy_at(&m_storage);
            std::construct_at(&m_storage);
            m_capacity = InlineCapacity;
            m_size = 0;
        }

        void releaseHeap() noexcept
        {
            if (isHeap())
                AllocTraits::deallocate(m_allocator, m_storage.heapData, m_capacity);
        }

        void destroyElements() noexcept
        {
            destroyRange(data(), data() + m_size);
        }

        void destroyRange(value_type* first, value_type* last) noexcept
        {
            while (last != first)
            {
                --last;
                AllocTraits::destroy(m_allocator, last);
            }
        }

        void constructRelocated(value_type* dest, value_type& src)
        {
            AllocTraits::construct(m_allocator, dest, std::move_if_noexcept(src));
        }

        [[nodiscard]] bool containsAddress(const value_type* ptr) const noexcept
        {
            for (const value_type* it = begin(); it != end(); ++it)
            {
                if (it == ptr)
                    return true;
            }
            return false;
        }

        [[nodiscard]] size_type indexOf(const_iterator pos) const noexcept
        {
            assert(pos >= begin() && pos <= end());
            return static_cast<size_type>(pos - begin());
        }

        [[nodiscard]] size_type growthCapacity(size_type minimum) const
        {
            if (minimum > max_size())
                throw std::length_error("TY::HybridArray capacity overflow");

            size_type grown = m_capacity;
            if (grown < InlineCapacity)
                grown = InlineCapacity;

            if (grown <= max_size() / 2)
                grown *= 2;
            else
                grown = max_size();

            return std::max(grown, minimum);
        }

        void reserveForSize(size_type desired)
        {
            if (desired > m_capacity)
                reserve(growthCapacity(desired));
        }

        void reallocate(size_type newCapacity)
        {
            value_type* oldData = data();
            const bool oldHeap = isHeap();
            const size_type oldCapacity = m_capacity;

            value_type* newData = AllocTraits::allocate(m_allocator, newCapacity);
            size_type constructed = 0;
            try
            {
                for (; constructed < m_size; ++constructed)
                    constructRelocated(newData + constructed, oldData[constructed]);
            }
            catch (...)
            {
                destroyRange(newData, newData + constructed);
                AllocTraits::deallocate(m_allocator, newData, newCapacity);
                throw;
            }

            destroyRange(oldData, oldData + m_size);
            if (oldHeap)
                AllocTraits::deallocate(m_allocator, oldData, oldCapacity);

            activateHeap(newData);
            m_capacity = newCapacity;
        }

        void moveHeapToInline()
        {
            value_type* oldData = m_storage.heapData;
            const size_type oldCapacity = m_capacity;
            const size_type oldSize = m_size;

            // Save pointer locally, then reactivate inline storage.
            std::destroy_at(&m_storage);
            std::construct_at(&m_storage);
            m_capacity = InlineCapacity;

            value_type* newData = inlineData();
            size_type constructed = 0;
            try
            {
                for (; constructed < oldSize; ++constructed)
                    constructRelocated(newData + constructed, oldData[constructed]);
            }
            catch (...)
            {
                destroyRange(newData, newData + constructed);
                // Restore heap representation so *this remains destructible.
                activateHeap(oldData);
                m_capacity = oldCapacity;
                throw;
            }

            destroyRange(oldData, oldData + oldSize);
            AllocTraits::deallocate(m_allocator, oldData, oldCapacity);
            m_size = oldSize;
        }

        template <class InputIt>
        void appendRange(InputIt first, InputIt last)
        {
            using Category = typename std::iterator_traits<InputIt>::iterator_category;
            if constexpr (std::is_base_of_v<std::forward_iterator_tag, Category>)
            {
                const auto distance = std::distance(first, last);
                if (distance > 0)
                    reserveForSize(m_size + static_cast<size_type>(distance));
            }

            for (; first != last; ++first)
                emplace_back(*first);
        }

        void stealHeapFrom(HybridArray& other) noexcept
        {
            assert(other.isHeap());
            activateHeap(other.m_storage.heapData);
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            other.resetToInline();
        }

        void moveConstructFrom(HybridArray&& other)
        {
            if (other.isHeap())
            {
                stealHeapFrom(other);
                return;
            }

            reserve(other.size());
            for (auto& value : other)
                emplace_back(std::move(value));
            other.clear();
        }

        void replaceWith(HybridArray&& other)
        {
            clear();
            releaseHeap();
            resetToInline();

            if (m_allocator == other.m_allocator && other.isHeap())
            {
                stealHeapFrom(other);
            }
            else
            {
                reserve(other.size());
                for (auto& value : other)
                    emplace_back(std::move(value));
                other.clear();
            }
        }

        iterator insertRangeFromTemp(size_type index, HybridArray& temp)
        {
            if (temp.empty())
                return begin() + index;

            const size_type count = temp.size();
            HybridArray result(m_allocator);
            result.reserveForSize(m_size + count);

            for (size_type i = 0; i < index; ++i)
                result.emplace_back(std::move_if_noexcept(data()[i]));
            for (auto& value : temp)
                result.emplace_back(std::move(value));
            for (size_type i = index; i < m_size; ++i)
                result.emplace_back(std::move_if_noexcept(data()[i]));

            replaceWith(std::move(result));
            return begin() + index;
        }

    private:
        Storage m_storage;
        [[no_unique_address]] allocator_type m_allocator{};
        size_type m_size = 0;
        size_type m_capacity = InlineCapacity;
    };

    template <class T, std::size_t InlineCapacity, class Allocator>
    [[nodiscard]] bool operator==(const HybridArray<T, InlineCapacity, Allocator>& lhs,
                                  const HybridArray<T, InlineCapacity, Allocator>& rhs)
    {
        return lhs.size() == rhs.size() &&
            std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    template <class T, std::size_t InlineCapacity, class Allocator>
    [[nodiscard]] auto operator<=>(const HybridArray<T, InlineCapacity, Allocator>& lhs,
                                   const HybridArray<T, InlineCapacity, Allocator>& rhs)
    {
        const auto synthThreeWay = [](const T& a, const T& b)
        {
            if constexpr (requires { a <=> b; })
            {
                return a <=> b;
            }
            else
            {
                if (a < b)
                    return std::weak_ordering::less;
                if (b < a)
                    return std::weak_ordering::greater;
                return std::weak_ordering::equivalent;
            }
        };

        return std::lexicographical_compare_three_way(
            lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), synthThreeWay);
    }

    template <class T, std::size_t InlineCapacity, class Allocator>
    void swap(HybridArray<T, InlineCapacity, Allocator>& lhs,
              HybridArray<T, InlineCapacity, Allocator>& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template <class T, std::size_t InlineCapacity, class Allocator, class U>
    typename HybridArray<T, InlineCapacity, Allocator>::size_type
    erase(HybridArray<T, InlineCapacity, Allocator>& container, const U& value)
    {
        const auto oldSize = container.size();
        container.erase(std::remove(container.begin(), container.end(), value), container.end());
        return oldSize - container.size();
    }

    template <class T, std::size_t InlineCapacity, class Allocator, class Pred>
    typename HybridArray<T, InlineCapacity, Allocator>::size_type
    erase_if(HybridArray<T, InlineCapacity, Allocator>& container, Pred pred)
    {
        const auto oldSize = container.size();
        container.erase(std::remove_if(container.begin(), container.end(), pred), container.end());
        return oldSize - container.size();
    }
}
