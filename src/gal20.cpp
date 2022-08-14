// gal20.cpp : Defines the entry point for the application.
//

#include "gal20.h"

#include "selection.hpp"
#include "replacement.hpp"
#include "basic.hpp"

#include <tuple>

using pop_t = gal::population<int, int, int, int>;

struct parent_replacement_t
    : std::tuple<pop_t::iterator_t, pop_t::individual_t> {
  using tuple::tuple;
};

pop_t::iterator_t& get_parent(parent_replacement_t& x) {
  return get<0>(x);
}

pop_t::individual_t& get_child(parent_replacement_t& x) {
  return get<1>(x);
}

gal::coupling_metadata get_metadata(parent_replacement_t x) {
  return {};
}

template<typename Population,
         gal::selection_range<typename Population::iterator_t> Range>
void test_selection_range(Range&& r) {
}

template<typename Population, gal::selection<Population> Selection>
void test_selection(Selection&& s) {
}

template<
    typename Population,
    gal::coupling<Population, std::vector<pop_t::iterator_t>> Coupling>
void test_coupling(Coupling&& s) {
}

template<
    typename Population,
    gal::replacement<Population, std::vector<parent_replacement_t>> Replacement>
void test_replacement(Replacement&& r) {
}

std::vector<pop_t::iterator_t> s(pop_t const& p) {
  return {};
}

std::vector<parent_replacement_t> c(std::vector<pop_t::iterator_t>&& p) {
  return {};
}

std::vector<pop_t::individual_t> r(pop_t& p,
                                   std::vector<parent_replacement_t>&& v) {
  return {};
}

int main() {

  std::vector<parent_replacement_t> x{};

  pop_t p{true};
  p.replace(std::move(x));

  test_selection_range<pop_t>(std::vector<pop_t::iterator_t>{});

  test_selection<pop_t>(s);
  test_coupling<pop_t>(c);
  test_replacement<pop_t>(r);

  std::mt19937 gen{};

  gal::replace::random<10, std::mt19937> a{gen};
  a(p, std::vector<parent_replacement_t>{});

  return 0;
}
