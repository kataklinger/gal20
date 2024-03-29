
#pragma once

#include "sampling.hpp"

#include <tuple>

namespace gal::replace {

namespace details {

  template<typename Replaced, typename Replacement>
  struct replacement_view : std::tuple<Replaced, Replacement> {
    using std::tuple<Replaced, Replacement>::tuple;
  };

  template<typename Replaced, typename Replacement>
  replacement_view(Replaced&, Replacement&)
      -> replacement_view<Replaced&, Replacement&>;

  template<typename Replaced, typename Replacement>
  auto& get_parent(replacement_view<Replaced, Replacement>& pair) {
    return std::get<0>(pair);
  }

  template<typename Replaced, typename Replacement>
  auto& get_parent(replacement_view<Replaced, Replacement>&& pair) {
    return std::get<0>(pair);
  }

  template<typename Replaced, typename Replacement>
  auto& get_child(replacement_view<Replaced, Replacement>& pair) {
    return std::get<1>(pair);
  }

  template<typename Replaced, typename Replacement>
  auto& get_child(replacement_view<Replaced, Replacement>&& pair) {
    return std::get<1>(pair);
  }

  template<std::ranges::sized_range Replaced, typename Offspring>
  inline auto apply_replaced(Replaced& replaced, Offspring& offspring) {
    return std::views::iota(std::size_t{}, std::ranges::size(replaced)) |
           std::views::transform([&replaced, &offspring](std::size_t idx) {
             return replacement_view{replaced[idx], get_child(offspring[idx])};
           });
  }

  template<typename Offspring>
  inline auto extract_children(Offspring& offspring) {
    return offspring |
           std::views::transform([](auto& o) { return get_child(o); });
  }

} // namespace details

template<typename Generator, std::size_t Elitism, typename FitnessTag>
class random {
public:
  using generator_t = Generator;
  using fitness_tag_t = FitnessTag;

private:
  using distribution_t = std::uniform_int_distribution<std::size_t>;

  inline static constexpr fitness_tag_t fitness_tag{};

public:
  inline explicit random(generator_t& generator,
                         countable_t<Elitism> /*unused*/) noexcept
      : generator_{&generator} {
  }

  template<typename Population,
           replacement_range<typename Population::iterator_t,
                             typename Population::individual_t> Offspring>
  inline auto operator()(Population& population, Offspring&& offspring) const {
    auto allowed = population.current_size() - Elitism;
    if constexpr (Elitism > 0) {
      population.sort(fitness_tag);
    }

    distribution_t dist{0, allowed - 1};
    auto to_replace = sample_many(
        population,
        unique_sample{std::min(allowed, std::ranges::size(offspring))},
        [this, &dist]() { return Elitism + dist(*generator_); });

    return population.replace(details::apply_replaced(to_replace, offspring));
  }

private:
  generator_t* generator_;
};

template<typename Generator, std::size_t Elitism>
class random_raw : public random<Generator, Elitism, raw_fitness_tag> {
private:
  using base_t = random<Generator, Elitism, raw_fitness_tag>;

public:
  inline explicit random_raw(Generator& generator,
                             countable_t<Elitism> size) noexcept
      : base_t{generator, size} {
  }
};

template<typename Generator, std::size_t Elitism>
class random_scaled : public random<Generator, Elitism, scaled_fitness_tag> {
private:
  using base_t = random<Generator, Elitism, scaled_fitness_tag>;

public:
  inline explicit random_scaled(Generator& generator,
                                countable_t<Elitism> size) noexcept
      : base_t{generator, size} {
  }
};

template<typename FitnessTag>
class worst {
public:
  using fitness_tag_t = FitnessTag;

private:
  inline static constexpr fitness_tag_t fitness_tag{};

public:
  template<typename Population,
           replacement_range<typename Population::iterator_t,
                             typename Population::individual_t> Offspring>
  inline auto operator()(Population& population, Offspring&& offspring) const {
    population.sort(fitness_tag);

    auto removed = population.trim(std::ranges::size(offspring));
    population.insert(details::extract_children(offspring));
    return removed;
  }
};

using worst_raw = worst<raw_fitness_tag>;
using worst_scaled = worst<scaled_fitness_tag>;

template<typename FitnessTag>
class crowd {
public:
  using fitness_tag_t = FitnessTag;

private:
  inline static constexpr fitness_tag_t fitness_tag{};

public:
  template<typename Population,
           replacement_range<typename Population::iterator_t,
                             typename Population::individual_t> Offspring>
  inline auto operator()(Population& population, Offspring&& offspring) const {
    population.insert(details::extract_children(offspring));
    population.sort(fitness_tag);
    return population.trim();
  }
};

using crowd_raw = crowd<raw_fitness_tag>;
using crowd_scaled = crowd<scaled_fitness_tag>;

class parents {
public:
  template<typename Population,
           replacement_range<typename Population::iterator_t,
                             typename Population::individual_t> Offspring>
  inline auto operator()(Population& population, Offspring&& offspring) const {
    return population.replace(offspring);
  }
};

template<typename FitnessTag>
class nondominating_parents {
public:
  using fitness_tag_t = FitnessTag;

private:
  inline static constexpr fitness_tag_t fitness_tag{};

public:
  template<typename Population,
           replacement_range<typename Population::iterator_t,
                             typename Population::individual_t> Offspring>
  inline auto operator()(Population& population, Offspring&& offspring) const {
    return population.replace(
        offspring |
        std::views::filter([cmp = fitness_better{
                                population.comparator(fitness_tag)}](auto& p) {
          return !cmp(get_parent(p)->eval().get(fitness_tag),
                      get_child(p).eval().get(fitness_tag));
        }));
  }
};

using nondominating_parents_raw = nondominating_parents<raw_fitness_tag>;
using nondominating_parents_scaled = nondominating_parents<scaled_fitness_tag>;

class total {
public:
  template<typename Population,
           replacement_range<typename Population::iterator_t,
                             typename Population::individual_t> Offspring>
  inline auto operator()(Population& population, Offspring&& offspring) const {
    auto removed = population.trim_all();
    population.insert(details::extract_children(offspring));
    return removed;
  }
};

class append {
public:
  template<typename Population,
           replacement_range<typename Population::iterator_t,
                             typename Population::individual_t> Offspring>
  inline auto operator()(Population& population, Offspring&& offspring) const {
    population.insert(details::extract_children(offspring));
    return typename Population::collection_t{};
  }
};

} // namespace gal::replace
