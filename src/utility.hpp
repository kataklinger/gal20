
#pragma once

#include "operation.hpp"

#include <random>
#include <unordered_set>
#include <vector>

namespace gal {
namespace details {

  struct nonunique_state {
    inline explicit nonunique_state(std::size_t size) noexcept
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

  class unique_state {
  public:
    inline explicit unique_state(std::size_t size) noexcept
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

  private:
    std::size_t size_;
    std::unordered_set<std::size_t> existing_{};
  };

  template<typename Fn>
  concept index_producer = std::is_invocable_r_v<std::size_t, Fn>;

  template<index_producer Fn>
  std::size_t select_single(unique_state& s, Fn&& produce) {
    std::size_t idx{};
    do {
      idx = std::invoke(produce);
    } while (!s.update(idx));

    return idx;
  }

  template<index_producer Fn>
  std::size_t select_single(nonunique_state /*unused*/, Fn&& produce) {
    return std::invoke(std::forward<Fn>(produce));
  }

  template<typename Population, typename State, index_producer Fn>
  auto select_many(Population& population, State&& state, Fn&& produce) {
    state.begin();

    auto size = std::min(state.size(), population.current_size());
    std::vector<typename Population::iterator_t> result{size};

    std::ranges::generate_n(
        result.begin(),
        size,
        [first = population.individuals().begin(), &state, &produce] {
          return first + select_single(state, std::forward<Fn>(produce));
        });

    return result;
  }

  template<bool Unique>
  using state_t = std::conditional_t<Unique, unique_state, nonunique_state>;

  template<typename Replaced, typename Replacement>
  struct parentship : std::tuple<Replaced, Replacement> {
    using std::tuple<Replaced, Replacement>::tuple;
  };

  template<typename Replaced, typename Replacement>
  auto const& get_parent(parentship<Replaced, Replacement> const& pair) {
    return std::get<0>(pair);
  }

  template<typename Replaced, typename Replacement>
  auto& get_parent(parentship<Replaced, Replacement>& pair) {
    return std::get<0>(pair);
  }

  template<typename Replaced, typename Replacement>
  auto&& get_parent(parentship<Replaced, Replacement>&& pair) {
    return std::move(std::get<0>(pair));
  }

  template<typename Replaced, typename Replacement>
  auto const& get_child(parentship<Replaced, Replacement> const& pair) {
    return std::get<1>(pair);
  }

  template<typename Replaced, typename Replacement>
  auto& get_child(parentship<Replaced, Replacement>& pair) {
    return std::get<1>(pair);
  }

  template<typename Replaced, typename Replacement>
  auto&& get_child(parentship<Replaced, Replacement>&& pair) {
    return std::move(std::get<1>(pair));
  }

} // namespace details
} // namespace gal
