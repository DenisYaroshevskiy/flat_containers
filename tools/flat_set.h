// Copyright (c) 2016 Yandex. All rights reserved.
// Author: Denis Yaroshevskiy <dyaroshev@yandex-team.ru>

#ifndef TOOLS_FLAT_SET_H_
#define TOOLS_FLAT_SET_H_

#include <algorithm>
#include <functional>
#include <set>
#include <utility>
#include <vector>

#include "flat_sorted_container_base.h"

namespace tools {

namespace internal {

template <typename Key, class Compare>
struct set_compare : private Compare {
  using key_type = Key;
  using value_type = Key;

  using Compare::operator();

  template <typename Lhs, typename Rhs>
  bool equal(const Lhs& lhs, const Rhs& rhs) {
    return !operator()(lhs, rhs) && !operator()(rhs, lhs);
  }

  const key_type& key_from_value(const value_type& value) { return value; }

  key_type& key_from_value(value_type& value) { return value; }  // NOLINT
};

}  // namespace internal

// std::vector is not particulary friendly with const value type,
// so, unlike std::map, we use non const Key
template <typename Key,
          class Compare = std::less<Key>,
          class UnderlyingType = std::vector<Key>>
class flat_set : public internal::flat_sorted_container_base<
                     internal::set_compare<Key, Compare>,
                     UnderlyingType> {
  using base_type =
      internal::flat_sorted_container_base<internal::set_compare<Key, Compare>,
                                           UnderlyingType>;

 public:
  using base_type::base_type;
  using std_set = std::set<Key>;
};

}  // namespace tools

#endif  // TOOLS_FLAT_SET_H_
