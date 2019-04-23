#pragma once

#include <type_traits>

namespace ext
{
    template<class T>
    class bitenum
    {
        static_assert(std::is_enum_v<T>);

    public:

        operator bool()
        {
            return std::underlying_type_t<T>(_enum) != 0;
        }

        bitenum()
        {
        }

        bitenum(const T& val)
            : _enum(val)
        {
        }

        bitenum(const bitenum& val)
            : _enum(val._enum)
        {
        }

        bitenum& operator=(const T& val)
        {
            _enum = val;
            return *this;
        }

        bitenum& operator=(const bitenum& val)
        {
            _enum = val._enum;
            return *this;
        }

        bitenum& operator&=(const T& val)
        {
            std::underlying_type_t<T>(_enum) &= std::underlying_type_t<T>(val);
            return *this;
        }

        bitenum& operator&=(const bitenum& val)
        {
            std::underlying_type_t<T>(_enum) &= std::underlying_type_t<T>(val._enum);
            return *this;
        }

        bitenum& operator|=(const T& val)
        {
            std::underlying_type_t<T>(_enum) |= std::underlying_type_t<T>(val);
            return *this;
        }

        bitenum& operator|=(const bitenum& val)
        {
            std::underlying_type_t<T>(_enum) |= std::underlying_type_t<T>(val._enum);
            return *this;
        }

        bitenum operator&(const T& val) const
        {
            return std::underlying_type_t<T>(_enum) & std::underlying_type_t<T>(val);
        }

        bitenum operator&(const bitenum& val) const
        {
            return std::underlying_type_t<T>(_enum) & std::underlying_type_t<T>(val._enum);
        }

        bitenum operator|(const T& val) const
        {
            return std::underlying_type_t<T>(_enum) | std::underlying_type_t<T>(val);
        }

        bitenum operator|(const bitenum& val) const
        {
            return std::underlying_type_t<T>(_enum) | std::underlying_type_t<T>(val._enum);
        }

        bool operator==(const T& val) const
        {
            return _enum == val;
        }

        bool operator==(const bitenum& val) const
        {
            return _enum == val._enum;
        }

    private:

        T _enum;
    };

    template<typename T>
    void swapByteOrder(T& val)
    {
        static_assert(sizeof(T) % 2 == 0);

        for (auto i = 0; i < sizeof(T)/2; ++i)
            std::swap(reinterpret_cast<uint8_t*>(&val)[i], reinterpret_cast<uint8_t*>(&value)[sizeof(T)-i-1]);
    }

    template<typename T>
    T convByteOrder(const T& val)
    {
        static_assert(sizeof(T) % 2 == 0);
        T ret;

        for (int i = sizeof(T), j = 0; i != 0;)
            reinterpret_cast<const uint8_t*>(&ret)[j++] = reinterpret_cast<const uint8_t*>(&val)[--i];

        return ret;
    }
}