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

/**
 * @defgroup Plugin Plugin utilities
 * @{
 */

#include "refl.hpp"

#include <vector>
#include <filesystem>

namespace pluginpp{
	struct function_info{
		std::string_view name;
		reflpp::type_info result_type;
		std::vector<reflpp::type_info> param_types;
	};

	/**
	 * @brief Information about an executable/plugin.
	 */
	class library{
		public:
			virtual ~library() = default;

			virtual const std::vector<std::string> &symbols() const noexcept = 0;

			virtual const std::vector<reflpp::type_info> &exported_types() const noexcept = 0;
			virtual const std::vector<reflpp::function_info> &exported_functions() const noexcept = 0;

			virtual std::string demangle(const std::string &symbol_name) const noexcept = 0;

			virtual void *get_symbol(const std::string &name) const noexcept = 0;
	};

	/**
	 * @brief Get a list of plugins placed in the same folder as the executable.
	 */
	std::vector<std::filesystem::path> nearby_plugins();

	/**
	 * @brief Try to load a plugin.
	 * @returns `nullptr` on error, handle to the loaded plugin on success
	 */
	const library *load(const std::filesystem::path &path);

	/**
	 * @brief Get a reference to the running executable.
	 */
	const library *self();
}

#ifndef METACPP_NO_NAMESPACE_ALIAS
namespace METACPP_PLUGIN_NAMESPACE = pluginpp;
#endif

/**
 * @}
 */

#endif // !METACPP_PLUGIN_HPP
