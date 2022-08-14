
#pragma once

#include <random>
#include <unordered_set>

#include "operation.hpp"

namespace gal {
namespace details {

  template<std::size_t Size>
  struct nonunique_state {
    inline static constexpr std::size_t size = Size;
  };

  template<std::size_t Size>
  class unique_state {
  public:
    inline static constexpr std::size_t size = Size;

  public:
    inline unique_state() {
      existing_.reserve(Size);
    }

    inline void begin() noexcept {
      existing_.clear();
    }

    inline bool update(std::size_t selected) {
      return existing_.insert(selected).second;
    }

  private:
    std::unordered_set<std::size_t> existing_{};
  };

  template<typename Fn>
  concept index_producer = std::is_invocable_r_v<std::size_t, Fn>;

  template<std::size_t Size, index_producer Fn>
  std::size_t select_single(unique_state<Size>& s, Fn&& produce) {
    std::size_t idx{};
    do {
      idx = std::invoke(produce);
    } while (!s.update(idx));

    return idx;
  }

  template<std::size_t Size, index_producer Fn>
  std::size_t select_single(nonunique_state<Size> /*unused*/, Fn&& produce) {
    return std::invoke(std::forward<Fn>(produce));
  }

  template<typename Population, typename State, index_producer Fn>
  auto select_many(Population& population, State&& state, Fn&& produce) {
    state.begin();

    std::array<typename Population::iterator_t, State::size> result{};
    std::ranges::generate(
        result.begin(),
        State::size,
        [first = population.individuals().begin(), &state, &produce] {
          return first + select_single(state, std::forward<Fn>(produce));
        });

    return result;
  }

  template<std::size_t Size, bool Unique>
  using state_t =
      std::conditional_t<Unique, unique_state<Size>, nonunique_state<Size>>;

} // namespace details
} // namespace gal
