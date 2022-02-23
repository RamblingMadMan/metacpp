/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vector>
#include <string_view>
#include <tuple>

namespace ns{
	template<typename T>
	using Vector = std::vector<T>;
}

class [[my::attrib(1, "2", 3.0)]] example{
	public:
		void method1(std::string_view s) const noexcept;
		void method1(const std::string &str) noexcept;

		std::tuple<int, float, char> method2();

		std::string_view member1;

		ns::Vector<std::string> member2;

		static float testVal() noexcept{ return 42.f; }
};

enum class example_enum{
	case_0 = 69,
	case_1 = 420,
	case_2 = 1337
};
