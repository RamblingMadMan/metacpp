#include "fmt/format.h"

#include "metacpp/plugin.hpp"

#include "test/example.meta.h"

int main(int argc, char *argv[]){
	auto plugs = plugin::nearby_plugins();

	for(auto &&path : plugs){
		fmt::print("Plugin '{}'\n", path.c_str());

		auto plug = plugin::load(path);

		auto &&types = plug->exported_types();
		for(auto type : types){
			fmt::print("  Exports type '{}'\n", type->name());
		}

		auto &&fns = plug->exported_functions();
		for(auto fn : fns){
			fmt::print("  Exports fn '{}'\n", fn->name());
		}
	}

	return 0;
}
