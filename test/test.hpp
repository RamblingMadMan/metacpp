/*
 *	Meta C++ Tool and Library
 *	Copyright (C) 2022  Keith Hammond
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation; either
 *	version 2.1 of the License, or (at your option) any later version.
 *
 *	This library is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	Lesser General Public License for more details.
 */

#ifndef TEST_HPP
#define TEST_HPP 1

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

class TestClass{
	public:
		int m_0;
		float m_1;

		std::string_view f_0(std::string &str) const noexcept;
};

class [[test::attrib]] TestClassAttrib{
	TestClassAttrib(){}
	~TestClassAttrib(){}
};

class [[test::attrib, foo::bar(1, "a", b, 'c')]] TestClass2Attribs{
	TestClass2Attribs(){}
	~TestClass2Attribs(){}
};

class [[test::predefined]] TestPredefined{};

[[other::attrib(with, "args")]]
inline void testFn(int a, TestClass b){
}

#endif // !TEST_HPP
