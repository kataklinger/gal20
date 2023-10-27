
#pragma once

#include "utility.hpp"

#include <functional>
#include <tuple>

namespace gal {

template<typename Event>
struct observer_definition;

template<typename Observer, typename Definitions, typename Event>
concept observer =
    observer_definition<Event>::template satisfies<Observer, Definitions>;

template<typename... Events>
struct observer_tags {};

namespace details {

  template<std::size_t Idx, typename Event, typename List>
  struct event_index_impl;

  template<std::size_t Idx, typename Event>
  struct event_index_impl<Idx, Event, observer_tags<>>;

  template<std::size_t Idx, typename Event, typename Ty, typename... Rest>
  struct event_index_impl<Idx, Event, observer_tags<Ty, Rest...>>
      : event_index_impl<Idx + 1, Event, observer_tags<Rest...>> {};

  template<std::size_t Idx, typename Event, typename... Rest>
  struct event_index_impl<Idx, Event, observer_tags<Event, Rest...>>
      : std::integral_constant<std::size_t, Idx> {};

  template<typename Event, typename List>
  struct event_index : details::event_index_impl<0, Event, List> {};

  template<typename Event, typename List>
  inline constexpr auto event_index_v = event_index<Event, List>::value;

  template<typename Event, typename Observers>
  struct has_observer : std::false_type {};

  template<typename Event, typename... Rest>
  struct has_observer<Event, observer_tags<Event, Rest...>> : std::true_type {};

  template<typename Event, typename This, typename... Rest>
  struct has_observer<Event, observer_tags<This, Rest...>>
      : has_observer<Event, Rest...> {};

  template<typename Event, typename Observers>
  inline constexpr auto has_observer_v = has_observer<Event, Observers>::value;

} // namespace details

template<typename Event, typename Observer>
class observe {
public:
  using event_t = Event;
  using observer_t = Observer;

public:
  inline constexpr observe(event_t /*unused*/, observer_t const& observer)
      : observer_{observer} {
  }

  inline constexpr observe(event_t /*unused*/, observer_t&& observer)
      : observer_{std::move(observer)} {
  }

  inline auto&& observer() && noexcept {
    return std::move(observer_);
  }

  inline auto& observer() & noexcept {
    return observer_;
  }

private:
  observer_t observer_;
};

template<typename Events, typename... Observers>
class observer_pack {
private:
  using events_t = Events;
  using observers_t = std::tuple<Observers...>;

public:
  inline constexpr observer_pack(events_t /*unused*/, Observers&&... observers)
      : observers_{std::move(observers)...} {
  }

  template<typename Event, typename... Args>
  inline void observe(Event /*unused*/, Args&&... args) {
    if constexpr (details::has_observer_v<Event, events_t>) {
      std::invoke(std::get<details::event_index_v<Event, events_t>>(observers_),
                  std::forward<Args>(args)...);
    }
  }

private:
  observers_t observers_;
};

} // namespace gal
