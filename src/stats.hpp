
#pragma once

#include "population.hpp"

#include <concepts>
#include <type_traits>

namespace gal {
namespace stats {

  template<typename Value, typename Population>
  concept node_value =
      !std::is_final_v<Value> &&
      std::is_constructible_v<Value, Population const&, Value const&>;

  namespace details {

    template<typename Population, node_value<Population>... Values>
    class node;

    template<typename Population>
    class node<Population> {};

    template<typename Population, node_value<Population> Value>
    class node<Population, Value> : public Value {
    public:
      using value_t = Value;

    public:
      template<typename Population>
      inline node(Population const& population,
                  node<Population> const& previous)
          : value_t{population, previous} {
      }
    };

    template<typename Population,
             node_value<Population> Value,
             node_value<Population>... Rest>
    class node<Population, Value, Rest...> : public Value,
                                             public node<Population, Rest...> {
    public:
      using value_t = Value;
      using base_t = node<Population, Rest...>;

    public:
      template<typename Population>
      inline node(Population const& population, node const& previous)
          : value_t{population, previous}
          , base_t{previous} {
      }
    };

  } // namespace details

  class generation {
  public:
    template<typename Population>
    inline generation(Population const& population,
                      generation const& previous) noexcept
        : value_{previous.value_ + 1} {
    }

    inline auto generation_value() const noexcept {
      return value_;
    }

  private:
    std::size_t value_{};
  };

  class population_size {
  public:
    template<typename Population>
    inline population_size(Population const& population,
                           population_size const& previous) noexcept
        : value_{population.current_size()} {
    }

    inline auto population_size_value() const noexcept {
      return value_;
    }

  private:
    std::size_t value_{};
  };

  template<typename Fitness>
  concept ordered_fitness = fitness<Fitness> && std::totally_ordered<Fitness>;

  template<ordered_fitness Fitness, typename FitnessTag>
  class best_fitness {
  public:
    using fitness_t = Fitness;
    using fitness_tag_t = FitnessTag;

    inline static constexpr fitness_tag_t fitness_tag{};

  public:
    template<typename Population>
    inline best_fitness(Population const& population,
                        best_fitness const& previous) noexcept
        : value_{/* population.best(fitness_tag).evaluation.get(fitness_tag) */} {
    }

    inline auto best_fitness_value() const noexcept {
      return value_;
    }

  private:
    std::optional<fitness_t> value_{};
  };

  template<typename Population, node_value<Population>... Values>
  class statistics : public details::node<Population, Values...> {
  private:
    using base_t = details::node<Population, Values...>;

  private:
    inline statistics(Population const& population, statistics const& previous)
        : base_t{population, previous} {
    }

  public:
    inline auto next(Population const& population) const {
      return statistics(population, *this);
    }
  };

} // namespace stats
} // namespace gal
