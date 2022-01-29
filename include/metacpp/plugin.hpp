/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef METACPP_PLUGIN_HPP
#define METACPP_PLUGIN_HPP 1

#include "refl.hpp"

#include <vector>
#include <filesystem>

namespace pluginpp{
	class function_info{
		std::string_view name;
		reflpp::type_info result_type;
		std::vector<reflpp::type_info> param_types;
	};

	class library{
		public:
			virtual ~library() = default;

			virtual const std::vector<std::string> &symbols() const noexcept = 0;

			virtual const std::vector<reflpp::type_info> &exported_types() const noexcept = 0;
			virtual const std::vector<reflpp::function_info> &exported_functions() const noexcept = 0;

			virtual std::string demangle(const std::string &symbol_name) const noexcept = 0;

			virtual void *get_symbol(const std::string &name) const noexcept = 0;
	};

	std::vector<std::filesystem::path> nearby_plugins();

	const library *load(const std::filesystem::path &path);
	const library *self();
}

namespace METACPP_PLUGIN_NAMESPACE = pluginpp;

#endif // !METACPP_PLUGIN_HPP
