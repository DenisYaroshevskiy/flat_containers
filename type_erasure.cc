#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class streamable {
 public:
  template <typename T>
  streamable(T that)
      : body_{new obj_t<T>(that)} {};
  template <typename T>
  streamable& operator=(T that) {
    streamable tmp(that);
    return *this = std::move(tmp);
  }

  // defining default constructor depends on style
  streamable() = default;
  streamable(const streamable& that) : body_(that.body_->clone()) {}
  streamable(streamable&&) = default;
  streamable& operator=(const streamable& that) {
    auto tmp = that;
    return *this = std::move(tmp);
  }
  streamable& operator=(streamable&&) = default;

  friend std::ostream& operator<<(std::ostream& out, const streamable& that) {
    return that.body_->stream(out);
  }

 private:
  struct concept_t {
    virtual std::ostream& stream(std::ostream& stream) const = 0;
    virtual std::unique_ptr<concept_t> clone() const = 0;
    virtual ~concept_t() {}
  };

  template <typename T>
  class obj_t : public concept_t {
   public:
    obj_t(T that) : body_{std::move(that)} {}

    std::ostream& stream(std::ostream& stream) const final {
      return stream << body_;
    }

    std::unique_ptr<concept_t> clone() const final {
      return std::unique_ptr<concept_t>{new obj_t{body_}};
    }

   private:
    T body_;
  };

  std::unique_ptr<concept_t> body_;
};

int main() {
  std::vector<streamable> test{1, 8u, std::string("abc")};
  test.emplace_back();
  test.back() = std::string("ddd");
  auto test_copy = test;                   // copyable
  auto test_copy2 = std::move(test_copy);  // movable
  std::copy(test.begin(), test.end(),
            std::ostream_iterator<streamable>(std::cout, " "));
  std::cout << std::endl;
}
