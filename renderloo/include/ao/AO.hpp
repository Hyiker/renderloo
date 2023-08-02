#ifndef RENDERLOO_INCLUDE_AO_AO_HPP
#define RENDERLOO_INCLUDE_AO_AO_HPP
#include "GTAO.hpp"
#include "SSAO.hpp"
enum class AOMethod : int {
    None = 0,
    SSAO = 1,
    GTAO = 2,
    // RTAO = 4,
};

#endif /* RENDERLOO_INCLUDE_AO_AO_HPP */
