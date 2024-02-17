#pragma once

#include <vector>

namespace tests {

class defined_random_sequence {
public:
  inline defined_random_sequence(std::initializer_list<std::size_t> seq)
      : seq_{seq} {
  }

  inline auto next() noexcept {
    return seq_[current_++];
  }

private:
  std::vector<std::size_t> seq_;
  std::size_t current_{0};
};

struct random_index_adapter {
  inline auto operator()() noexcept {
    return seq_->next();
  }

  defined_random_sequence* seq_;
};

struct deterministic_index_generator {
public:
  inline deterministic_index_generator(std::initializer_list<std::size_t> seq)
      : seq_{seq} {
  }

  inline auto operator()() const noexcept {
    return random_index_adapter{&seq_};
  }

  inline auto operator()(std::size_t min_idx,
                         std::ranges::sized_range auto const& range) const {
    return random_index_adapter{&seq_};
  }

  inline auto operator()(std::ranges::sized_range auto const& range) const {
    return random_index_adapter{&seq_};
  }

  inline auto operator()(std::size_t min_idx, std::size_t max_idx) const {
    return random_index_adapter{&seq_};
  }

  inline auto operator()(std::size_t max_idx) const {
    return random_index_adapter{&seq_};
  }

private:
  mutable defined_random_sequence seq_;
};

} // namespace tests
