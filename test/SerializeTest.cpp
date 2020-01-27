#include "gtest/gtest.h"

#include "tser/base64encoding.hpp"
#include "tser/serialize.hpp"
#include "tser/compare.hpp"
#include "tser/print.hpp"

#include <unordered_map>
#include <unordered_set>
#include <optional>

using namespace tser;


TEST(binaryArchive, readBits)
{
       BinaryArchive binaryArchive;
       binaryArchive.save(true);
       binaryArchive.save(false);
       binaryArchive.save(true);
       binaryArchive.reset();
       ASSERT_TRUE(binaryArchive.load<bool>());
       ASSERT_FALSE(binaryArchive.load<bool>());
       ASSERT_TRUE(binaryArchive.load<bool>());
}


TEST(binaryArchive, readInts)
{
       BinaryArchive binaryArchive;
       binaryArchive.save(15);
       binaryArchive.save(1);
       binaryArchive.save(-15);
       binaryArchive.reset();
       ASSERT_TRUE(binaryArchive.load<int>() == 15);
       ASSERT_TRUE(binaryArchive.load<int>() == 1);
       ASSERT_TRUE(binaryArchive.load<int>() == -15);
}

enum class SomeEnum {A,B,C};
TEST(binaryArchive, readPair)
{
    BinaryArchive binaryArchive;
    auto somePair = std::pair(SomeEnum::A, std::string("A"));
    binaryArchive.save(somePair);
    ASSERT_TRUE(binaryArchive.load<decltype(somePair)>() == somePair);
}

TEST(binaryArchive, readTuple)
{
    BinaryArchive binaryArchive;
    auto someTuple = std::make_tuple(1, 2.0, std::string("Hello"));
    auto someTrivialTuple = std::make_tuple(1, 2.0);
    binaryArchive.save(someTuple);
    binaryArchive.save(someTrivialTuple);
    ASSERT_TRUE(binaryArchive.load<decltype(someTuple)>() == someTuple);
    ASSERT_EQ(binaryArchive.load<decltype(someTrivialTuple)>(), someTrivialTuple);
}

TEST(binaryArchive, readArray)
{
    BinaryArchive binaryArchive;
    auto someArray = std::array<std::string, 3>{"abc", "def", "ghi"};
    binaryArchive.save(someArray);
    auto loadedRawArray = binaryArchive.load<decltype(someArray)>();
    ASSERT_EQ(loadedRawArray, someArray);
}

TEST(binaryArchive, readRawPtr)
{
    BinaryArchive binaryArchive;
    auto someArray = new std::string("Hello World!");
    binaryArchive.save(someArray);
    ASSERT_TRUE(*std::unique_ptr<std::string>(binaryArchive.load<decltype(someArray)>()) == "Hello World!");
}

struct Point
{
    DEFINE_SERIALIZABLE(Point, x, y)
    short x = 0, y = 0;
};

TEST(binaryArchive, readCustomPoints)
{
    BinaryArchive binaryArchive;
    binaryArchive.save(Point{1,2});
    binaryArchive.save(Point{5,6});
    ASSERT_TRUE((binaryArchive.load<Point>() == Point{ 1,2 }));
    ASSERT_TRUE((binaryArchive.load<Point>() == Point{ 5,6 }));
}


//if we don't want pointer types to break our comparision operator we have to define them here
DEFINE_HASHABLE(Point)

struct PointerWrapper
{
    DEFINE_SERIALIZABLE(PointerWrapper, intPtr, unique, shared)
    DEFINE_DEEP_POINTER_COMPARISION(PointerWrapper)

    int* intPtr = nullptr;
    std::unique_ptr<Point> unique;
    std::shared_ptr<Point> shared;
};

TEST(binaryArchive, smartPointerOfCustom)
{
    BinaryArchive binaryArchive;
    PointerWrapper smartWrapper{new int(5), std::unique_ptr<Point>(new Point{1,2}), std::shared_ptr<Point>(new Point{1,2}) };
    //the content of a raw pointer is serialized, not the address
    binaryArchive.save(&smartWrapper);
    //the serialized layout of shared_ptr<T>, unique_ptr<T>, optional<T> and T* are all the same
    //so if I really wanted to I could load T* as optional
    auto loadedSmartWrapper =  binaryArchive.load<std::optional<PointerWrapper>>();
    //here we test if the deep pointer comparision macro works (as well as the serialization and deserialization of T*, unique_ptr<T>, shared_ptr<T> and optional<T>)
    ASSERT_EQ(smartWrapper, *loadedSmartWrapper);
}


struct ComplexType
{
    ComplexType() = default;
    ComplexType(Point p1, Point p2) : x1(p1), x2(p2) {}
    DEFINE_SERIALIZABLE(ComplexType, x1, x2, intArray, ints, mapping, sets, opt)
    Point x1, x2;
    std::array<Point, 4> intArray = { x1, x2, x1, x2 };
    std::vector<char> ints = { '1','2','3'};
    std::unordered_map<char, char> mapping{ {'a', 'b'},  {'b', 'c'} };
    std::unordered_set<Point> sets{ Point{1,2}, Point{3,4} };

    std::optional<Point> opt;
};

TEST(binaryArchive, complexType)
{
       BinaryArchive binaryArchive;
       ComplexType c(Point{ 1,2 }, Point{ 3, 4 });
       c.ints.push_back('4');
       c.opt = std::make_optional(Point{ 7,8 });
       binaryArchive.save(c);
       auto str = base64_encode(binaryArchive.getString());
       BinaryArchive readStream;
       readStream << str;
       std::cout << c;
       ComplexType c2;
       c2.ints.clear();
       c2.sets.clear();
       readStream.load(c2);

       ASSERT_EQ(c, c2);
}

TEST(hashing, points)
{
    std::unordered_set<Point> points;
    points.insert(Point{ 0,0 });
    points.insert(Point{ 0,1 });
    points.insert(Point{ 2,3 });
    //just a compile time check
    ASSERT_TRUE(true);
}
