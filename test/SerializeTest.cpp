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
    auto someRawArray = std::array<std::string*, 3>{new std::string("abc"), new std::string("def"), new std::string("ghi")};
    binaryArchive.save(someArray);
    binaryArchive.save(someRawArray);
    ASSERT_TRUE(binaryArchive.load<decltype(someArray)>() == someArray);
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
DEFINE_SMART_POINTER_COMPARISIONS(std::unique_ptr<Point>)
DEFINE_SMART_POINTER_COMPARISIONS(std::shared_ptr<Point>)
DEFINE_HASHABLE(Point)

struct SmartWrapper
{
    DEFINE_SERIALIZABLE(SmartWrapper, unique, shared, optional)
    std::unique_ptr<Point> unique;
    std::shared_ptr<Point> shared;
    std::optional<Point> optional;
};

TEST(binaryArchive, smartPointerOfCustom)
{
    BinaryArchive binaryArchive;
    SmartWrapper smartWrapper{ std::unique_ptr<Point>(new Point{1,2}), std::shared_ptr<Point>(new Point{1,2}),  std::optional<Point>(Point{1,2}) };
    binaryArchive.save(smartWrapper);
    SmartWrapper smartWrapper2 =  binaryArchive.load<SmartWrapper>();
    ASSERT_EQ(smartWrapper, smartWrapper2);
}


struct ComplexType
{
    ComplexType() = default;
    ComplexType(Point p1, Point p2) : x1(p1), x2(p2) {}
    DEFINE_SERIALIZABLE(ComplexType, x1, x2, intArray, ints, mapping, sets)
    Point x1, x2;
    std::array<Point, 4> intArray = { x1, x2, x1, x2 };
    std::vector<char> ints = { '1','2','3'};
    std::unordered_map<char, char> mapping{ {'a', 'b'},  {'b', 'c'} };
    std::unordered_set<Point> sets{ Point{1,2}, Point{3,4} };

    std::optional<std::unique_ptr<Point>> opt;
};

TEST(binaryArchive, complexType)
{
       BinaryArchive binaryArchive;
       ComplexType c(Point{ 1,2 }, Point{ 3, 4 });
       c.ints.push_back('4');
       c.opt = std::make_optional(std::unique_ptr<Point>(new Point{ 7,8 }));
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
