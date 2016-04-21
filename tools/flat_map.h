#ifndef TOOLS_FLAT_MAP_H_
#define TOOLS_FLAT_MAP_H_

#include <map>
#include <utility>
#include <vector>

#include "flat_sorted_container_base.h"

namespace tools {

namespace internal {

template <typename Key, typename T, class Compare>
struct base_map_traits : private Compare {
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<key_type, T>;

  bool cmp(const key_type& lhs, const key_type& rhs) const {
    return Compare::operator()(lhs, rhs);
  }

  bool cmp(const value_type& lhs, const key_type& rhs) const {
    return cmp(lhs.first, rhs);
  }

  bool cmp(const key_type& lhs, const value_type& rhs) const {
    return cmp(lhs, rhs.first);
  }

  bool cmp(const value_type& lhs, const value_type& rhs) const {
    return cmp(lhs.first, rhs.first);
  }

  template <typename Lhs, typename Rhs>
  bool equal(const Lhs& lhs, const Rhs& rhs) const {
    return !cmp(lhs, rhs) && !cmp(rhs, lhs);
  }

  const key_type& key_from_value(const value_type& value) {
    return value.first;
  }

  key_type& key_from_value(value_type& value) { return value.first; }
};

// std::vector is not particulary friendly with const value type,
// so, unlike std::map, we use non const Key
template <typename Traits, class UnderlyingType>
class flat_map_base
    : public flat_sorted_container_base<Traits, UnderlyingType> {
  using base_type = flat_sorted_container_base<Traits, UnderlyingType>;

 public:
  // typedefs------------------------------------------------------------------

  // ours
  using std_map = typename Traits::std_map;
  using mapped_type = typename Traits::mapped_type;
  using key_type = typename base_type::key_type;

  // ctors---------------------------------------------------------------------
  using base_type::base_type;

  // methods-------------------------------------------------------------------

  mapped_type& at(const key_type& key) {
    return const_cast<mapped_type&>(
        static_cast<const flat_map_base&>(*this).at(key));
  }

  const mapped_type& at(const key_type& key) const {
    auto pos = this->find(key);
    if (pos == this->end())
      throw std::out_of_range("flat_map::at");
    return pos->second;
  }

  mapped_type& operator[](key_type key) {
    auto pos = this->lower_bound(key);
    if (pos != this->end() && this->key_value_comp().equal(*pos, key)) {
      return pos->second;
    }
    auto guard = this->unsafe_access();
    mapped_type& res =
        guard->emplace(pos, std::move(key), mapped_type())->second;
    guard.release();
    return res;
  }
};

}  // namespace internal

template <typename Key, typename T, class Compare>
class flat_map_traits
    : public internal::base_map_traits<Key, T, Compare>,
      public internal::std_unique_traits<flat_map_traits<Key, T, Compare>>,
      public internal::std_sort_traits<flat_map_traits<Key, T, Compare>> {
  using base_traits = internal::base_map_traits<Key, T, Compare>;

 public:
  using std_map = std::map<Key, T, Compare>;

  using base_traits::base_traits;
};

template <typename Key,
          typename T,
          class Compare = std::less<Key>,
          class UnderlyingType = std::vector<std::pair<Key, T>>>
using flat_map =
    internal::flat_map_base<flat_map_traits<Key, T, Compare>, UnderlyingType>;

}  // namespace tools

#endif  // TOOLS_FLAT_MAP_H_
