
#pragma once

#include "operation.hpp"

#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gal {

class nonunique_sample {
public:
  struct outer {};

public:
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
  class outer {
  public:
    inline bool operator()(std::size_t index, std::size_t count) {
      if (auto& used = usage_[index]; used < count) {
        ++used;
        return true;
      }

      return false;
    }

  private:
    std::unordered_map<std::size_t, std::size_t> usage_;
  };

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

using sample_group = std::tuple<std::size_t, std::size_t>;

template<typename Fn>
concept outer_index_producer = std::is_invocable_r_v<sample_group, Fn>;

template<typename Fn>
concept inner_index_producer =
    std::is_invocable_r_v<std::size_t, Fn, std::size_t>;

namespace details {

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
    return std::invoke(produce);
  }

  template<outer_index_producer Out>
  inline std::size_t sample_outer(nonunique_sample::outer& /*unused*/,
                                  Out& outter) {
    return std::get<0>(std::invoke(outter));
  }

  template<outer_index_producer Out>
  inline std::size_t sample_outer(unique_sample::outer& state, Out& outer) {
    while (true) {
      if (auto [index, count] = std::invoke(outer); state(index, count)) {
        return index;
      }
    }
  }

  template<typename Population, typename State>
  inline auto initialize_sample(Population& population, State& state) {
    state.begin();

    auto size = std::min(state.size(), population.current_size());
    return std::vector<typename Population::iterator_t>{size};
  }

} // namespace details

template<typename Population, typename State, index_producer Fn>
auto sample_many(Population& population, State&& state, Fn&& produce) {
  auto result = details::initialize_sample(population, state);

  std::ranges::generate_n(
      result.begin(),
      result.size(),
      [first = std::ranges::begin(population.individuals()), &state, &produce] {
        return first + details::sample_single(state, produce);
      });

  return result;
}

template<typename State, index_producer Fn>
std::vector<std::size_t> sample_many(State&& state, Fn&& produce) {
  std::vector<std::size_t> result(state.size());

  std::ranges::generate_n(result.begin(), state.size(), [&state, &produce] {
    return details::sample_single(state, produce);
  });

  return result;
}

template<typename Population,
         typename State,
         outer_index_producer Out,
         inner_index_producer In>
auto sample_many(Population& population,
                 State&& state,
                 Out&& outer,
                 In&& inner) {
  auto result = details::initialize_sample(population, state);

  typename State::outer outer_state{};

  std::ranges::generate_n(
      result.begin(),
      result.size(),
      [first = std::ranges::begin(population.individuals()),
       &state,
       &outer_state,
       &outer,
       &inner] {
        auto index = details::sample_outer(outer_state, outer);
        return first + details::sample_single(state, [index, &inner] {
                 return std::invoke(inner, index);
               });
      });

  return result;
}

template<bool Unique>
using sample_t = std::conditional_t<Unique, unique_sample, nonunique_sample>;

} // namespace gal
