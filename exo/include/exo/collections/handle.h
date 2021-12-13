#pragma once
#include "exo/maths/numerics.h"
#include "exo/hash.h"

namespace exo
{
template <typename T>
struct Pool;
template <typename T>
struct PoolIterator;
template <typename T>
struct ConstPoolIterator;

/// --- Handle type (Typed index that can be invalid)
template <typename T>
struct Handle
{
    static constexpr Handle invalid()
    {
        return Handle();
    }

    constexpr Handle()
        : Handle(u32_invalid, u32_invalid)
    {
    }

    constexpr Handle &operator=(const Handle &other)        = default;
    constexpr bool    operator==(const Handle &other) const = default;

    [[nodiscard]] constexpr u32 value() const
    {
        return index;
    }

    [[nodiscard]] constexpr u64 hash() const
    {
        return u64(index) << 32 | gen;
    }

    [[nodiscard]] constexpr bool is_valid() const
    {
        return index != u32_invalid && gen != u32_invalid;
    }

  private:
    constexpr Handle(u32 _index, u32 _gen)
        : index(_index)
        , gen(_gen)
    {
    }

    u32 index;
    u32 gen;

    friend Pool<T>;
    friend PoolIterator<T>;
    friend ConstPoolIterator<T>;
    friend inline usize hash_value(const Handle<T> &h)
    {
        usize seed = hash_value(h.index);
        hash_combine(seed, h.gen);
        return seed;
    }
};
} // namespace exo
