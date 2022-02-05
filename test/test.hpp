/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef TEST_TEST_HPP
#define TEST_TEST_HPP 1

#include <string_view>

class TestPredefined;

namespace test{
	class TestClassNS{
		public:
			void test_member(std::string_view a);

			float test_member2(float a, float b){ return a + b; }
	};
}

using TestUsing = test::TestClassNS;

using namespace test;

typedef TestClassNS TestTypedef;

enum class [[special_enum]] TestEnum{
	_0, _1 = 420, _2,
	a, b = 69, c,
	count
};

class TestClass{
	public:
		int m_0;
		float m_1;

		std::string_view f_0(std::string &str) const noexcept;
};

class TestBase{
	public:
		virtual ~TestBase() = default;
};

class TestDerived: public TestBase, private TestClass{
	public:
};

template<typename T>
struct TestTemplateClass{
	public:
		using pointer = T*;

		TestTemplateClass() = default;

		template<typename U>
		TestTemplateClass(U &&val) noexcept(noexcept(T(std::forward<U>(val))))
			: m_value(std::forward<U>(val)){}

		template<typename U>
		void set_value(U &&v){
			m_value = std::forward<U>(v);
		}

		template<typename U>
		TestTemplateClass<U> rebind(U &&v){
			return TestTemplateClass{std::forward<U>(v)};
		}

		const T &value() const noexcept{ return m_value; }

		pointer ptr() noexcept{ return &m_value; }

	private:
		T m_value;
};

template<typename T>
TestTemplateClass(T&&) -> TestTemplateClass<std::decay_t<T>>;

inline bool operator>(TestTemplateClass<int> a, TestTemplateClass<int> b){
	return a.value() > b.value();
}

namespace detail{
	template<typename ... Ts>
	class TestDerivedTemplateHelper: public TestTemplateClass<Ts>...{

	};
}

namespace test{
	class TestDerivedTemplate: public TestTemplateClass<std::string_view>{};
}

class [[test::attrib]] TestClassAttrib{
	public:
		TestClassAttrib(){}
		~TestClassAttrib(){}
};

class [[test::attrib, foo::bar(1, "2", '3', 4.0, 5.f)]] TestClass2Attribs{
	public:
		TestClass2Attribs(){}
		~TestClass2Attribs(){}
};

class [[test::predefined]] TestPredefined{};

[[other::attrib(with, "args")]]
inline void testFn(int a, TestClass b){
}

#endif // !TEST_TEST_HPP
