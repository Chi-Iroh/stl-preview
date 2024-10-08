//
// Created by Chi-Iroh on 03/10/2024.
//

#ifndef PREVIEW_BIT_BYTESWAP_H_
#define PREVIEW_BIT_BYTESWAP_H_

#include <climits>
#include <cstdint>
#include <type_traits>

#include "preview/config.h"
#include "preview/__bit/bit_cast.h"
#include "preview/__type_traits/negation.h"

#if PREVIEW_CXX_VERSION >= 23
  #include <bit>
#endif

namespace preview {
namespace detail {

template<typename T>
constexpr T byteswap_impl(T n, std::true_type /* is unsigned */) noexcept {
  constexpr T byte_mask{ (1 << CHAR_BIT) - 1 };
  T result{ 0 };

  for (unsigned i{ 0 }; i < sizeof(T); i++) {
    const std::size_t nth_byte{ CHAR_BIT * (sizeof(T) - i - 1) };
    const T byte{ static_cast<T>((n & (byte_mask << nth_byte)) >> nth_byte) };
    result |= byte << (i * CHAR_BIT);
  }
  return result;
}

template<typename T, typename UnsignedT = typename std::make_unsigned<T>::type>
constexpr UnsignedT byteswap_impl(T n, std::false_type /* is signed */) {
  return preview::bit_cast<UnsignedT>(preview::detail::byteswap_impl<UnsignedT>(preview::bit_cast<UnsignedT>(n), std::true_type{}));
}

} // namespace detail

template<typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
constexpr inline T byteswap(T n) noexcept {
#if PREVIEW_CXX_VERSION >= 23
  return std::byteswap<T>(n);
#else
  return sizeof(T) == 1 ? n : preview::detail::byteswap_impl<T>(n, std::is_unsigned<T>{});
#endif
}

} // namespace preview

#endif