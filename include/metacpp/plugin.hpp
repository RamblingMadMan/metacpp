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

#include "metacpp/config.hpp"

#include <vector>
#include <filesystem>

namespace pluginpp{
	class library{
		public:
			virtual ~library() = default;

			virtual const std::vector<std::string> &symbols() const noexcept = 0;

			virtual std::string demangle(const std::string &symbol_name) const noexcept = 0;

			virtual void *get_symbol(const std::string &name) const noexcept = 0;
	};

	const library *load(const std::filesystem::path &path);
	const library *self();
}

namespace METACPP_PLUGIN_NAMESPACE = pluginpp;

#endif // !METACPP_PLUGIN_HPP
