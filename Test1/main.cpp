#include <utility>
#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <Windows.h>

namespace Details
{

using namespace std;


struct _Tag_1 {};
struct _Tag_2 {};

template <typename...>
struct _Tagger_1 { using type = _Tag_1; };

template <typename... Ts>
using Tag1_t = typename _Tagger_1<Ts...>::type;

template <typename...>
struct _Tagger_2 { using type = _Tag_2; };

template <typename... Ts>
using Tag2_t = typename _Tagger_2<Ts...>::type;



template <typename FunObj, typename Ret, typename Obj, typename Tuple, size_t... idx>
decltype(auto) CallWithTuple(
    Obj &&o
    , Ret FunObj::*f
    , Tuple&& t
    , index_sequence<idx...>
    // make sure it is defined only if f is callable with these arguments
    , decltype((declval<Obj>().*f)(get<idx>(declval<Tuple>())...)) * = nullptr
)
{
    return (forward<Obj>(o).*f)(
        get<idx>(forward<Tuple>(t))...
        );
}

// The trick is ReturnRc is always called with the last argument a pointer
// to the out parameter. When f is a function that doesn't return the result
// in the last out argument, it will have one argument less than ReturnRc
// and therefore decltype(declval<F>()(declval<Args>()...)) won't have
// sense, ReturnRC below will be disregarded by SFINAE and the second
// ReturnRC will be called. For example let's have 
// int foo()
// {
//     return -1;
// }
// We call it using
// {
//     RC rc;
//     int a;
//     rc = ReturnRc(foo, &a);
// }
template <typename Ret, typename FunObj, typename... Args>
decltype(auto) Foo(Ret FunObj::*f, Args&&... args) { }
// The parameters pack below will be <int(&)(), int *&> and the second
// argument will be decltype(foo(int *&)) *, which doesn't make sense,
// because foo doesn't have arguments at all.

constexpr int OK = 0;

using RC = int;

template <typename FunObj, typename Ret, typename Obj, typename... Args>
RC ReturnRC(
    Obj &&o
    , Ret FunObj::*f
    , Tag1_t<decltype((declval<Obj>().*f)(declval<Args>()...))> *
    , Args  &&...args
)
{
    return (forward<Obj>(o).*f)(forward<Args>(args)...);
}

// A aa;
// int a;
//rc = RC_WRAP(aa, foo, &a);

// template <A, RC, A, int>
//ReturnRC(aa, &A::foo, nullptr, a);
// (
// A&& o,
// 
// decltype(


template <typename FunObj, typename Ret, typename Obj, typename... Args>
RC ReturnRC(
    Obj&& o
    , Ret FunObj::*f
    , Tag2_t<decltype(CallWithTuple(declval<Obj>(), f, declval<tuple<Args...>>(), make_index_sequence<sizeof...(Args)-1>()))> *
    , Args&&... args
)
{
    // invoke f without the last arg
    auto ret = CallWithTuple(
        forward<Obj>(o)
        , f
        , forward_as_tuple(args...)
        , make_index_sequence<sizeof...(args)-1>()
    );
    // stuff the result into the last argument
    *get<sizeof...(args)-1>(forward_as_tuple(args...)) = ret;

    return OK;
}



// RcWrap assumes that the last parameter of the arguments pack is an out
// parameter. If f can be called with the args, RcWrap calls it and returns what
// f returns. If f cannot be called with args, RcWrap will try to call f without
// the last argument supplied to RcWrap, then copy the f result to the last out
// parameter and return OK.
template <typename FunObj, typename Ret, typename Obj, typename... Args>
RC RcWrap(Obj &&o, Ret FunObj::*f, Args &&...args)
{
    return ReturnRC(forward<Obj>(o), f, nullptr, forward<Args>(args)...);
}

struct A
{
    RC foo(int *pval) const
    {
        *pval = 5;
        return -1;
    }
};

struct B {
    int bar() const
    {
        return 6;
    }
};

#define RC_WRAP(obj, fun, ...) RcWrap(obj, &(decay_t<decltype(obj)>::fun), __VA_ARGS__)

#define RC_WRAP_PTR(ptr, fun, ...) RcWrap(*ptr, &(remove_pointer<decltype(ptr)>::type::fun), __VA_ARGS__)


void test()
{
    RC rc;

    A aa;
    A* aPtr = new A;

    B bb;
    int b;

    int a;

    //rc = RcWrap(aa, &A::foo, &a);
    //rc = aa.foo();
    rc = RC_WRAP(aa, foo, &a);

    cout << "a = " << a << ", rc = " << (int)rc << "\n";
    rc = RC_WRAP(bb, bar, &b);
    cout << "b = " << b << ", rc = " << (int)rc << "\n";

    //rc = RC_WRAP(*aPtr, foo, &a);
    rc = RcWrap(*aPtr, &A::foo, &a);
    //cout << "a = " << a << ", rc = " << (int)rc << "\n";
    //rc = RC_WRAP(*aPtr, bar, &a);
    //cout << "a = " << a << ", rc = " << (int)rc << "\n";*/

    auto fp = &A::foo;

    Tag1_t<int> x;

    delete aPtr;
}

}

//#define RC_WRAP_PTR(ptr, fun, ...) Details::RcWrap(*ptr, &(std::remove_pointer<decltype(ptr)>::type::fun), __VA_ARGS__)


namespace declval_test
{

struct EmptyConstructor {
    EmptyConstructor() { }
    int foo() const { return 1; }
};

struct FullConstructor {
    FullConstructor(int, double, float) { }
    double foo() const { return 1.0; }
};

struct NoConstructor {
    NoConstructor() = delete;
    char foo() const { return 'c'; }
};

struct NoConstructorStatic {
    NoConstructorStatic() = delete;
    static unsigned long foo() { return 1; }
};

void test()
{
    decltype(EmptyConstructor().foo()) x = 2;

    decltype(std::declval<FullConstructor>().foo()) y = 3.0;

    decltype(std::declval<NoConstructor>().foo()) z = 'z';

    decltype(NoConstructorStatic::foo()) a = 4;

    decltype(std::declval<NoConstructorStatic>().foo()) b = 5;
}

}

namespace decltype_test
{
	using namespace std;
	
	string& strRef()
	{
		return string("wot");
	}

	const int constInt()
	{
		return 3;
	}

	void test()
	{
		auto s = strRef();
		decltype(auto) s2 = strRef();
		//cout << strRef() << endl;

		decltype(auto) x = constInt();
		const int y = constInt();
	}
}
namespace thread_test
{

DWORD __stdcall f1Starter(void* p)
{
	CRITICAL_SECTION* cs = (CRITICAL_SECTION*)p;
	EnterCriticalSection(cs);
	printf("Thread f1\n");
	printf("GetCurrentThread() = 0x%X\n", GetCurrentThread());
	printf("GetCurrentThreadId() = %d\n", GetCurrentThreadId());
	LeaveCriticalSection(cs);

	return TRUE;
}

DWORD __stdcall f2Starter(void* p)
{
	CRITICAL_SECTION* cs = (CRITICAL_SECTION*)p;
	EnterCriticalSection(cs);
	printf("Thread f2\n");
	printf("GetCurrentThread() = 0x%X\n", GetCurrentThread());
	printf("GetCurrentThreadId() = %d\n", GetCurrentThreadId());
	LeaveCriticalSection(cs);

	return TRUE;
}

void test()
{
	CRITICAL_SECTION cs;
	InitializeCriticalSection(&cs);
	HANDLE h1, h2;
	DWORD tid1, tid2;
	h1 = CreateThread(NULL, 0, f1Starter, &cs, 0, &tid1);
	h2 = CreateThread(NULL, 0, f2Starter, &cs, 0, &tid2);

	printf("tid1 = %d\ntid2 = %d\n", tid1, tid2);

	WaitForSingleObject(h1, INFINITE);
	WaitForSingleObject(h2, INFINITE);
}

}

namespace lt200
{
using namespace std;

class Solution {
public:
    inline bool isWater(vector<string>& grid, int i, int j)
    {
        return (i < 0 || j < 0 || i >= grid.size() || j >= grid[0].size() || grid[i][j] != '1');
    }
    
    void destroyLand(vector<string>& grid, int i, int j)
    {
        if (isWater(grid, i, j)) return;
		grid[i][j] = '0';
        destroyLand(grid, i+1, j);
        destroyLand(grid, i-1, j);
        destroyLand(grid, i, j+1);
        destroyLand(grid, i, j-1);
    }
    
    int numIslands(vector<string>& grid) {
        int count = 0;
        for (int i = 0; i < grid.size(); i++)
        {
            for (int j = 0; j < grid[i].size(); j++)
            {
                if (grid[i][j] == '1') {
                    destroyLand(grid, i, j);
                    count++;
                }
            }
        }
        return count;
    }
};

void test()
{
	Solution s;
	int result = s.numIslands(vector<string>{ "11110","11010","11000","00000" });

	cout << result << endl;
}

}

namespace dynamic_coin_change
{

void test()
{
	int coinValue[3] = { 10, 6, 1 };
	
}

}

namespace macro_test
{

#define QUOTE(x) #x
#define MY_MACRO 1

#define SETGET_PROP(propName, type) \
	type propName;

void test()
{
	printf("name %s\n", QUOTE(MY_MACRO));
}

class MyClass {

};

}


int main()
{
	//decltype_test::test();
	macro_test::test();

    return 0;
}