#pragma once

// defines a stick curve entry
constexpr float exp_entry(float expCoef, float x, float scale = 1.0f) {
  return scale * pow(x, 2.71828 * expCoef);
}

// get a packed RGB color for the LEDs.
constexpr uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
  }

// Helper functions, used to make compile-time std::array concatenation. Quite ugly, but very simple.
namespace detail
{
	template<class T1,          class T2,
		 size_t S1,         size_t S2,
		 size_t ... SPack1, size_t ... SPack2>
	constexpr std::array<T1, S1 + S2> 
	concatArray(std::array<T1, S1> const arr1, std::array<T2, S2> const arr2,
		    std::integer_sequence<size_t, SPack1...>, std::integer_sequence<size_t, SPack2...>)
	{
		return {{arr1[SPack1]..., arr2[SPack2]...}};										  	
	}
	
	template<class T1,  class T2,
		 size_t S1, size_t S2>
	constexpr std::array<T1, S1 + S2> 
	concatArray(std::array<T1, S1> const arr1, std::array<T2, S2> const arr2)
	{
		static_assert(std::is_same<T1, T2>::value, "Array have different value type");
		return concatArray(arr1, arr2, std::make_index_sequence<S1>{}, std::make_index_sequence<S2>{});
	}
}



