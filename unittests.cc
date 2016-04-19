// Copyright (c) 2016 Yandex. All rights reserved.
// Author: Denis Yaroshevskiy <dyaroshev@yandex-team.ru>

#include "flat_map.h"
#include "flat_set.h"

#include <algorithm>
#include <iterator>
#include <iostream>
#include <string>

namespace {

std::string Serialize(const std::string& that) {
  return that;
}

std::string Serialize(int that) {
  return std::to_string(that);
}

template <typename First, typename Second>
std::string Serialize(const std::pair<First, Second>& that) {
  return std::string("{") + Serialize(that.first) + ", " +
         Serialize(that.second) + std::string("}");
}

template <typename Range>
std::string Serialize(const Range& range) {
  std::string res = "[";
  auto it = std::begin(range);
  if (it != std::end(range)) {
    res += Serialize(*it);
    for (++it; it != std::end(range); ++it) {
      res += ", " + Serialize(*it);
    }
  }
  return res + ']';
}

template <typename Lhs, typename Rhs>
std::string ExpectedActualMsg(const Lhs& expected, const Rhs& rhs) {
  return "\nExpected: " + Serialize(expected) + "\nDesired: " + Serialize(rhs) +
         '\n';
}

struct AnyPairEquals {
  template <typename Lhs, typename Rhs>
  bool operator()(const Lhs& lhs, const Rhs& rhs) {
    return lhs == rhs;
  }
  template <typename L1, typename L2, typename R1, typename R2>
  bool operator()(const std::pair<L1, L2>& lhs, const std::pair<R1, R2>& rhs) {
    return lhs.first == rhs.first && lhs.second == rhs.second;
  }
};

template <typename FlatMap, typename TestRange>
bool check_map(const FlatMap& fl_map, const TestRange& test) {
  auto test_beg = std::begin(test);
  auto test_end = std::end(test);
  if (static_cast<typename FlatMap::size_type>(
          std::distance(test_beg, test_end)) != fl_map.size())
    return false;
  return std::equal(fl_map.begin(), fl_map.end(), test_beg, AnyPairEquals());
}

class OptionalLogger {
 public:
  explicit OptionalLogger(bool enabled) : enabled_{enabled} {}

  template <typename Rhs>
  friend const OptionalLogger& operator<<(const OptionalLogger& stream,
                                          Rhs&& rhs) {
    if (stream.enabled_)
      std::clog << std::forward<Rhs>(rhs);
    return stream;
  }

 private:
  bool enabled_;
};

OptionalLogger EXPECT_TRUE(bool test) {
  return OptionalLogger(!test);
}

template <typename Lhs, typename Rhs>
OptionalLogger EXPECT_EQ(const Lhs& lhs, const Rhs& rhs) {
  return EXPECT_TRUE(lhs == rhs) << " == ";
}

template <typename Lhs, typename Rhs>
OptionalLogger EXPECT_NE(const Lhs& lhs, const Rhs& rhs) {
  return EXPECT_TRUE(lhs != rhs) << " != ";
}

template <typename Lhs, typename Rhs>
OptionalLogger EXPECT_LT(const Lhs& lhs, const Rhs& rhs) {
  return EXPECT_TRUE(lhs < rhs) << " < ";
}

template <typename Lhs, typename Rhs>
OptionalLogger EXPECT_LE(const Lhs& lhs, const Rhs& rhs) {
  return EXPECT_TRUE(lhs <= rhs) << " <= ";
}

template <typename Lhs, typename Rhs>
OptionalLogger EXPECT_GT(const Lhs& lhs, const Rhs& rhs) {
  return EXPECT_TRUE(lhs > rhs) << " > ";
}

template <typename Lhs, typename Rhs>
OptionalLogger EXPECT_GE(const Lhs& lhs, const Rhs& rhs) {
  return EXPECT_TRUE(lhs >= rhs) << " >= ";
}

}  // namespace

using RegularFlatMap = base::flat_map<std::string, int>;
using RegularFlatSet = base::flat_set<std::string>;

std::vector<RegularFlatMap::value_type> RegularKeyValuePairs() {
  RegularFlatMap::value_type key_value_pairs[] = {
      {"b", 3},
      {"b", 5},
      {"a", 3},
      {"fr", 3},
      {"fa", 3},
      {"d", 12},
      {"a", 7},
      {"long", 1233},
      {"q", 0},
  };
  return std::vector<RegularFlatMap::value_type>(std::begin(key_value_pairs),
                                                 std::end(key_value_pairs));
}

struct FlatMapTest
{
  void Insertions();
  void RegularTypeAndConstructors();
  void Getters();
  void Erasers();
};

std::vector<RegularFlatSet::value_type> RegularKeys() {
  auto key_value_pairs = RegularKeyValuePairs();
  std::vector<RegularFlatSet::value_type> keys;
  keys.reserve(key_value_pairs.size());
  for (const auto& kv_pair : key_value_pairs)
    keys.push_back(kv_pair.first);
  return keys;
}

template <typename FlatCont, typename StdCont, typename KeyValuePairs>
void insert_test(const KeyValuePairs& key_value_pairs) {
  {
    const char prefix[] = "<it, bool> insert (value) ";
    FlatCont fl_cont;
    StdCont test_cont;
    for (const auto& test_case : key_value_pairs) {
      std::pair<typename FlatCont::iterator, bool> fl_insert =
          fl_cont.insert(test_case);
      std::pair<typename StdCont::iterator, bool> test_insert =
          test_cont.insert(test_case);

      EXPECT_TRUE(check_map(fl_cont, test_cont))
          << prefix << ExpectedActualMsg(test_cont, fl_cont);
      EXPECT_EQ(std::distance(fl_cont.begin(), fl_insert.first),
                std::distance(test_cont.begin(), test_insert.first))
          << prefix;
      EXPECT_EQ(fl_insert.second, test_insert.second) << prefix;
    }
  }
  {
    const char prefix[] = "it insert (hint, value) ";
    FlatCont fl_cont;
    StdCont test_cont;
    auto fl_hint = fl_cont.begin();
    auto test_hint = test_cont.begin();
    for (const auto& test_case : key_value_pairs) {
      fl_hint = fl_cont.insert(fl_hint, test_case);
      test_hint = test_cont.insert(test_hint, test_case);

      EXPECT_TRUE(check_map(fl_cont, test_cont))
          << prefix << ExpectedActualMsg(test_cont, fl_cont);
      EXPECT_EQ(std::distance(fl_cont.begin(), fl_hint),
                std::distance(test_cont.begin(), test_hint))
          << prefix;
    }
  }
  {
    const char prefix[] = "void insert (first, last) ";
    FlatCont fl_cont;
    StdCont test_cont;
    fl_cont.insert(std::begin(key_value_pairs), std::end(key_value_pairs));
    test_cont.insert(std::begin(key_value_pairs), std::end(key_value_pairs));
    EXPECT_TRUE(check_map(fl_cont, test_cont))
        << prefix << ExpectedActualMsg(test_cont, fl_cont);
  }
  {
    const char prefix[] = "<it, bool> emplace(args...) ";
    FlatCont fl_cont;
    StdCont test_cont;
    for (const auto& test_case : key_value_pairs) {
      std::pair<typename FlatCont::iterator, bool> fl_emplace =
          fl_cont.emplace(test_case);
      std::pair<typename StdCont::iterator, bool> test_emplace =
          test_cont.emplace(test_case);

      EXPECT_TRUE(check_map(fl_cont, test_cont))
          << prefix << ExpectedActualMsg(test_cont, fl_cont);
      EXPECT_EQ(std::distance(fl_cont.begin(), fl_emplace.first),
                std::distance(test_cont.begin(), test_emplace.first))
          << prefix;
      EXPECT_EQ(fl_emplace.second, test_emplace.second) << prefix;
    }
  }

  {
    const char prefix[] = "it emplace_hint (hint, value) ";
    FlatCont fl_cont;
    StdCont test_cont;
    auto fl_hint = fl_cont.begin();
    auto test_hint = test_cont.begin();
    for (const auto& test_case : key_value_pairs) {
      fl_hint = fl_cont.emplace_hint(fl_hint, test_case);
      test_hint = test_cont.emplace_hint(test_hint, test_case);

      EXPECT_TRUE(check_map(fl_cont, test_cont))
          << prefix << ExpectedActualMsg(test_cont, fl_cont);
      EXPECT_EQ(std::distance(fl_cont.begin(), fl_hint),
                std::distance(test_cont.begin(), test_hint))
          << prefix;
    }
  }
}



void FlatMapTest::Insertions() {
  using FlatMap = RegularFlatMap;
  using FlatSet = RegularFlatSet;
  using StdMap = RegularFlatMap::std_map;
  using StdSet = RegularFlatSet::std_set;

  auto key_value_pairs = RegularKeyValuePairs();
  auto keys = RegularKeys();

  {
    const char prefix[] = "operator[] ";
    FlatMap fl_map;
    StdMap test_map;
    for (const auto& test_case : key_value_pairs) {
      fl_map[test_case.first] = test_case.second;
      test_map[test_case.first] = test_case.second;
      EXPECT_TRUE(check_map(fl_map, test_map))
          << prefix << ExpectedActualMsg(test_map, fl_map);
    }
  }

  insert_test<FlatMap, StdMap>(key_value_pairs);
  insert_test<FlatSet, StdSet>(keys);
}

template <typename FlatCont, typename StdCont, typename KeyValuePairs>
void regular_type_test(const KeyValuePairs& key_value_pairs) {
  {
    const char prefix[] = "default ";
    FlatCont fl_cont;
    StdCont test_cont;
    EXPECT_TRUE(check_map(fl_cont, test_cont))
        << prefix << ExpectedActualMsg(test_cont, fl_cont);
  }
  {
    const char prefix[] = "from underlying type ";
    FlatCont fl_cont((typename FlatCont::underlying_type(key_value_pairs)));
    StdCont test_cont(key_value_pairs.begin(), key_value_pairs.end());
    EXPECT_TRUE(check_map(fl_cont, test_cont))
        << prefix << ExpectedActualMsg(test_cont, fl_cont);
  }
  {
    const char prefix[] = "from iterators ";
    FlatCont fl_cont(key_value_pairs.begin(), key_value_pairs.end());
    StdCont test_cont(key_value_pairs.begin(), key_value_pairs.end());
    EXPECT_TRUE(check_map(fl_cont, test_cont))
        << prefix << ExpectedActualMsg(test_cont, fl_cont);
  }
  {
    const char prefix[] = "copy ";
    FlatCont fl_cont(key_value_pairs.begin(), key_value_pairs.end());
    StdCont test_cont(key_value_pairs.begin(), key_value_pairs.end());

    FlatCont fl_cont1 = fl_cont;
    StdCont test_cont1 = test_cont;
    EXPECT_TRUE(check_map(fl_cont, test_cont))
        << prefix << ExpectedActualMsg(test_cont, fl_cont);
  }
  {
    const char prefix[] = "copy assign";
    FlatCont fl_cont(key_value_pairs.begin(), key_value_pairs.end());
    StdCont test_cont(key_value_pairs.begin(), key_value_pairs.end());

    FlatCont fl_cont1;
    fl_cont1 = fl_cont;
    StdCont test_cont1;
    test_cont1 = test_cont;
    EXPECT_TRUE(check_map(fl_cont, test_cont))
        << prefix << ExpectedActualMsg(test_cont, fl_cont);
  }
  {
    const char prefix[] = "move ";
    FlatCont fl_cont(key_value_pairs.begin(), key_value_pairs.end());
    StdCont test_cont(key_value_pairs.begin(), key_value_pairs.end());

    FlatCont fl_cont1 = std::move(fl_cont);
    StdCont test_cont1 = std::move(test_cont);
    EXPECT_TRUE(check_map(fl_cont, test_cont))
        << prefix << ExpectedActualMsg(test_cont, fl_cont);
  }
  {
    const char prefix[] = "move assign";
    FlatCont fl_cont(key_value_pairs.begin(), key_value_pairs.end());
    StdCont test_cont(key_value_pairs.begin(), key_value_pairs.end());

    FlatCont fl_cont1;
    fl_cont1 = std::move(fl_cont);
    StdCont test_cont1;
    test_cont1 = std::move(test_cont);
    EXPECT_TRUE(check_map(fl_cont, test_cont))
        << prefix << ExpectedActualMsg(test_cont, fl_cont);
  }
  {
    const char prefix[] = "comparators ";
    FlatCont lhs(key_value_pairs.begin(), key_value_pairs.end());
    FlatCont rhs(key_value_pairs.begin(), key_value_pairs.end());
    EXPECT_EQ(lhs, rhs);
    lhs.erase(lhs.begin() + 5, lhs.end());
    EXPECT_NE(lhs, rhs) << prefix << ExpectedActualMsg(lhs, rhs);
    EXPECT_TRUE(lhs < rhs) << prefix << ExpectedActualMsg(lhs, rhs);
    EXPECT_LE(lhs, rhs) << prefix << ExpectedActualMsg(lhs, rhs);

    EXPECT_GT(rhs, lhs) << prefix << ExpectedActualMsg(lhs, rhs);
    EXPECT_GE(rhs, lhs) << prefix << ExpectedActualMsg(lhs, rhs);
  }
  {
    const char prefix[] = "swap ";
    FlatCont lhs(key_value_pairs.begin(), key_value_pairs.end());
    FlatCont rhs(key_value_pairs.begin(), key_value_pairs.end());
    lhs.erase(lhs.begin() + 5, lhs.end());

    typename FlatCont::underlying_type original_lhs(lhs.begin(), lhs.end());
    typename FlatCont::underlying_type original_rhs(rhs.begin(), rhs.end());

    swap(lhs, rhs);
    EXPECT_TRUE(check_map(lhs, original_rhs))
        << prefix << ExpectedActualMsg(original_rhs, lhs);
    EXPECT_TRUE(check_map(rhs, original_lhs))
        << prefix << ExpectedActualMsg(original_lhs, rhs);

    lhs.swap(rhs);
    EXPECT_TRUE(check_map(lhs, original_lhs))
        << prefix << ExpectedActualMsg(original_lhs, lhs);
    EXPECT_TRUE(check_map(rhs, original_rhs))
        << prefix << ExpectedActualMsg(original_rhs, rhs);
  }
}

void FlatMapTest::RegularTypeAndConstructors(){
  using FlatMap = RegularFlatMap;
  using StdMap = RegularFlatMap::std_map;
  using FlatSet = RegularFlatSet;
  using StdSet = RegularFlatSet::std_set;

  auto key_value_pairs = RegularKeyValuePairs();
  auto keys = RegularKeys();

  regular_type_test<FlatMap, StdMap>(key_value_pairs);
  regular_type_test<FlatSet, StdSet>(keys);
}

template <typename FlatCont,
          typename StdCont,
          typename KeyValuePairs,
          typename Keys>
void getters_test(const KeyValuePairs& key_value_pairs, const Keys& keys) {
  {
    const char prefix[] = "count(key) ";
    FlatCont fl_cont(key_value_pairs.begin(), key_value_pairs.end());
    StdCont test_cont(key_value_pairs.begin(), key_value_pairs.end());
    for (const auto& key : keys) {
      EXPECT_EQ(fl_cont.count(key), test_cont.count(key)) << prefix << key;
    }
  }
  {
    const char prefix[] = "empty, size, max_size";
    FlatCont fl_cont;
    EXPECT_TRUE(fl_cont.empty()) << prefix;
    EXPECT_EQ(fl_cont.size(), typename FlatCont::size_type(0)) << prefix;
    EXPECT_EQ(fl_cont.max_size(),
              typename FlatCont::underlying_type().max_size())
        << prefix;
  }
  {
    const char prefix[] = "it find (key) ";
    FlatCont fl_cont(key_value_pairs.begin(), key_value_pairs.end());
    StdCont test_cont(key_value_pairs.begin(), key_value_pairs.end());
    const FlatCont& fl_const = fl_cont;
    const StdCont& test_const = test_cont;

    for (const auto& key : keys) {
      auto fl_found = fl_cont.find(key);
      auto test_found = test_cont.find(key);
      auto fl_const_found = fl_const.find(key);
      auto test_const_found = test_const.find(key);

      EXPECT_EQ(std::distance(fl_cont.begin(), fl_found),
                std::distance(test_cont.begin(), test_found))
          << prefix;
      EXPECT_EQ(std::distance(fl_found, fl_cont.end()),
                std::distance(test_found, test_cont.end()))
          << prefix;
      EXPECT_EQ(std::distance(fl_const.begin(), fl_const_found),
                std::distance(test_const.begin(), test_const_found))
          << prefix;
      EXPECT_EQ(std::distance(fl_const_found, fl_const.end()),
                std::distance(test_const_found, test_const.end()))
          << prefix;
    }
  }
  {
    const char prefix[] = "<it, it> equal_range (key) ";
    FlatCont fl_cont(key_value_pairs.begin(), key_value_pairs.end());
    StdCont test_cont(key_value_pairs.begin(), key_value_pairs.end());
    const FlatCont& fl_const = fl_cont;
    const StdCont& test_const = test_cont;

    for (const auto& key : keys) {
      auto fl_found = fl_cont.equal_range(key);
      auto test_found = test_cont.equal_range(key);
      auto fl_const_found = fl_const.equal_range(key);
      auto test_const_found = test_const.equal_range(key);

      EXPECT_EQ(std::distance(fl_cont.begin(), fl_found.first),
                std::distance(test_cont.begin(), test_found.first))
          << prefix;
      EXPECT_EQ(std::distance(fl_found.first, fl_found.second),
                std::distance(test_found.first, test_found.second))
          << prefix;
      EXPECT_EQ(std::distance(fl_found.second, fl_cont.end()),
                std::distance(test_found.second, test_cont.end()))
          << prefix;

      EXPECT_EQ(std::distance(fl_const.begin(), fl_const_found.first),
                std::distance(test_const.begin(), test_const_found.first))
          << prefix;
      EXPECT_EQ(std::distance(fl_const_found.first, fl_const_found.second),
                std::distance(test_const_found.first, test_const_found.second))
          << prefix;
      EXPECT_EQ(std::distance(fl_const_found.second, fl_const.end()),
                std::distance(test_const_found.second, test_const.end()))
          << prefix;
    }
  }
  {
    const char prefix[] = "it lower_bound (key) ";
    FlatCont fl_cont(key_value_pairs.begin(), key_value_pairs.end());
    StdCont test_cont(key_value_pairs.begin(), key_value_pairs.end());
    const FlatCont& fl_const = fl_cont;
    const StdCont& test_const = test_cont;

    for (const auto& key : keys) {
      auto fl_found = fl_cont.lower_bound(key);
      auto test_found = test_cont.lower_bound(key);
      auto fl_const_found = fl_const.lower_bound(key);
      auto test_const_found = test_const.lower_bound(key);

      EXPECT_EQ(std::distance(fl_cont.begin(), fl_found),
                std::distance(test_cont.begin(), test_found))
          << prefix;
      EXPECT_EQ(std::distance(fl_found, fl_cont.end()),
                std::distance(test_found, test_cont.end()))
          << prefix;
      EXPECT_EQ(std::distance(fl_const.begin(), fl_const_found),
                std::distance(test_const.begin(), test_const_found))
          << prefix;
      EXPECT_EQ(std::distance(fl_const_found, fl_const.end()),
                std::distance(test_const_found, test_const.end()))
          << prefix;
    }
  }
  {
    const char prefix[] = "it upper_bound (key) ";
    FlatCont fl_cont(key_value_pairs.begin(), key_value_pairs.end());
    StdCont test_cont(key_value_pairs.begin(), key_value_pairs.end());
    const FlatCont& fl_const = fl_cont;
    const StdCont& test_const = test_cont;

    for (const auto& key : keys) {
      auto fl_found = fl_cont.upper_bound(key);
      auto test_found = test_cont.upper_bound(key);
      auto fl_const_found = fl_const.upper_bound(key);
      auto test_const_found = test_const.upper_bound(key);

      EXPECT_EQ(std::distance(fl_cont.begin(), fl_found),
                std::distance(test_cont.begin(), test_found))
          << prefix;
      EXPECT_EQ(std::distance(fl_found, fl_cont.end()),
                std::distance(test_found, test_cont.end()))
          << prefix;
      EXPECT_EQ(std::distance(fl_const.begin(), fl_const_found),
                std::distance(test_const.begin(), test_const_found))
          << prefix;
      EXPECT_EQ(std::distance(fl_const_found, fl_const.end()),
                std::distance(test_const_found, test_const.end()))
          << prefix;
    }
  }
}

void FlatMapTest::Getters() {
  using FlatMap = RegularFlatMap;
  using StdMap = RegularFlatMap::std_map;
  using FlatSet = RegularFlatSet;
  using StdSet = RegularFlatSet::std_set;

  auto key_value_pairs = RegularKeyValuePairs();
  auto keys = RegularKeys();

  {
    const char prefix[] = "at(key) ";
    FlatMap fl_map(key_value_pairs.begin(), key_value_pairs.end());
    StdMap test_map(key_value_pairs.begin(), key_value_pairs.end());
    const FlatMap& fl_const = fl_map;
    const StdMap& test_const = test_map;
    for (const auto& key : keys) {
      EXPECT_EQ(fl_map.at(key), test_map.at(key)) << prefix << key;
      EXPECT_EQ(fl_const.at(key), test_const.at(key)) << prefix << key;
    }
  }

  auto keys_with_one_extra(keys);

  keys_with_one_extra.emplace_back("not found");

  getters_test<FlatMap, StdMap>(key_value_pairs, keys_with_one_extra);
  getters_test<FlatSet, StdSet>(keys, keys_with_one_extra);
}

template <typename FlatCont,
          typename StdCont,
          typename KeyValuePairs,
          typename Keys>
void erasers_test(const KeyValuePairs& key_value_pairs, const Keys& keys) {
  {
    const char prefix[] = "size_type erase(const key&) ";
    FlatCont fl_cont(key_value_pairs.begin(), key_value_pairs.end());
    StdCont test_cont(key_value_pairs.begin(), key_value_pairs.end());
    for (const auto& key : keys) {
      EXPECT_EQ(fl_cont.erase(key), test_cont.erase(key)) << prefix;
      EXPECT_TRUE(check_map(fl_cont, test_cont))
          << prefix << ExpectedActualMsg(test_cont, fl_cont);
    }
  }
  //  erase with iterators works like in underlying type, not in a map
  {
    const char prefix[] = "it erase (const_it) ";
    FlatCont fl_cont(key_value_pairs.begin(), key_value_pairs.end());
    typename FlatCont::underlying_type test_cont(fl_cont.begin(),
                                                 fl_cont.end());

    for (const auto& key : keys) {
      auto fl_it = fl_cont.find(key);
      auto test_it = std::find_if(
          test_cont.begin(), test_cont.end(),
          [&fl_cont, &key](const typename FlatCont::value_type& val) {
            return fl_cont.key_value_comp().equal(key, val);
          });
      if (test_it == test_cont.end()) {
        EXPECT_EQ(fl_it, fl_cont.end()) << prefix;
        continue;
      }

      fl_it = fl_cont.erase(fl_it);
      test_it = test_cont.erase(test_it);

      EXPECT_TRUE(check_map(fl_cont, test_cont))
          << prefix << ExpectedActualMsg(test_cont, fl_cont);

      EXPECT_EQ(std::distance(fl_cont.begin(), fl_it),
                std::distance(test_cont.begin(), test_it))
          << prefix;
      EXPECT_EQ(std::distance(fl_it, fl_cont.end()),
                std::distance(test_it, test_cont.end()))
          << prefix;
    }
  }
}

void FlatMapTest::Erasers() {
  using FlatMap = RegularFlatMap;
  using StdMap = RegularFlatMap::std_map;
  using FlatSet = RegularFlatSet;
  using StdSet = RegularFlatSet::std_set;

  auto key_value_pairs = RegularKeyValuePairs();
  auto keys = RegularKeys();

  auto keys_with_one_extra(keys);
  keys_with_one_extra.emplace_back("not found");

  erasers_test<FlatMap, StdMap>(key_value_pairs, keys_with_one_extra);
  erasers_test<FlatSet, StdSet>(keys, keys_with_one_extra);
}

int main() {
  FlatMapTest test;
  test.Getters();
  test.Erasers();
  test.RegularTypeAndConstructors();
  test.Insertions();
}
