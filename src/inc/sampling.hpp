
#pragma once

#include "operation.hpp"

#include <random>
#include <unordered_set>
#include <vector>

namespace gal {

struct nonunique_sample {
  inline explicit nonunique_sample(std::size_t size) noexcept
      : size_{size} {
  }

  inline void begin() const noexcept {
  }

  inline auto size() const noexcept {
    return size_;
  }

private:
  std::size_t size_;
};

class unique_sample {
public:
  inline explicit unique_sample(std::size_t size) noexcept
      : size_{size} {
    existing_.reserve(size);
  }

  inline void begin() noexcept {
    existing_.clear();
  }

  inline bool update(std::size_t selected) {
    return existing_.insert(selected).second;
  }

  inline auto size() const noexcept {
    return size_;
  }

  inline bool full() const noexcept {
    return existing_.size() == size_;
  }

private:
  std::size_t size_;
  std::unordered_set<std::size_t> existing_{};
};

template<typename Fn>
concept index_producer = std::is_invocable_r_v<std::size_t, Fn>;

template<index_producer Fn>
std::size_t sample_single(unique_sample& s, Fn&& produce) {
  std::size_t idx{};
  do {
    idx = std::invoke(produce);
  } while (!s.update(idx));

  return idx;
}

template<index_producer Fn>
std::size_t sample_single(nonunique_sample /*unused*/, Fn&& produce) {
  return std::invoke(std::forward<Fn>(produce));
}

template<typename Population, typename State, index_producer Fn>
auto sample_many(Population& population, State&& state, Fn&& produce) {
  state.begin();

  auto size = std::min(state.size(), population.current_size());
  std::vector<typename Population::iterator_t> result{size};

  std::ranges::generate_n(
      result.begin(),
      size,
      [first = population.individuals().begin(), &state, &produce] {
        return first + sample_single(state, produce);
      });

  return result;
}

template<typename State, index_producer Fn>
std::vector<std::size_t> sample_many(State&& state, Fn&& produce) {
  std::vector<std::size_t> result{state.size(), std::allocator<std::size_t>{}};

  std::ranges::generate_n(result.begin(), state.size(), [&state, &produce] {
    return sample_single(state, produce);
  });

  return result;
}

template<bool Unique>
using sample_t = std::conditional_t<Unique, unique_sample, nonunique_sample>;

} // namespace gal
