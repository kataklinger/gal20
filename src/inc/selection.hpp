
#pragma once

#include "sampling.hpp"

namespace gal {
namespace select {

  template<typename Attribute>
  concept attribute = requires {
    { Attribute::size } -> util::decays_to<std::size_t>;
    { Attribute::unique } -> util::decays_to<bool>;
    { Attribute::sample() } -> std::same_as<sample_t<Attribute::unique>>;
  };

  template<bool Unique, std::size_t Size>
  struct selection_attribute {
    inline static constexpr auto size = Size;
    inline static constexpr auto unique = Unique;

    inline static auto sample() {
      return sample_t<unique>{size};
    }
  };

  template<std::size_t Size>
  inline constexpr selection_attribute<false, Size> nonunique{};

  template<std::size_t Size>
  inline constexpr selection_attribute<true, Size> unique{};

  template<attribute Attribute, typename Generator>
  class random {
  public:
    using generator_t = Generator;
    using attribute_t = Attribute;

  private:
    using distribution_t = std::uniform_int_distribution<std::size_t>;

  public:
    inline random(attribute_t /*unused*/, generator_t& generator) noexcept
        : generator_{&generator} {
    }

    template<typename Population>
    inline auto operator()(Population& population) const {
      return sample_many(
          population, attribute_t::sample(), [&population, this]() {
            return distribution_t{0,
                                  population.current_size() - 1}(*generator_);
          });
    }

  private:
    generator_t* generator_;
  };

  template<std::size_t Size, typename FitnessTag>
  class best {
  private:
    using fitness_tag_t = FitnessTag;

  public:
    template<ordered_population<fitness_tag_t> Population>
    inline auto operator()(Population& population) const {
      population.sort(fitness_tag_t{});

      std::size_t idx{};
      return sample_many(
          population, nonunique_sample{Size}, [&idx]() { return idx++; });
    }
  };

  template<std::size_t Size>
  using best_raw = best<Size, raw_fitness_tag>;

  template<std::size_t Size>
  using best_scaled = best<Size, scaled_fitness_tag>;

  namespace details {

    template<typename Range>
    using pgenerator_input_t = std::add_rvalue_reference_t<
        std::add_const_t<std::ranges::range_value_t<Range>>>;

    template<typename Generator, typename Range>
    using pgenerator_output_t =
        std::invoke_result_t<Generator, pgenerator_input_t<Range>>;

    template<typename Generator, typename Range>
    concept pgenerator = requires(Generator g) {
      requires std::invocable<Generator, pgenerator_input_t<Range>>;

      typename fitness_traits<
          pgenerator_output_t<Generator, Range>>::totalizator_t;
    };

    template<std::ranges::range Range, pgenerator<Range> Generator>
    auto generate_wheel(Range const& range, Generator&& generate) {
      using item_t = pgenerator_output_t<Generator, Range>;
      using total_t = typename fitness_traits<
          pgenerator_output_t<Generator, Range>>::totalizator_t;

      std::vector<item_t> wheel;
      for (total_t total; auto&& item : range) {
        total = total.add(std::invoke(generate, item));
        wheel.push_back(total.sum());
      }

      return wheel;
    }

    template<typename Dist,
             std::ranges::range Range,
             typename Generator,
             typename Proj = std::identity>
    inline auto
        roll_wheel(Range const& wheel, Generator& generator, Proj proj = {}) {
      auto selected = Dist{{}, *std::ranges::rend(wheel)}(generator);
      return std::ranges::lower_bound(
                 wheel, selected, std::ranges::less{}, proj) -
             wheel.begin();
    }

  } // namespace details

  template<typename FitnessTag, attribute Attribute, typename Generator>
  class roulette {
  public:
    using generator_t = Generator;
    using attribute_t = Attribute;

  private:
    using fitness_tag_t = FitnessTag;

  private:
    inline static constexpr fitness_tag_t fitness_tag{};

  public:
    inline roulette(attribute_t /*unused*/, generator_t& generator) noexcept
        : generator_{&generator} {
    }

    template<typename Population>
    inline auto operator()(Population& population) const
      requires ordered_population<Population, FitnessTag> &&
               averageable_population<Population, FitnessTag>
    {
      using fitness_t = get_fitness_t<fitness_tag_t, Population>;
      using distribution_t =
          typename fitness_traits<fitness_t>::random_distribution_t;

      population.sort(fitness_tag);

      auto wheel =
          details::generate_wheel(population.individuals(), [](auto& ind) {
            return ind.evaluation().get(fitness_tag);
          });

      return sample_many(population, attribute_t::sample(), [&wheel, this]() {
        return details::roll_wheel<distribution_t>(wheel, *generator_);
      });
    }

  private:
    generator_t* generator_;
  };

  template<attribute Attribute, typename Generator>
  class roulette_raw : public roulette<raw_fitness_tag, Attribute, Generator> {
    using roulette<raw_fitness_tag, Attribute, Generator>::roulette;
  };

  template<attribute Attribute, typename Generator>
  roulette_raw(Attribute, Generator) -> roulette_raw<Attribute, Generator>;

  template<attribute Attribute, typename Generator>
  class roulette_scaled
      : public roulette<scaled_fitness_tag, Attribute, Generator> {
  public:
    using roulette<scaled_fitness_tag, Attribute, Generator>::roulette;
  };

  template<attribute Attribute, typename Generator>
  roulette_scaled(Attribute, Generator)
      -> roulette_scaled<Attribute, Generator>;

  template<typename Options>
  concept clustering = requires {
    typename Options::cluster_tag_t;
    requires std::same_as<decltype(Options::sharing), bool const>;
  };

  template<typename ClusterTag, bool Sharing>
  struct selection_clustering {
    using cluster_tag_t = ClusterTag;

    inline static constexpr auto sharing = Sharing;
  };

  template<typename ClusterTag>
  inline constexpr selection_clustering<ClusterTag, false> uniform{};

  template<typename ClusterTag>
  inline constexpr selection_clustering<ClusterTag, true> shared{};

  template<clustering Clustering,
           attribute Attribute,
           typename Generator,
           typename ClusterIndex =
               cluster_index<typename Clustering::cluster_tag_t>>
  class cluster {
  private:
    using clustering_t = Clustering;

  public:
    using cluster_tag_t = typename Clustering::cluster_tag_t;
    using attribute_t = Attribute;
    using generator_t = Generator;
    using cluster_index_t = ClusterIndex;

    inline static constexpr auto sharing = Clustering::sharing;

  private:
    using wheel_t = std::conditional_t<sharing, double, std::size_t>;
    using cluster_dist_t =
        std::conditional_t<sharing,
                           std::uniform_real_distribution<wheel_t>,
                           std::uniform_int_distribution<wheel_t>>;
    using item_dist_t = std::uniform_int_distribution<std::size_t>;

  public:
    inline cluster(clustering_t /*unused*/,
                   attribute_t /*unused*/,
                   generator_t& generator,
                   cluster_index_t index) noexcept
        : generator_{&generator}
        , index_{index} {
    }

    inline cluster(clustering_t clust,
                   attribute_t attrib,
                   generator_t& generator) noexcept
        : cluster{clust, attrib, generator, cluster_index_t{}} {
    }

    template<population_tagged_with<cluster_tag_t> Population>
    auto operator()(Population& population) const {
      std::vector<std::tuple<std::size_t, std::size_t>> clusters;
      for (auto&& individual : population.individuals()) {
        auto index = index_(get_tag<cluster_tag_t>(individual));
        if (index >= clusters.size()) {
          clusters.resize(index);
        }

        ++std::get<0>(clusters[index]);
      }

      for (std::size_t i = 0; auto&& [pos, fill] : clusters) {
        i += std::exchange(pos, i);
        fill = pos;
      }

      std::vector<std::size_t> buffer(population.current_size());
      for (std::size_t i = 0; auto&& individual : population.individuals()) {
        auto index = index_(get_tag<cluster_tag_t>(individual));
        auto& fill = std::get<1>(clusters[index]);
        buffer[fill++] = i++;
      }

      for (auto&& [pos, count] : clusters) {
        count -= pos;
      }

      std::ranges::sort(clusters, std::ranges::greater{}, [](auto const& c) {
        return std::get<1>(c);
      });

      auto wheel = details::generate_wheel(clusters, [](auto const& c) {
        return get_probability(std::get<1>(c));
      });

      return sample_many(
          population,
          attribute_t::sample(),
          [&wheel, &clusters, this]() {
            auto c = details::roll_wheel<cluster_dist_t>(wheel, *generator_);
            return std::tuple{c, std::get<1>(clusters[c])};
          },
          [&clusters, &buffer, this](std::size_t c) {
            auto& [pos, count] = clusters[c];
            return buffer[item_dist_t{pos, pos + count - 1}(*generator_)];
          });
    }

  private:
    inline static wheel_t get_probability(std::size_t cluster_size) noexcept {
      if constexpr (sharing) {
        return 1. / cluster_size;
      }
      else {
        return cluster_size;
      }
    }

  private:
    generator_t* generator_;
    cluster_index_t index_;
  };

  template<typename FitnessTag>
  class ancestry {
  private:
    using fitness_tag_t = FitnessTag;

    inline static constexpr fitness_tag_t fitness_tag{};

  public:
    template<population_tagged_with<ancestry_t> Population>
    auto operator()(Population& population) const {
      std::vector<typename Population::iterator_t> result;

      for (auto it = population.individuals().begin();
           it != population.individuals().end();
           ++it) {

        if (auto& ancestry = get_tag<ancestry_t>(*it);
            static_cast<std::uint8_t>(ancestry.get()) < 2) {
          result.push_back(it);
          ancestry = ancestry_status::none;
        }
      }

      if (result.size() > 1) {
        std::ranges::sort(
            result, population.comparator(fitness_tag), [](auto const& ind) {
              return ind->evaluation().get(fitness_tag);
            });

        result.erase(result.begin() + 1, result.end());
      }

      return result;
    }
  };

  class ancestry_raw : public ancestry<raw_fitness_tag> {
    using ancestry<raw_fitness_tag>::ancestry;
  };

  class ancestry_scaled : public ancestry<scaled_fitness_tag> {
    using ancestry<scaled_fitness_tag>::ancestry;
  };

} // namespace select
} // namespace gal
