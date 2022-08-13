
#pragma once

#include <concepts>
#include <type_traits>

namespace gal {

template<typename Type>
concept chromosome =
    std::regular<Type> && std::is_nothrow_move_constructible_v<Type> &&
    std::is_nothrow_move_assignable_v<Type>;

} // namespace gal