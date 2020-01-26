#include "gtest/gtest.h"

#include "tser/Base64Encoding.hpp"
#include "tser/Serialize.hpp"
#include "tser/Compare.hpp"
#include "tser/Print.hpp"

#include <unordered_map>
#include <unordered_set>
#include <optional>

using namespace tser;


TEST(bitstream, readBits)
{
       BinaryArchive bitstream;
       bitstream.save(true);
       bitstream.save(false);
       bitstream.save(true);
       bitstream.reset();
       ASSERT_TRUE(bitstream.load<bool>());
       ASSERT_FALSE(bitstream.load<bool>());
       ASSERT_TRUE(bitstream.load<bool>());
}


TEST(bitstream, readInts)
{
       BinaryArchive bitstream;
       bitstream.save(15);
       bitstream.save(1);
       bitstream.save(-15);
       bitstream.reset();
       ASSERT_TRUE(bitstream.load<int>() == 15);
       ASSERT_TRUE(bitstream.load<int>() == 1);
       ASSERT_TRUE(bitstream.load<int>() == -15);
}

enum class SomeEnum {A,B,C};
TEST(bitstream, readPair)
{
    BinaryArchive bitstream;
    auto somePair = std::pair(SomeEnum::A, std::string("A"));
    bitstream.save(somePair);
    ASSERT_TRUE(bitstream.load<decltype(somePair)>() == somePair);
}

TEST(bitstream, readTuple)
{
    BinaryArchive bitstream;
    auto someTuple = std::make_tuple(1, 2.0, std::string("Hello"));
    bitstream.save(someTuple);
    ASSERT_TRUE(bitstream.load<decltype(someTuple)>() == someTuple);
}

TEST(bitstream, readArray)
{
    BinaryArchive bitstream;
    auto someArray = std::array<std::string, 5>{"abc"};
    bitstream.save(someArray);
    ASSERT_TRUE(bitstream.load<decltype(someArray)>() == someArray);
}


struct Point
{
    DEFINE_SERIALIZABLE(Point, x, y)
    short x = 0, y = 0;
};

TEST(bitstream, readPoints)
{
    BinaryArchive bitstream;
    bitstream.save(Point{1,2});
    bitstream.save(Point{5,6});
    ASSERT_TRUE((bitstream.load<Point>() == Point{ 1,2 }));
    ASSERT_TRUE((bitstream.load<Point>() == Point{ 5,6 }));
}


//if we don't want pointer types to break our comparision operator we have to define them here
DEFINE_SMART_POINTER_COMPARISIONS(std::unique_ptr<Point>)
DEFINE_SMART_POINTER_COMPARISIONS(std::shared_ptr<Point>)
DEFINE_HASHABLE(Point)

struct ComplexType
{
    ComplexType() = default;
    ComplexType(Point p1, Point p2) : x1(p1), x2(p2) {}
    DEFINE_SERIALIZABLE(ComplexType, x1, x2, intArray, ints, mapping, sets, pUnique, pSUnique, opt)
    Point x1, x2;
    std::array<Point, 4> intArray = { x1, x2, x1, x2 };
    std::vector<char> ints = { '1','2','3' };
    std::unordered_map<char, char> mapping{ {'a', 'b'},  {'b', 'c'} };
    std::unordered_set<Point> sets{ Point{1,2}, Point{3,4} };
    std::unique_ptr<Point> pUnique;
    std::shared_ptr<Point> pSUnique;
    std::optional<std::unique_ptr<Point>> opt;
};

TEST(bitstream, readWrite)
{
       BinaryArchive bitstream;
       ComplexType c(Point{ 1,2 }, Point{ 3, 4 });
       c.ints.push_back(4);
       c.pUnique = std::unique_ptr<Point>(new Point{3,4});
       c.pSUnique = std::unique_ptr<Point>(new Point{5,6});
       c.opt = std::make_optional(std::unique_ptr<Point>(new Point{ 7,8 }));
       bitstream.save(c);
       auto str = base64_encode(bitstream.getString());
       BinaryArchive readStream;
       readStream << str;
       ComplexType c2;
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
