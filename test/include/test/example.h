#pragma once

#include <string_view>
#include <tuple>

class [[my::attrib(1, "2", 3.0)]] example{
	public:
		void method1(std::string_view s) const noexcept;
		void method1(const std::string &str) noexcept;

		std::tuple<int, float, char> method2();
};

enum class example_enum{
	case_0 = 69,
	case_1 = 420,
	case_2 = 1337
};

inline example_enum example_fn(){
	return example_enum::case_0;
}
