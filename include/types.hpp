#ifndef TYPES_HPP
#define TYPES_HPP

#include <type_traits>
#include <vector>

#include "glm/glm.hpp"
#include "libaffa/aa.h"

using i8 = signed char;
using u8 = unsigned char;
using i16 = signed short;
using u16 = unsigned short;
using i32 = signed int;
using u32 = unsigned int;
using i64 = signed long long;
using u64 = unsigned long long;
using f32 = float;
using f64 = double;
using f128 = long double;

// If false, will fall back to Interval Arithmetic
#define USING_AA true

using Type = std::conditional_t<USING_AA, AAF, Interval>;

#endif // TYPES_HPP