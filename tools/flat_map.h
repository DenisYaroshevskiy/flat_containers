// Copyright (c) 2016 Yandex. All rights reserved.
// Author: Denis Yaroshevskiy <dyaroshev@yandex-team.ru>

#ifndef TOOLS_FLAT_MAP_H_
#define TOOLS_FLAT_MAP_H_

#include <algorithm>
#include <functional>
#include <map>
#include <utility>
#include <vector>

#include "flat_sorted_container_base.h"

namespace tools {

namespace internal {

template <typename Key, typename T, class Compare>
struct map_compare : private Compare {
  using key_type = Key;
  using value_type = std::pair<key_type, T>;

  using Compare::operator();

  bool operator()(const value_type& lhs, const key_type& rhs) const {
    return operator()(lhs.first, rhs);
  }

  bool operator()(const key_type& lhs, const value_type& rhs) const {
    return operator()(lhs, rhs.first);
  }

  bool operator()(const value_type& lhs, const value_type& rhs) const {
    return operator()(lhs.first, rhs.first);
  }

  template <typename Lhs, typename Rhs>
  bool equal(const Lhs& lhs, const Rhs& rhs) const {
    return !operator()(lhs, rhs) && !operator()(rhs, lhs);
  }

  const key_type& key_from_value(const value_type& value) {
    return value.first;
  }

  key_type& key_from_value(value_type& value) {  // NOLINT
    return value.first;
  }
};

}  // namespace internal

// std::vector is not particulary friendly with const value type,
// so, unlike std::map, we use non const Key
template <typename Key,
          typename T,
          class Compare = std::less<Key>,
          class UnderlyingType = std::vector<std::pair<Key, T>>>
class flat_map : public internal::flat_sorted_container_base<
                     internal::map_compare<Key, T, Compare>,
                     UnderlyingType> {
  using base_type = internal::flat_sorted_container_base<
      internal::map_compare<Key, T, Compare>,
      UnderlyingType>;

 public:
  // typedefs------------------------------------------------------------------

  // ours
  using std_map = std::map<Key, T, Compare>;
  using mapped_type = T;
  using key_type = typename base_type::key_type;

  // ctors---------------------------------------------------------------------
  using base_type::base_type;

  // methods-------------------------------------------------------------------

  mapped_type& at(const key_type& key) {
    return const_cast<T&>(static_cast<const flat_map&>(*this).at(key));
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
    T& res = guard->emplace(pos, std::move(key), T())->second;
    guard.release();
    return res;
  }
};

}  // namespace tools

#endif  // TOOLS_FLAT_MAP_H_
