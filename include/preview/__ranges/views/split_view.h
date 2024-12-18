//
// Created by yonggyulee on 2/3/24.
//

#ifndef PREVIEW_RANGES_VIEWS_SPLIT_VIEW_H_
#define PREVIEW_RANGES_VIEWS_SPLIT_VIEW_H_

#include <type_traits>
#include <utility>

#include "preview/__algorithm/ranges/search.h"
#include "preview/__concepts/constructible_from.h"
#include "preview/__concepts/copy_constructible.h"
#include "preview/__concepts/default_initializable.h"
#include "preview/__concepts/same_as.h"
#include "preview/__functional/equal_to.h"
#include "preview/__iterator/detail/have_cxx20_iterator.h"
#include "preview/__iterator/indirectly_comparable.h"
#include "preview/__iterator/iterator_tag.h"
#include "preview/__memory/addressof.h"
#include "preview/__ranges/begin.h"
#include "preview/__ranges/empty.h"
#include "preview/__ranges/end.h"
#include "preview/__ranges/forward_range.h"
#include "preview/__ranges/iterator_t.h"
#include "preview/__ranges/non_propagating_cache.h"
#include "preview/__ranges/range_value_t.h"
#include "preview/__ranges/subrange.h"
#include "preview/__ranges/view.h"
#include "preview/__ranges/view_interface.h"
#include "preview/__ranges/viewable_range.h"
#include "preview/__ranges/views/all.h"
#include "preview/__ranges/views/single.h"
#include "preview/__type_traits/conjunction.h"
#include "preview/__type_traits/remove_cvref.h"
#include "preview/__utility/cxx20_rel_ops.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#endif

namespace preview {
namespace ranges {

template<typename V, typename Pattern>
class split_view : public view_interface<split_view<V, Pattern>> {
  template<typename R, bool = forward_range<R>::value /* false */>
  struct check_range : std::false_type {};
  template<typename R>
  struct check_range<R, true>
      : conjunction<
            constructible_from<V, views::all_t<R>>,
            constructible_from<Pattern, single_view<range_value_t<R>>>
        > {};

 public:
  static_assert(forward_range<V>::value, "Constraints not satisfied");
  static_assert(forward_range<Pattern>::value, "Constraints not satisfied");
  static_assert(view<V>::value, "Constraints not satisfied");
  static_assert(view<Pattern>::value, "Constraints not satisfied");
  static_assert(indirectly_comparable<iterator_t<V>, iterator_t<Pattern>, equal_to>::value, "Constraints not satisfied");

  class sentinel;
  class iterator;
  friend class sentinel;
  friend class iterator;

  class iterator {
   public:
    friend class sentinel;

    using iterator_concept = forward_iterator_tag;
    using iterator_category = input_iterator_tag;
    using value_type = subrange<iterator_t<V>>;
    using difference_type = range_difference_t<V>;
#if !PREVIEW_STD_HAVE_CXX20_ITERATOR
    using pointer = void;
    using reference = value_type;
#endif

    iterator() = default;

    PREVIEW_CONSTEXPR_AFTER_CXX17
    iterator(split_view& parent, iterator_t<V> current, subrange<iterator_t<V>> next)
        : parent_(preview::addressof(parent))
        , cur_(std::move(current))
        , next_(std::move(next))
        , trailing_empty_(false) {}

    constexpr iterator_t<V> base() const {
      return cur_;
    }

    constexpr value_type operator*() const {
      return {cur_, next_.begin()};
    }

    constexpr iterator& operator++() {
      const auto last = ranges::end(parent_->base_);
      cur_ = next_.begin();
      if (cur_ != last) {
        cur_ = next_.end();
        if (cur_ == last) {
          trailing_empty_ = true;
          next_ = {cur_, cur_};
        } else {
          next_ = parent_->find_next(cur_);
        }
      } else {
        trailing_empty_ = false;
      }

      return *this;
    }

    constexpr iterator operator++(int) {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

#if defined(_MSC_VER) && _MSC_VER <= 1920
    friend
#else
    friend constexpr
#endif
    bool operator==(const iterator& x, const iterator& y) {
      return x.cur_ == y.cur_ && x.trailing_empty_ == y.trailing_empty_;
    }

#if defined(_MSC_VER) && _MSC_VER <= 1920
    friend
#else
    friend constexpr
#endif
    bool operator!=(const iterator& x, const iterator& y) {
      return !(x == y);
    }

   private:
    split_view* parent_ = nullptr;
    iterator_t<V> cur_{};
    subrange<iterator_t<V>> next_{};
    bool trailing_empty_ = false;
  };

  class sentinel {
   public:
    sentinel() = default;

    constexpr explicit sentinel(split_view& parent)
        : end_(ranges::end(parent.base_)) {}

    friend constexpr bool operator==(const iterator& x, const sentinel& y) {
      return y.equal_with(x);
    }

    friend constexpr bool operator!=(const iterator& x, const sentinel& y) {
      return !(x == y);
    }

    friend constexpr bool operator==(const sentinel& y, const iterator& x) {
      return x == y;
    }

    friend constexpr bool operator!=(const sentinel& y, const iterator& x) {
      return !(x == y);
    }

   private:
    constexpr bool equal_with(const iterator& x) const {
      using namespace preview::rel_ops;
      return x.cur_ == end_ && !x.trailing_empty_;
    }
    sentinel_t<V> end_ = sentinel_t<V>();
  };

  template<bool B = default_initializable<V>::value && default_initializable<Pattern>::value, std::enable_if_t<B, int> = 0>
  constexpr split_view() {}

  constexpr explicit split_view(V base, Pattern pattern)
      : base_(std::move(base))
      , pattern_(std::move(pattern)) {}

  template<typename R, std::enable_if_t<check_range<R>::value, int> = 0>
  constexpr explicit split_view(R&& r, range_value_t<R> e)
      : base_(views::all(std::forward<R>(r)))
      , pattern_(views::single(std::move(e))) {}

  template<typename Dummy = void, std::enable_if_t<conjunction<std::is_void<Dummy>,
      copy_constructible<V>
  >::value, int> = 0>
  constexpr V base() const& {
    return base_;
  }

  constexpr V base() && {
    return std::move(base_);
  }

  constexpr iterator begin() {
    auto first = ranges::begin(base_);
    if (!cached_begin_.has_value())
      cached_begin_.emplace(find_next(first));
    return {*this, first, *cached_begin_};
  }

  constexpr auto end() {
    return end_impl(common_range<V>{});
  }

 private:
  constexpr iterator end_impl(std::true_type /* common_range<V> */) {
    return iterator{*this, ranges::end(base_), {}};
  }
  constexpr sentinel end_impl(std::false_type /* common_range<V> */) {
    return sentinel{*this};
  }

  constexpr subrange<iterator_t<V>> find_next(iterator_t<V> it) {
    auto b_e = ranges::search(preview::ranges::make_subrange(it, ranges::end(base_)), pattern_);
    auto b = b_e.begin();
    auto e = b_e.end();

    if (b != ranges::end(base_) && ranges::empty(pattern_)) {
      ++b;
      ++e;
    }

    return {std::move(b), std::move(e)};
  }

  V base_{};
  Pattern pattern_{};
  non_propagating_cache<subrange<iterator_t<V>>> cached_begin_{};
};

namespace detail {

template<typename R, typename POV, bool = input_range<R>::value>
struct same_with_range_value : std::false_type {};
template<typename R, typename POV>
struct same_with_range_value<R, POV, true> : same_as<range_value_t<R>, remove_cvref_t<POV>> {};

template<typename R, std::enable_if_t<input_range<R>::value, int> = 0>
constexpr auto make_split_view_impl(R&& r, range_value_t<R> patteern, std::true_type /* range_value_t */) {
  return split_view<views::all_t<R>, single_view<range_value_t<R>>>(std::forward<R>(r), std::move(patteern));
}

template<typename R, typename P>
constexpr auto make_split_view_impl(R&& r, P&& pattern, std::false_type /* range_value_t */) {
  return split_view<views::all_t<R>, views::all_t<P>>(std::forward<R>(r), std::forward<P>(pattern));
}

} // namespace detail

template<typename R, typename P>
constexpr auto make_split_view(R&& r, P&& pattern) {
  return detail::make_split_view_impl(
      std::forward<R>(r),
      std::forward<P>(pattern),
      detail::same_with_range_value<R, P>{}
  );
}

#if __cplusplus >= 201703L

template<typename R, typename P>
split_view(R&&, P&&)
    -> split_view<
            std::enable_if_t<negation<std::is_same<range_value_t<R>, remove_cvref_t<P>>>::value,
          views::all_t<R>>,
          views::all_t<P>
        >;

template<typename R>
split_view(R&&, range_value_t<R>)
    -> split_view<
            std::enable_if_t<input_range<R>::value,
        views::all_t<R>>,
        single_view<range_value_t<R>>
    >;

#endif

} // namespace ranges
} // namespace preview

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif // PREVIEW_RANGES_VIEWS_SPLIT_VIEW_H_
