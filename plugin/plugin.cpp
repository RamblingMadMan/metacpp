#include "metacpp/plugin.hpp"

#include "fmt/format.h"

#include "boost/core/demangle.hpp"

#define BOOST_DLL_USE_STD_FS 1
#include "boost/dll/library_info.hpp"
#include "boost/dll/shared_library.hpp"
#include "boost/dll/runtime_symbol_info.hpp"

#ifdef __linux__
#include <dlfcn.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#error "Unsupported operating system"
#endif

namespace pluginpp::detail{
#ifdef __linux__
	using lib_handle = void*;
#else
	using lib_handle = HMODULE;
#endif

	static lib_handle load_library(const char *path){
#ifdef __linux__
		return dlopen(path, RTLD_LAZY);
#else
		return path ? LoadLibraryA(path) : GetModuleHandleA(nullptr);
#endif
	}

	static bool close_library(lib_handle handle){
#ifdef __linux__
		return dlclose(handle) == 0;
#else
		return (handle == GetModuleHandleA(nullptr)) ? true : FreeLibrary(handle);
#endif
	}

	static void *get_symbol(lib_handle lib, const char *name){
#ifdef __linux__
		return dlsym(lib, name);
#else
		return reinterpret_cast<void*>(GetProcAddress(lib, name));
#endif
	}

	static const char *get_error(){
#ifdef __linux__
		return dlerror();
#else
		thread_local char buf[256];
		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			buf, sizeof(buf),
			nullptr
		);

		return buf;
#endif
	}
}

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
				: m_handle(detail::load_library(nullptr))
			{
				if(!m_handle){
					auto msg = fmt::format("Error in load_library: {}", detail::get_error());
					throw std::runtime_error(msg);
				}

				auto info = dll::library_info(dll::program_location());

				m_symbols = info.symbols();

				import_entities();
			}

			explicit dynamic_library(const fs::path &path){
				auto path_utf8 = path.u8string();

				m_handle = detail::load_library(path.u8string().c_str());
				if(!m_handle){
					auto msg = fmt::format("Error in load_library: {}", detail::get_error());
					throw std::runtime_error(msg);
				}

				auto info = dll::library_info(path);

				m_symbols = info.symbols();

				import_entities();
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

			const std::vector<reflpp::function_info> &exported_functions() const noexcept override{
				return m_fns;
			}

			void *get_symbol(const std::string &name) const noexcept override{
				if(!is_valid()) return nullptr;

				auto sym = detail::get_symbol(m_handle, name.c_str());
				if(!sym){
					print_error("Error in get_symbol: {}", detail::get_error());
				}

				return sym;
			}

		private:
			void import_entities(){
				for(auto &&sym : m_symbols){
					auto readable = demangle(sym);

					constexpr std::string_view exportFnName = "reflpp::detail::function_export";
					const auto exported_fn_res = readable.find(exportFnName);
					if(exported_fn_res != std::string::npos){
						auto ptr = get_symbol(sym);
						auto f = reinterpret_cast<refl::detail::function_export_fn>(ptr);
						assert(f);
						auto fn = m_fns.emplace_back(f());
						continue;
					}

					constexpr std::string_view exportTypeName = "reflpp::detail::type_export";
					const auto exported_type_res = readable.find(exportTypeName);
					if(exported_type_res != std::string::npos){
						auto ptr = get_symbol(sym);
						auto f = reinterpret_cast<refl::detail::type_export_fn>(ptr);
						assert(f);
						auto type = m_types.emplace_back(f());
						refl::detail::register_type(type);
						continue;
					}
				}
			}

			void reset(detail::lib_handle handle = nullptr){
				auto old_handle = std::exchange(m_handle, handle);

				if(old_handle && !detail::close_library(old_handle)){
					print_error("Error in close_library: {}", detail::get_error());
				}
			}

			detail::lib_handle m_handle;
			std::vector<std::string> m_symbols;
			std::vector<refl::type_info> m_types;
			std::vector<refl::function_info> m_fns;
	};

	class plugin_loader{
		public:
			plugin_loader(): m_self(self_t{}){}

			const dynamic_library *load(const fs::path &path){
				if(!fs::exists(path)){
					print_error("Plugin path '{}' does not exist", path.u8string());
					return nullptr;
				}
				else if(!fs::is_regular_file(path)){
					print_error("Plugin path '{}' is not a file", path.u8string());
					return nullptr;
				}

				auto abs_path = fs::absolute(path);
				auto abs_path_utf8 = abs_path.u8string();

				auto res = m_libraries.find(abs_path_utf8);
				if(res != m_libraries.end()){
					return &res->second;
				}

				auto emplace_res = m_libraries.try_emplace(abs_path_utf8, abs_path);
				if(!emplace_res.second){
					print_error("Internal error in std::unordered_map::try_emplace");
					return nullptr;
				}

				return &emplace_res.first->second;
			}

			const dynamic_library *self() const noexcept{
				return &m_self;
			}

		private:
			dynamic_library m_self;
			std::unordered_map<std::string, dynamic_library> m_libraries;
	};

	static plugin_loader loader;
}

const library *plugin::load(const fs::path &path){
	return loader.load(path);
}

const library *plugin::self(){
	return loader.self();
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
