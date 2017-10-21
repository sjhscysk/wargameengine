#pragma once

namespace wargameEngine
{
inline namespace math
{
template<class T>
struct vec3T
{
	T x;
	T y;
	T z;
	operator T*() { return &x; }
	operator const T*() { return &x; }
};

using vec3 = vec3T<float>;
using ivec3 = vec3T<int>;
using uivec3 = vec3T<unsigned>;
using dvec3 = vec3T<double>;
}
}