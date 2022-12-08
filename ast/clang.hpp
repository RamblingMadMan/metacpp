/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef METACPP_AST_CLANG_HPP
#define METACPP_AST_CLANG_HPP 1

#include "metacpp/ast.hpp"

#include "clang-c/Index.h"
#include "clang-c/CXCompilationDatabase.h"

#include "fmt/format.h"

#include <iterator>
#include <functional>
#include <optional>
#include <utility>

namespace astpp::clang{
	namespace fs = std::filesystem;

	template<typename T, auto DestroyFn>
	class handle{
		public:
			handle(const handle&) = delete;

			handle(handle &&other)
				: m_handle(other.m_handle)
			{
				other.m_handle = nullptr;
			}

			virtual ~handle(){
				if(m_handle) DestroyFn(m_handle);
			}

			handle &operator=(const handle&) = delete;

			handle &operator=(handle &&other) noexcept{
				if(this != &other){
					auto old_handle = std::exchange(m_handle, other.m_handle);

					other.m_handle = nullptr;

					if(old_handle){
						DestroyFn(old_handle);
					}
				}

				return *this;
			}

			operator T() const noexcept{ return m_handle; }
			operator bool() const noexcept{ return m_handle != nullptr; }

		protected:
			handle(T handle_) noexcept
			: m_handle(handle_){}

			void set_handle(T handle_){
				auto old_handle = std::exchange(m_handle, handle_);
				if(old_handle) DestroyFn(old_handle);
			}

		private:
			T m_handle;
	};

	class index: public handle<CXIndex, clang_disposeIndex>{
		public:
			index(bool excludeDeclarationsFromPCH, bool displayDiagnostics)
			: handle(clang_createIndex(excludeDeclarationsFromPCH, displayDiagnostics))
			{}

			index()
#ifndef NDEBUG
				: index(false, true)
#else
				: index(false, false)
#endif
			{}
	};

	namespace detail{
		template<typename T>
		void null_destroy(T){}

		inline std::string convert_str(CXString str){
			std::string ret = clang_getCString(str);
			clang_disposeString(str);
			return ret;
		}
	}

	class token{
		public:
			token(CXTranslationUnit tu, CXToken tok) noexcept
				: m_tu(tu), m_tok(tok){}

			token() = default;

			token(const token&) = default;

			operator CXToken() const noexcept{ return m_tok; }

			bool is_valid() const noexcept{ return m_tu; }

			CXTokenKind kind() const noexcept{ return clang_getTokenKind(m_tok); }

			std::string str() const noexcept{
				return is_valid() ? detail::convert_str(clang_getTokenSpelling(m_tu, m_tok)) : "";
			}

		private:
			CXTranslationUnit m_tu = nullptr;
			CXToken m_tok;

			friend class token_iterator;
	};

	class token_iterator{
		public:
			token_iterator() = default;

			token_iterator(const token_iterator&) = default;

			token_iterator &operator=(const token_iterator&) = default;

			bool is_valid() const noexcept{ return m_it; }

			const token &operator*() const noexcept{
				return m_val;
			}

			const token *operator->() const noexcept{
				return &m_val;
			}

			token_iterator &operator++() noexcept{
				if(m_it == m_end) return *this;
				++m_it;
				m_val = token(m_tu, *m_it);
				return *this;
			}

			token_iterator &operator--() noexcept{
				if(m_it == m_begin) return *this;
				--m_it;
				m_val = token(m_tu, *m_it);
				return *this;
			}

			bool operator==(const token_iterator &other) const noexcept{
				return m_tu == other.m_tu && m_it == other.m_it;
			}

			bool operator!=(const token_iterator &other) const noexcept{
				return m_tu != other.m_tu || m_it != other.m_it;
			}

			using difference_type = long;
			using value_type = token;
			using pointer = const token*;
			using reference = const token&;
			using iterator_category = std::bidirectional_iterator_tag;

		private:
			token_iterator(CXTranslationUnit tu, CXToken *it, CXToken *begin_, CXToken *end_)
			: m_tu(tu)
			, m_it(it)
			, m_begin(begin_)
			, m_end(end_)
			, m_val(it == end_ ? token() : token(tu, *m_it))
			{}

			CXTranslationUnit m_tu = nullptr;
			CXToken *m_it = nullptr, *m_begin = nullptr, *m_end = nullptr;
			token m_val;

			friend class tokens;
	};

	class tokens{
		public:
			tokens(tokens &&other) noexcept
			: m_tu(std::exchange(other.m_tu, nullptr))
			, m_toks(std::exchange(other.m_toks, nullptr))
			, m_num_toks(std::exchange(other.m_num_toks, 0))
			{}

			tokens(const tokens&) = delete;

			~tokens(){
				clang_disposeTokens(m_tu, m_toks, m_num_toks);
			}

			tokens &operator=(tokens &&other) noexcept{
				if(this != &other){
					if(m_toks) clang_disposeTokens(m_tu, m_toks, m_num_toks);

					m_tu = std::exchange(other.m_tu, nullptr);
					m_toks = std::exchange(other.m_toks, nullptr);
					m_num_toks = std::exchange(other.m_num_toks, 0);
				}

				return *this;
			}

			tokens &operator=(const tokens&) = delete;

			unsigned int num_tokens() const noexcept{ return m_num_toks; }

			bool empty() const noexcept{ return m_num_toks == 0; }

			std::size_t size() const noexcept{ return m_num_toks; }

			auto begin() const noexcept{
				return token_iterator(m_tu, m_toks, m_toks, m_toks + m_num_toks);
			}

			auto end() const noexcept{
				const auto end_ptr = m_toks + m_num_toks;
				return token_iterator(m_tu, end_ptr, m_toks, end_ptr);
			}

		private:
			tokens(CXCursor c){
				m_tu = clang_Cursor_getTranslationUnit(c);
				auto extent = clang_getCursorExtent(c);

				if(c.kind == CXCursor_FieldDecl){
					CXSourceLocation loc = clang_getCursorLocation(c);
					while(--loc.int_data > 0){
						auto tokp = clang_getToken(m_tu, loc);
						if(!tokp){
							continue;
						}

						auto tok = clang::token(m_tu, *tokp);
						if(!tok.is_valid()){
							break;
						}

						if(tok.str() == ";" || tok.str() == "{"){
							++loc.int_data;
							extent = clang_getRange(loc, clang_getRangeEnd(extent));
							break;
						}
					}
				}

				clang_tokenize(m_tu, extent, &m_toks, &m_num_toks);
			}

			CXTranslationUnit m_tu = nullptr;
			CXToken *m_toks = nullptr;
			unsigned int m_num_toks = 0;

			friend class cursor;
	};

	class cursor;

	class type{
		public:
			type(CXType t) noexcept
				: m_handle(t){}

			type(const type&) noexcept = default;

			type &operator=(const type&) noexcept = default;

			operator CXType() const noexcept{ return m_handle; }			

			bool is_valid() const noexcept{ return m_handle.kind != CXType_Invalid; }

			CXTypeKind kind() const noexcept{ return m_handle.kind; }			

			std::string kind_spelling() const noexcept{
				return detail::convert_str(clang_getTypeKindSpelling(kind()));
			}

			std::string spelling(){
				return detail::convert_str(clang_getTypeSpelling(m_handle));
			}

			cursor declaration() const noexcept;

		private:
			CXType m_handle;
	};

	class cursor{
		public:
			cursor(CXCursor c) noexcept
				: m_handle(c){}

			cursor(const cursor&) noexcept = default;

			cursor &operator=(const cursor&) noexcept = default;

			operator CXCursor() const noexcept{ return m_handle; }

			CXCursorKind kind() const noexcept{ return clang_getCursorKind(m_handle); }

			class type type() const noexcept{ return clang_getCursorType(m_handle); }

			class tokens tokens() const noexcept{ return clang::tokens(m_handle); }

			std::string display_name(){
				return detail::convert_str(clang_getCursorDisplayName(m_handle));
			}

			std::string spelling(){
				return detail::convert_str(clang_getCursorSpelling(m_handle));
			}

			std::string kind_spelling(){
				return detail::convert_str(clang_getCursorKindSpelling(kind()));
			}

			bool is_null() const noexcept{
				return clang_Cursor_isNull(m_handle);
			}

			bool is_valid() const noexcept{
				return !clang_isInvalid(kind());
			}

			bool is_class_decl() const noexcept{
				return clang_getCursorKind(m_handle) == CXCursor_ClassDecl;
			}

			bool is_attribute() const noexcept{
				return clang_isAttribute(clang_getCursorKind(m_handle));
			}

			template<typename F, typename ... Args>
			void visit_children(F &&f, Args &&... args){
				using namespace std::placeholders;

				auto f0 = std::bind(std::forward<F>(f), _1, _2, std::forward<Args>(args)...);

				clang_visitChildren(
					m_handle,
					[](CXCursor c, CXCursor parent, CXClientData client_data){
						auto &&f = *reinterpret_cast<decltype(f0)*>(client_data);
						f(cursor(c), cursor(parent));
						return CXChildVisit_Continue;
					},
					&f0
				);
			}

			std::optional<std::size_t> num_args() const noexcept{
				auto n = clang_Cursor_getNumArguments(m_handle);
				if(n == -1){
					return std::nullopt;
				}

				return std::size_t(n);
			}

			std::optional<cursor> arg(std::size_t i) const noexcept{
				auto num_args_opt = num_args();
				if(!num_args_opt) return std::nullopt;

				auto &&n = *num_args_opt;
				if(i >= n) return std::nullopt;

				return clang_Cursor_getArgument(m_handle, i);
			}

		private:
			CXCursor m_handle;
	};

	inline cursor type::declaration() const noexcept{
		return clang_getTypeDeclaration(m_handle);
	}

	class compilation_database: public handle<CXCompilationDatabase, clang_CompilationDatabase_dispose>{
		public:
			explicit compilation_database(const fs::path &build_dir)
				: handle(nullptr)
			{
				auto build_dir_utf8 = build_dir.u8string();

				CXCompilationDatabase_Error db_err;
				auto comp_db = clang_CompilationDatabase_fromDirectory(build_dir_utf8.c_str(), &db_err);

				if(db_err != CXCompilationDatabase_NoError){
					auto msg = fmt::format("Compilation database could not be loaded from directory '{}'", build_dir_utf8.c_str());
					throw std::runtime_error(msg);
				}

				set_handle(comp_db);
			}

			compilation_database(compilation_database&&) = default;

			std::vector<std::string> all_options() const{
				std::vector<std::string> ret;

				auto cmds = clang_CompilationDatabase_getAllCompileCommands(*this);
				if(cmds){
					auto num_commands = clang_CompileCommands_getSize(cmds);
					ret.reserve(num_commands);

					for(unsigned int i = 0; i < num_commands; i++){
						auto cmd = clang_CompileCommands_getCommand(cmds, i);
						if(!cmd){
							continue;
						}

						auto num_args = clang_CompileCommand_getNumArgs(cmd);

						// always start from 1 and end 1 before the end
						// head is compiler executable
						// last is compiled file
						for(unsigned int j = 1; j < (num_args - 1); j++){
							auto arg = clang::detail::convert_str(clang_CompileCommand_getArg(cmd, j));
							ret.emplace_back(std::move(arg));
						}
					}

					std::fflush(stdout);

					clang_CompileCommands_dispose(cmds);
				}

				return ret;
			}

			std::vector<std::string> file_options(const fs::path &path) const{
				std::vector<std::string> ret;

				auto abs_path = fs::absolute(path);
				auto abs_path_utf8 = abs_path.u8string();

				auto cmds = clang_CompilationDatabase_getCompileCommands(*this, abs_path_utf8.c_str());
				if(cmds){
					auto num_commands = clang_CompileCommands_getSize(cmds);
					ret.reserve(num_commands);

					for(unsigned int i = 0; i < num_commands; i++){
						auto cmd = clang_CompileCommands_getCommand(cmds, i);
						if(!cmd){
							continue;
						}

						auto num_args = clang_CompileCommand_getNumArgs(cmd);

						// always start from 1 and end 1 before the end
						// head is compiler executable
						// last is compiled file
						for(unsigned int j = 1; j < (num_args - 1); j++){
							auto arg = clang::detail::convert_str(clang_CompileCommand_getArg(cmd, j));
							ret.emplace_back(std::move(arg));
						}
					}

					std::fflush(stdout);

					clang_CompileCommands_dispose(cmds);
				}

				return ret;
			}
	};

	class translation_unit: public handle<CXTranslationUnit, clang_disposeTranslationUnit>{
		public:
			translation_unit()
				: handle(nullptr)
			{}

			translation_unit(translation_unit&&) = default;

			translation_unit(CXIndex index, const fs::path &path, const std::vector<std::string> &options = {})
				: handle(nullptr)
			{
				std::vector<const char*> option_cstrs;
				option_cstrs.reserve(options.size());

				std::transform(
					options.begin(), options.end(),
					std::back_inserter(option_cstrs),
					[](const std::string &opt){ return opt.c_str(); }
				);

				const auto num_options = static_cast<unsigned int>(options.size());

				auto path_utf8 = path.u8string();

				CXTranslationUnit tu = nullptr;
				auto parse_err = clang_parseTranslationUnit2(
					index, path_utf8.c_str(),
					option_cstrs.data(), num_options,
					nullptr, 0,
					CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_KeepGoing,
					&tu
				);

				switch(parse_err){
					case CXError_Success: break;

					case CXError_Failure:{
						std::string errMsg;
						if(tu){
							const unsigned int num_diag = clang_getNumDiagnostics(tu);

							bool found_err = false;

							for(unsigned int i = 0; i < num_diag; i++){
								CXDiagnostic diagnotic = clang_getDiagnostic(tu, i);
								auto err_str =
									clang::detail::convert_str(
										clang_formatDiagnostic(diagnotic, clang_defaultDiagnosticDisplayOptions())
									);

								errMsg += err_str;
							}
						}
						else{
							errMsg = fmt::format("Failure in clang_parseTranslationUnit2 for '{}'", path_utf8);
						}

						throw std::runtime_error(errMsg);
					}

					case CXError_Crashed:{
						throw std::runtime_error(fmt::format("libclang crashed while in clang_parseTranslationUnit2 for '{}'", path_utf8));
					}

					case CXError_InvalidArguments:{
						throw std::runtime_error(fmt::format("clang_parseTranslationUnit2 detected that it's arguments violate the function contract for '{}'", path_utf8));
					}

					case CXError_ASTReadError:{
						throw std::runtime_error(fmt::format("An AST deserialization error occurred for '{}'", path_utf8.c_str()));
					}

					default:{
						throw std::runtime_error(fmt::format("Unknown error in clang_parseTranslationUnit2 for '{}'", path_utf8));
					}
				}

				set_handle(tu);
			}

			cursor get_cursor() const noexcept{
				return clang_getTranslationUnitCursor(*this);
			}
	};

	inline std::string version(){
		return detail::convert_str(clang_getClangVersion());
	}
}

#endif // !METACPP_AST_CLANG_HPP
