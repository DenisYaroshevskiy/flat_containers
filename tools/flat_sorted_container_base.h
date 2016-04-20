#ifndef TOOLS_FLAT_SORTED_CONTAINER_BASE_H_
#define TOOLS_FLAT_SORTED_CONTAINER_BASE_H_

#include <algorithm>
#include <memory>
#include <utility>
#include <cassert>

namespace tools {
namespace internal {

template <typename Compare>
struct sort_and_unique : private Compare {
  template <typename Cont>
  void operator()(Cont* rhs) {
    using value_type = typename Cont::value_type;

    std::sort(rhs->begin(), rhs->end(),
              [this](const value_type& lhs, const value_type& rhs) {
      return Compare::operator()(lhs, rhs);
    });

    rhs->erase(
        std::unique(rhs->begin(), rhs->end(),
                    [this](const value_type& lhs, const value_type& rhs) {
          return Compare::equal(lhs, rhs);
        }),
        rhs->end());
  }
};

template <typename Traits, class UnderlyingType>
class flat_sorted_container_base : private Traits {
  using traits = Traits;

  struct traits_compare {
    explicit traits_compare(traits tr) : tr_(tr) {}

    template <typename Lhs, typename Rhs>
    bool operator()(const Lhs& lhs, const Rhs& rhs) {
      return tr_(lhs, rhs);
    }

   private:
    traits tr_;
  };

  traits_compare traits_comp() { return traits_compare(*this); }

 public:
  using compare = Traits;
  using key_compare = compare;
  using value_compare = compare;
  using key_value_compare = compare;

  using underlying_type = UnderlyingType;
  using key_type = typename key_compare::key_type;
  using value_type = typename key_compare::value_type;

  using size_type = typename underlying_type::size_type;
  using difference_type = typename underlying_type::difference_type;
  using reference = typename underlying_type::reference;
  using const_reference = typename underlying_type::const_reference;
  using pointer = typename underlying_type::pointer;
  using const_pointer = typename underlying_type::const_pointer;
  using iterator = typename underlying_type::iterator;
  using const_iterator = typename underlying_type::const_iterator;
  using reverse_iterator = typename underlying_type::reverse_iterator;
  using const_reverse_iterator =
      typename underlying_type::const_reverse_iterator;

  // scoped object to do operations on body, without keeping order
  using unsafe_region =
      std::unique_ptr<underlying_type, sort_and_unique<compare>>;

  flat_sorted_container_base() = default;

  explicit flat_sorted_container_base(underlying_type body)
      : body_(std::move(body)) {
    unsafe_access();
  }

  template <typename It>
  flat_sorted_container_base(It first, It last)
      : body_(first, last) {
    unsafe_access();
  }

  // methods-------------------------------------------------------------------

  // returns scoped object, that gives access to underlying storrage.
  // at destruction, sorts and does unification.
  //
  // if you know, that on exit of the region, storrage is already sorted and
  // unified - call unsafe_region::release()
  unsafe_region unsafe_access() { return unsafe_region(&body_); }

  // get_allocator()

  iterator begin() { return body_.begin(); }
  const_iterator begin() const { return body_.begin(); }
  const_iterator cbegin() const { return body_.cbegin(); }

  iterator end() { return body_.end(); }
  const_iterator end() const { return body_.end(); }
  const_iterator cend() const { return body_.cend(); }

  reverse_iterator rbegin() { return body_.rbegin(); }
  const_reverse_iterator rbegin() const { return body_.rbegin(); }
  const_reverse_iterator crbegin() const { return body_.crbegin(); }

  reverse_iterator rend() { return body_.rend(); }
  const_reverse_iterator rend() const { return body_.rend(); }
  const_reverse_iterator crend() const { return body_.crend(); }

  bool empty() const { return body_.empty(); }
  size_type size() const { return body_.size(); }
  size_type max_size() const { return body_.max_size(); }

  void clear() { body_.clear(); }

  std::pair<iterator, bool> insert(value_type value) {
    auto pos = lower_bound(key_value_comp().key_from_value(value));
    if (pos != end() && Traits::equal(*pos, value))
      return std::make_pair(pos, false);
    return std::make_pair(body_.insert(pos, std::move(value)), true);
  }

  iterator insert(const_iterator hint, value_type value) {
    // todo(dyaroshev) use hint
    return insert(std::move(value)).first;
  }

  template <class InputIt>
  void insert(InputIt first, InputIt last) {
    auto tail = body_.insert(body_.end(), first, last);
    std::sort(tail, body_.end(), traits_comp());
    std::inplace_merge(body_.begin(), tail, body_.end(), traits_comp());
    body_.erase(
        std::unique(body_.begin(), body_.end(),
                    [this](const value_type& lhs, const value_type& rhs) {
          return Traits::equal(lhs, rhs);
        }),
        body_.end());
  }

  // void insert( std::initializer_list<value_type> ilist );

  template <class... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {  // NOLINT
    // todo(dyaroshev) reverse - use emplace for insert
    return insert(value_type(std::forward<Args>(args)...));  // NOLINT
  }

  template <class... Args>
  iterator emplace_hint(const_iterator hint, Args&&... args) {  // NOLINT
    // todo(dyaroshev) reverse - use emplace for insert
    return insert(hint, value_type(std::forward<Args>(args)...));  // NOLINT
  }

  iterator erase(const_iterator position) {
    assert(position != cend());
    return body_.erase(position);
  }
  void erase(const_iterator first, const_iterator last) {
    body_.erase(first, last);
  }

  size_type erase(const key_type& key) {
    auto range = equal_range(key);
    auto res = static_cast<size_type>(std::distance(range.first, range.second));
    erase(range.first, range.second);
    return res;
  }

  void swap(flat_sorted_container_base& other) { body_.swap(other.body_); }

  size_type count(const key_type& key) const {
    auto range = equal_range(key);
    return static_cast<size_type>(std::distance(range.first, range.second));
  }

  iterator find(const key_type& key) {
    auto pos = lower_bound(key);
    if (pos == end() || !Traits::equal(*pos, key))
      return end();
    return pos;
  }
  const_iterator find(const key_type& key) const {
    auto pos = lower_bound(key);
    if (pos == end() || !Traits::equal(*pos, key))
      return end();
    return pos;
  }

  std::pair<iterator, iterator> equal_range(const key_type& key) {
    return std::equal_range(body_.begin(), body_.end(), key, traits_comp());
  }

  std::pair<const_iterator, const_iterator> equal_range(
      const key_type& key) const {
    return std::equal_range(body_.begin(), body_.end(), key, key_value_comp());
  }

  iterator lower_bound(const key_type& key) {
    return std::lower_bound(body_.begin(), body_.end(), key, key_value_comp());
  }

  const_iterator lower_bound(const key_type& key) const {
    return std::lower_bound(body_.begin(), body_.end(), key, key_value_comp());
  }

  iterator upper_bound(const key_type& key) {
    return std::upper_bound(body_.begin(), body_.end(), key, key_value_comp());
  }

  const_iterator upper_bound(const key_type& key) const {
    return std::upper_bound(body_.begin(), body_.end(), key, key_value_comp());
  }

  key_compare key_comp() const { return traits(*this); }

  value_compare value_comp() const { return traits(*this); }

  key_value_compare key_value_comp() const { return traits(*this); }

  // regular-------------------------------------------------------------------
  // std::map defines it's comparators based on full value compares,
  // not provided cmps, so equvalent maps are not removed from sets, for example

  friend void swap(flat_sorted_container_base& lhs,
                   flat_sorted_container_base& rhs) {
    lhs.swap(rhs);
  }

  friend bool operator==(const flat_sorted_container_base& lhs,
                         const flat_sorted_container_base& rhs) {
    return lhs.body_ == rhs.body_;
  }

  friend bool operator!=(const flat_sorted_container_base& lhs,
                         const flat_sorted_container_base& rhs) {
    return !operator==(lhs, rhs);
  }

  // totally-ordered-----------------------------------------------------------

  friend bool operator<(const flat_sorted_container_base& lhs,
                        const flat_sorted_container_base& rhs) {
    return lhs.body_ < rhs.body_;
  }

  friend bool operator<=(const flat_sorted_container_base& lhs,
                         const flat_sorted_container_base& rhs) {
    return !(rhs < lhs);
  }

  friend bool operator>(const flat_sorted_container_base& lhs,
                        const flat_sorted_container_base& rhs) {
    return rhs < lhs;
  }

  friend bool operator>=(const flat_sorted_container_base& lhs,
                         const flat_sorted_container_base& rhs) {
    return !(lhs < rhs);
  }

 private:
  underlying_type body_;
};

}  // namespace internal

}  // namespace tools

#endif  // TOOLS_FLAT_SORTED_CONTAINER_BASE_H_
