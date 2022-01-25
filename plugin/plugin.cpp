#include "metacpp/plugin.hpp"

#include "fmt/format.h"

#include "boost/core/demangle.hpp"

#define BOOST_DLL_USE_STD_FS 1
#include "boost/dll/library_info.hpp"
#include "boost/dll/shared_library.hpp"
#include "boost/dll/runtime_symbol_info.hpp"

#include <dlfcn.h>

namespace fs = std::filesystem;
namespace dll = boost::dll;

using namespace pluginpp;

namespace {
	static std::function<void(const std::string&)> print_fn = [](const std::string &msg){
		fmt::print(stderr, "[ERROR] {}\n", msg);
	};

	template<typename Str, typename ... Args>
	void print_error(Str &&fmt_str, Args &&... args){
		auto msg = fmt::vformat(std::forward<Str>(fmt_str), fmt::make_format_args(std::forward<Args>(args)...));
		print_fn(msg);
	}

	class self_t{};

	class dynamic_library: public library{
		public:
			explicit dynamic_library(self_t)
				: m_handle(dlopen(nullptr, RTLD_LAZY))
			{
				if(!m_handle){
					auto msg = fmt::format("Error in dlopen: {}", dlerror());
					print_error("{}", msg);
					throw std::runtime_error(msg);
				}

				auto info = dll::library_info(dll::program_location());

				m_symbols = info.symbols();

				import_types();
			}

			explicit dynamic_library(const fs::path &path)
				: m_handle(dlopen(path.c_str(), RTLD_LAZY))
			{
				if(!m_handle){
					auto msg = fmt::format("Error in dlopen: {}", dlerror());
					print_error("{}", msg);
					throw std::runtime_error(msg);
				}

				auto info = dll::library_info(path);

				m_symbols = info.symbols();

				import_types();
			}

			dynamic_library(const dynamic_library&) = delete;

			dynamic_library(dynamic_library &&other) noexcept
				: m_handle(std::exchange(other.m_handle, nullptr))
				, m_symbols(std::move(other.m_symbols))
				, m_types(std::move(other.m_types))
			{}

			~dynamic_library(){
				reset();
			}

			dynamic_library &operator=(const dynamic_library&) = delete;

			dynamic_library &operator=(dynamic_library &&other) noexcept{
				if(this != &other){
					m_symbols = std::move(other.m_symbols);
					m_types = std::move(other.m_types);
					reset(std::exchange(other.m_handle, nullptr));
				}

				return *this;
			}

			bool is_valid() const noexcept{ return !!m_handle; }

			std::string demangle(const std::string &symbol_name) const noexcept override{
				return boost::core::demangle(symbol_name.c_str());
			}

			const std::vector<std::string> &symbols() const noexcept override{
				return m_symbols;
			}

			const std::vector<reflpp::type_info> &exported_types() const noexcept override{
				return m_types;
			}

			void *get_symbol(const std::string &name) const noexcept override{
				if(!is_valid()) return nullptr;

				auto sym = dlsym(m_handle, name.c_str());
				if(!sym){
					print_error("Error in dlsym: {}", dlerror());
				}

				return sym;
			}

		private:
			void import_types(){
				for(auto &&sym : m_symbols){
					auto readable = demangle(sym);

					constexpr std::string_view exportFnName = "reflpp::detail::fn_export";
					const auto exported_fn_res = readable.find(exportFnName);
					if(exported_fn_res != std::string::npos){
						// TODO: import function
						continue;
					}

					constexpr std::string_view exportTypeName = "reflpp::detail::type_export";
					const auto exported_type_res = readable.find(exportTypeName);
					if(exported_type_res != std::string::npos){
						auto ptr = get_symbol(sym);
						auto f = reinterpret_cast<refl::type_info(*)()>(ptr);
						assert(f);
						m_types.emplace_back(f());
						continue;
					}
				}
			}

			void reset(void *handle = nullptr){
				auto old_handle = std::exchange(m_handle, handle);

				if(old_handle && (dlclose(old_handle) != 0)){
					print_error("Error in dlclose: {}", dlerror());
				}
			}

			void *m_handle;
			std::vector<std::string> m_symbols;
			std::vector<refl::type_info> m_types;
	};

	class plugin_loader{
		public:
			plugin_loader(){
				const auto program_path = dll::program_location();
				const auto abs_path = fs::absolute(program_path);
				m_libraries.try_emplace(abs_path, self_t{});
			}

			const dynamic_library *load(const fs::path &path){
				if(!fs::exists(path)){
					print_error("Plugin path '{}' does not exist", path.c_str());
					return nullptr;
				}
				else if(!fs::is_regular_file(path)){
					print_error("Plugin path '{}' is not a file", path.c_str());
					return nullptr;
				}

				auto abs_path = fs::absolute(path);

				auto res = m_libraries.find(abs_path);
				if(res != m_libraries.end()){
					return &res->second;
				}

				auto emplace_res = m_libraries.try_emplace(abs_path, abs_path);
				if(!emplace_res.second){
					print_error("Internal error in std::unordered_map::try_emplace");
					return nullptr;
				}

				return &emplace_res.first->second;
			}

		private:
			std::unordered_map<std::string, dynamic_library> m_libraries;
	};
}

const library *plugin::load(const fs::path &path){
	static plugin_loader loader;
	return loader.load(path);
}

const library *plugin::self(){
	return plugin::load(dll::program_location());
}

std::vector<std::filesystem::path> plugin::nearby_plugins(){
	namespace fs = std::filesystem;

	auto exe_dir = dll::program_location().parent_path();

	std::vector<std::filesystem::path> ret;

	for(
		auto &&entry :
		fs::directory_iterator(exe_dir, fs::directory_options::skip_permission_denied)
	){
		if(!entry.is_regular_file()) continue;

		auto path = entry.path();

		if(path.extension() == dll::shared_library::suffix()){
			ret.emplace_back(std::move(path));
		}
	}

	return ret;
}
