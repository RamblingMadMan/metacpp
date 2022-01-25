/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef METACPP_SERIAL_HPP
#define METACPP_SERIAL_HPP 1

#include "meta.hpp"

#include <sstream>
#include <string>

namespace serialpp{
	namespace detail{
		template<typename T, typename = void>
		struct to_json_helper;

		template<typename Class>
		struct to_json_helper<Class, std::enable_if_t<std::is_class_v<Class>>>{
			static std::string invoke(const Class &cls){
				std::ostringstream output;
				output << std::noshowpoint;

				output << "{\"" << metapp::type_name<Class> << "\":[";

				metapp::for_all_i<metapp::members<Class>>(
					[&]<class Member, std::size_t Idx>{
						constexpr std::string_view name = Member::name;
						constexpr std::string_view type_name = meta::type_name<typename Member::type>;
						const auto &val = Member::get(cls);
						if constexpr(Idx != 0){
							output << ",";
						}
						output << "{\"member\":{";
						output << "\"name\":\"";
						output << name;
						output << "\",\"type\":\"";
						output << type_name;
						output << "\",\"value\":\"";
						output << val;
						output << "\"}}";
					}
				);

				output << "]}";

				return output.str();
			}
		};
	}

	template<typename T>
	std::string to_json(const T &val){
		return detail::to_json_helper<T>::invoke(val);
	}
}

namespace METACPP_SERIAL_NAMESPACE = serialpp;

#endif // !METACPP_SERIAL_HPP
