#define BOOST_TEST_MODULE DynamicVariantTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
namespace tt = boost::test_tools;
namespace bd = boost::unit_test::data;

#include "gulachek/dynamic_variant.hpp"
#include <variant>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <gulachek/gtree.hpp>
#include <gulachek/gtree/encoding/fusion.hpp>

namespace gt = gulachek::gtree;

template <typename ...Ts>
using dv = gulachek::dynamic_variant<Ts...>;

class instance_counter
{
	public:
		static std::size_t count() { return _count; }

		instance_counter()
		{
			++_count;
		}

		instance_counter(const instance_counter &ic)
		{
			++_count;
		}

		instance_counter(instance_counter &&ic)
		{
			++_count;
		}

		~instance_counter()
		{
			--_count;
		}

	private:
		static std::size_t _count;
};

std::size_t instance_counter::_count = 0;

struct flint : dv<float, int>
{
	template <typename ...Ts>
	flint(Ts ...args) : dv<float, int>{args...} {}
};

template <typename T>
struct wrap
{
	wrap(const T &t) : _val{t} {}

	operator T& () { return _val; }
	operator T const& () const { return _val; }

	public:
		T _val;
};

BOOST_AUTO_TEST_SUITE(Initialization)

BOOST_AUTO_TEST_CASE(DefaultIsValueless)
{
	const dv<int, std::string> v;
	BOOST_TEST(v.valueless_by_exception());
}

BOOST_AUTO_TEST_CASE(IdenticalFirst)
{
	dv<int, std::string> v = 1;
	BOOST_TEST(v.index() == 0);
}

BOOST_AUTO_TEST_CASE(IdenticalSecond)
{
	dv<int, std::string> v = std::string{"hello"};
	BOOST_TEST(v.index() == 1);
}

BOOST_AUTO_TEST_CASE(CharStarForStdString)
{
	dv<int, std::string> v = "hello";
	BOOST_TEST(v.index() == 1);
}

BOOST_AUTO_TEST_CASE(IntForUnsigned)
{
	dv<unsigned, std::string> v = 3;
	BOOST_TEST(v.index() == 0);
}

BOOST_AUTO_TEST_SUITE_END() // Initialization

BOOST_AUTO_TEST_CASE(DestructorFreesMemory)
{
	auto start = instance_counter::count();

	{
		dv<unsigned, instance_counter> v = instance_counter{};
		BOOST_TEST(instance_counter::count() > start);
	}

	BOOST_TEST(instance_counter::count() == start);
}

BOOST_AUTO_TEST_SUITE(GetIf)

BOOST_AUTO_TEST_CASE(SimpleInt)
{
	const dv<int, std::string> v = 3;
	const int* pint = std::get_if<int>(&v);
	BOOST_TEST(*pint == 3);
}

BOOST_AUTO_TEST_CASE(SimpleString)
{
	dv<int, std::string> v = "hello";
	auto pstr = std::get_if<std::string>(&v);
	BOOST_TEST(*pstr == "hello");
}

BOOST_AUTO_TEST_CASE(MismatchInt)
{
	dv<int, std::string> v = 3;
	auto pstr = std::get_if<std::string>(&v);
	BOOST_CHECK(!pstr);
}

BOOST_AUTO_TEST_CASE(MismatchString)
{
	dv<int, std::string> v = "hello";
	auto pint = std::get_if<int>(&v);
	BOOST_CHECK(!pint);
}

BOOST_AUTO_TEST_CASE(IntCanModifyInPlace)
{
	dv<int, std::string> v = 3;
	auto pint = std::get_if<int>(&v);
	*pint = 4;

	auto same = std::get_if<int>(&v);

	BOOST_TEST(*same == 4);
}

BOOST_AUTO_TEST_SUITE_END() // GetIf

BOOST_AUTO_TEST_CASE(CopyConstruct)
{
	const dv<int, std::string> v = 3;
	dv<int, std::string> v2{v};

	auto n = std::get<int>(v2);

	BOOST_TEST(n == 3);
}

BOOST_AUTO_TEST_CASE(CopyAssign)
{
	const dv<int, std::string> v = 3;
	dv<int, std::string> v2;
	v2 = v;

	auto n = std::get<int>(v2);

	BOOST_TEST(n == 3);
}

BOOST_AUTO_TEST_CASE(WrappedCanAssignCopy)
{
	dv<int, std::string> v = 3;
	wrap<dv<int, std::string>> w{v};

	dv<int, std::string> v2 = w;

	auto n = std::get<int>(v2);
	BOOST_TEST(n == 3);
}

BOOST_AUTO_TEST_CASE(WrappedCanAssignMove)
{
	dv<int, std::string> v = 3;
	wrap<dv<int, std::string>> w{v};

	dv<int, std::string> v2 = std::move(w);

	auto n = std::get<int>(v2);
	BOOST_TEST(n == 3);
}

BOOST_AUTO_TEST_CASE(CopyFromDerivative)
{
	flint fl = 3.0f;
	dv<float, int> v = fl;

	auto f = std::get<float>(v);
	BOOST_TEST(f == 3.0f);
}

BOOST_AUTO_TEST_CASE(MoveFromDerivative)
{
	flint fl = 3.0f;
	dv<float, int> v = std::move(fl);

	auto f = std::get<float>(v);
	BOOST_TEST(f == 3.0f);
}

/*
BOOST_AUTO_TEST_CASE(WrappedCanAssignMove)
{
	dv<int, std::string> v = 3;
	wrap<dv<int, std::string>> w{v};

	dv<int, std::string> v2 = std::move(w);

	auto n = std::get<int>(v2);
	BOOST_TEST(n == 3);
}
*/

BOOST_AUTO_TEST_SUITE(Visit)

struct strint_visitor
{
	std::string operator() (const std::string &s)
	{
		return "string";
	}

	std::string operator() (const int &s)
	{
		return "int";
	}
};

BOOST_AUTO_TEST_CASE(String)
{
	strint_visitor vis;
	dv<std::string, int> v = "hello";
	auto t = gulachek::visit(vis, v);
	BOOST_TEST(t == "string");
}

BOOST_AUTO_TEST_CASE(Int)
{
	strint_visitor vis;
	dv<std::string, int> v = 10;
	auto t = gulachek::visit(vis, v);
	BOOST_TEST(t == "int");
}

BOOST_AUTO_TEST_SUITE_END() // Visit

struct empty {};

BOOST_FUSION_ADAPT_STRUCT(::empty);

template <typename T>
struct cons;

template <typename T>
using linked_list = dv<empty, cons<T>>;

template <typename T>
struct cons
{
	T head;
	linked_list<T> tail;
};

BOOST_FUSION_ADAPT_TPL_STRUCT(
		(T),
		(::cons) (T),
		head,
		tail
		);

template <typename T>
std::size_t size(const linked_list<T> &l)
{
	struct {
		std::size_t operator() (const empty &)
		{ return 0; }

		std::size_t operator() (const cons<T> c)
		{ return 1 + size(c.tail); }
	} vis;

	return gulachek::visit(vis, l);
}

BOOST_AUTO_TEST_SUITE(LinkedListUseCase)

BOOST_AUTO_TEST_CASE(Empty)
{
	linked_list<int> l = empty{};
	BOOST_TEST(size(l) == 0);
}

BOOST_AUTO_TEST_CASE(SingleElem)
{
	linked_list<int> l = cons<int>{1, empty{}};
	BOOST_TEST(size(l) == 1);
}

BOOST_AUTO_TEST_CASE(TwoElems)
{
	linked_list<double> l =
		cons<double>{1.0, cons<double>{2.0, empty{}}};
	BOOST_TEST(size(l) == 2);
}

BOOST_AUTO_TEST_CASE(EncodeList)
{
	using u = std::size_t;
	linked_list<u> l = cons<u>{1, cons<u>{2, empty{}}};
	gt::mutable_tree tr;

	gt::encode(l, tr);

	// We know this isn't empty
	std::size_t index = 10;
	gt::decode(tr, index);
	BOOST_TEST(index == 1);

	// let's decode again
	linked_list<u> reconstruct;
	gt::decode(tr, reconstruct);
	BOOST_TEST(size(reconstruct) == 2);

	auto first = std::get<cons<u>>(reconstruct);
	auto second = std::get<cons<u>>(first.tail);
	auto last = std::get<empty>(second.tail);

	BOOST_TEST(first.head == 1);
	BOOST_TEST(second.head == 2);
}

BOOST_AUTO_TEST_CASE(BadCastToEmpty)
{
	using u = std::size_t;
	linked_list<u> l = cons<u>{1, cons<u>{2, empty{}}};

	auto go = [&](){
		volatile auto x = std::get<empty>(l);
	};

	BOOST_CHECK_THROW(go(), std::bad_variant_access);
}

BOOST_AUTO_TEST_SUITE_END() // LinkedListUseCase
