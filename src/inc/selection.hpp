
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

  template<std::size_t Count>
  struct selection_rounds : std::integral_constant<std::size_t, Count> {};

  template<std::size_t Count>
  inline constexpr selection_rounds<Count> rounds{};

  namespace details {

    template<typename Ty>
    struct is_rounds : std::false_type {};

    template<std::size_t Count>
    struct is_rounds<selection_rounds<Count>> : std::true_type {};

    template<typename Ty>
    inline constexpr auto is_rounds_v = is_rounds<Ty>::value;

  } // namespace details

  template<typename Ty>
  concept rounds_attribute = details::is_rounds_v<Ty>;

  template<typename FitnessTag,
           attribute Attribute,
           rounds_attribute Rounds,
           typename Generator>
  class tournament {
  public:
    using attribute_t = Attribute;
    using fitness_tag_t = FitnessTag;
    using rounds_t = Rounds;
    using generator_t = Generator;

  private:
    using distribution_t = std::uniform_int_distribution<std::size_t>;

    inline static constexpr fitness_tag_t fitness_tag{};

  public:
    inline tournament(fitness_tag_t /*unused*/,
                      attribute_t /*unused*/,
                      rounds_t /*unused*/,
                      generator_t& generator) noexcept
        : generator_{&generator} {
    }

    inline explicit tournament(generator_t& generator) noexcept
        : generator_{&generator} {
    }

    template<sortable_population<fitness_tag_t> Population>
    inline auto operator()(Population& population) const {
      fitness_better cmp{population.comparator(fitness_tag)};

      auto generate = [&population, this]() {
        auto idx =
            distribution_t{0, population.current_size() - 1}(*generator_);
        return std::tuple{
            idx, &population.individuals()[idx].evaluation().get(fitness_tag)};
      };

      return sample_many(population,
                         attribute_t::sample(),
                         [&population, cmp, generate, this]() {
                           auto [best_idx, best_fitness] = generate();

                           for (auto i = rounds_t::value - 1; i > 0; --i) {
                             if (auto [idx, current] = generate();
                                 cmp(*current, *best_fitness)) {
                               best_idx = idx;
                               best_fitness = current;
                             }
                           }

                           return best_idx;
                         });
    }

  private:
    generator_t* generator_;
  };

  template<attribute Attribute, rounds_attribute Rounds, typename Generator>
  class tournament_raw
      : public tournament<raw_fitness_tag, Attribute, Rounds, Generator> {
  private:
    using base_t = tournament<raw_fitness_tag, Attribute, Rounds, Generator>;

  public:
    using attribute_t = Attribute;
    using rounds_t = Rounds;
    using generator_t = Generator;

  public:
    inline tournament_raw(attribute_t /*unused*/,
                          rounds_t /*unused*/,
                          generator_t& generator) noexcept
        : base_t{generator} {
    }
  };

  template<attribute Attribute, rounds_attribute Rounds, typename Generator>
  class tournament_scaled
      : public tournament<scaled_fitness_tag, Attribute, Rounds, Generator> {
  private:
    using base_t = tournament<scaled_fitness_tag, Attribute, Rounds, Generator>;

  public:
    using attribute_t = Attribute;
    using rounds_t = Rounds;
    using generator_t = Generator;

  public:
    inline tournament_scaled(attribute_t /*unused*/,
                             rounds_t /*unused*/,
                             generator_t& generator) noexcept
        : base_t{generator} {
    }
  };

  template<std::size_t Size, typename FitnessTag>
  class best {
  private:
    using fitness_tag_t = FitnessTag;

  public:
    template<sortable_population<fitness_tag_t> Population>
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
    concept pgenerator = requires {
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
      auto selected = Dist{{}, *std::ranges::rbegin(wheel)}(generator);
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
      requires sortable_population<Population, FitnessTag> &&
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

  template<typename ClusterTag>
  inline constexpr std::false_type uniform{};

  template<typename ClusterTag>
  inline constexpr std::true_type shared{};

  namespace details {
    class clustering {
    public:
      template<typename Iter>
      clustering(Iter first, Iter valid, Iter last) {
        auto proper = std::ranges::find_if(valid, last, [](auto& i) {
          return !get_tag<cluster_label>(i).is_unique();
        });

        if (proper != valid) {
          clusters_.emplace_back(
              static_cast<std::size_t>(proper - valid), 0, true);
        }

        adjustement_ = clusters_.size();

        for (auto it = proper; it != last; ++it) {
          auto index = get_tag<cluster_label>(*it).index() + adjustement_;
          if (index >= clusters_.size()) {
            clusters_.resize(index + 1);
          }

          ++std::get<0>(clusters_[index]);
        }

        for (std::size_t i = 0; auto&& [pos, fill, u] : clusters_) {
          i += std::exchange(pos, i);
          fill = pos;
        }
      }

      template<typename Iter>
      auto prepare_buffer(Iter first, Iter valid, Iter last) {
        std::vector<std::size_t> buffer(last - first);

        auto i = static_cast<std::size_t>(valid - first);
        for (auto it = valid; it != last; ++it) {
          auto label = get_tag<cluster_label>(*it);
          auto index = label.is_unique() ? 0 : label.index() + adjustement_;
          auto& fill = std::get<1>(clusters_[index]);
          buffer[fill++] = i++;
        }

        return buffer;
      }

      inline auto extract_clusters() noexcept {
        for (auto&& [pos, count, u] : clusters_) {
          count -= pos;
        }

        std::ranges::sort(clusters_.begin() + adjustement_,
                          clusters_.end(),
                          std::ranges::less{},
                          [](auto const& c) { return std::get<1>(c); });

        return std::move(clusters_);
      }

    private:
      std::vector<std::tuple<std::size_t, std::size_t, bool>> clusters_;
      std::size_t adjustement_;
    };
  } // namespace details

  template<typename Clustering, attribute Attribute, typename Generator>
  class cluster {
  private:
    using clustering_t = Clustering;

  public:
    using attribute_t = Attribute;
    using generator_t = Generator;

    inline static constexpr auto sharing = Clustering::value;

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
                   generator_t& generator) noexcept
        : generator_{&generator} {
    }

    template<population_tagged_with<cluster_label> Population>
    auto operator()(Population& population) const {
      population.sort([](auto const& lhs, auto const& rhs) {
        return get_tag<cluster_label>(lhs) < get_tag<cluster_label>(rhs);
      });

      auto first = std::ranges::begin(population.individuals());
      auto last = std::ranges::end(population.individuals());
      auto valid = std::ranges::find_if(first, last, [](auto& i) {
        return !get_tag<cluster_label>(i).is_unassigned();
      });

      details::clustering staging{first, valid, last};
      auto buffer = staging.prepare_buffer(first, valid, last);
      auto clusters = staging.extract_clusters();

      auto wheel = details::generate_wheel(clusters, [](auto const& c) {
        auto const& [p, count, packed] = c;
        return get_probability(count, packed);
      });

      return sample_many(
          population,
          attribute_t::sample(),
          [&wheel, &clusters, this]() {
            auto c = details::roll_wheel<cluster_dist_t>(wheel, *generator_);
            return std::tuple{c, std::get<1>(clusters[c])};
          },
          [&clusters, &buffer, this](std::size_t c) {
            auto& [pos, count, u] = clusters[c];
            return buffer[item_dist_t{pos, pos + count - 1}(*generator_)];
          });
    }

  private:
    inline static wheel_t get_probability(std::size_t cluster_size,
                                          bool packed) noexcept {
      if (cluster_size == 0) {
        return 0;
      }

      if constexpr (sharing) {
        return packed ? static_cast<wheel_t>(cluster_size) : 1. / cluster_size;
      }
      else {
        return packed ? cluster_size : 1;
      }
    }

  private:
    generator_t* generator_;
  };

  template<typename FitnessTag>
  class local {
  private:
    using fitness_tag_t = FitnessTag;

    inline static constexpr fitness_tag_t fitness_tag{};

  public:
    template<population_tagged_with<ancestry_t> Population>
      requires(sortable_population<Population, FitnessTag>)
    auto operator()(Population& population) const {
      std::vector<typename Population::iterator_t> result;

      for (auto it = std::ranges::begin(population.individuals());
           it != std::ranges::end(population.individuals());
           ++it) {

        if (auto& ancestry = get_tag<ancestry_t>(*it);
            static_cast<std::uint8_t>(ancestry.get()) < 2) {
          result.push_back(it);
          ancestry = ancestry_status::none;
        }
      }

      if (result.size() > 1) {
        std::ranges::sort(
            result,
            gal::fitness_better{population.comparator(fitness_tag)},
            [](auto const& ind) { return ind->evaluation().get(fitness_tag); });

        result.erase(result.begin() + 1, result.end());
      }

      return result;
    }
  };

  class local_raw : public local<raw_fitness_tag> {};
  class local_scaled : public local<scaled_fitness_tag> {};

} // namespace select
} // namespace gal
