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
