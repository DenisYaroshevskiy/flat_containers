#ifndef TOOLS_FLAT_SET_H_
#define TOOLS_FLAT_SET_H_

#include <set>
#include <vector>

#include "flat_sorted_container_base.h"

namespace tools {

namespace internal {

template <typename Key, class Compare>
struct set_compare : private Compare {
  using key_type = Key;
  using value_type = Key;

  bool cmp(const key_type& lhs, const key_type& rhs) const {
    return Compare::operator()(lhs, rhs);
  }

  template <typename Lhs, typename Rhs>
  bool equal(const Lhs& lhs, const Rhs& rhs) const {
    return !cmp(lhs, rhs) && !cmp(rhs, lhs);
  }

  const key_type& key_from_value(const value_type& value) const {
    return value;
  }

  key_type& key_from_value(value_type& value) const { return value; }
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
