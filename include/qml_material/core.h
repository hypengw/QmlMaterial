#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <memory>
#include <span>
#include <utility>

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using idx         = std::ptrdiff_t;
using usize       = std::size_t;
using isize       = std::ptrdiff_t;
using byte        = std::byte;
using voidp       = void*;
using const_voidp = const void*;

namespace ycore 
{

template <class F>
struct y_combinator {
    F f; // the lambda will be stored here
    
    // a forwarding operator():
    template <class... Args>
    decltype(auto) operator()(Args&&... args) const {
        // we pass ourselves to f, then the arguments.
        return f(*this, std::forward<Args>(args)...);
    }
};

template <class F>
y_combinator(F) -> y_combinator<F>;

}