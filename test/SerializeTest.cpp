#include "gtest/gtest.h"
#include "tser/tser.hpp"

#include <unordered_map>
#include <unordered_set>
#include <optional>

//if you need deep pointer comparisions you could grab this macro
namespace tser::detail {
    template <class Tuple, std::size_t... I>
    bool compareTuples(const Tuple& lh, const Tuple& rh, std::index_sequence<I...>) {
        auto compareEQ = [](const auto& lhs, const auto& rhs) { if constexpr (!tser::is_pointer_v<std::remove_reference_t<decltype(lhs)>>) { return lhs == rhs; }
        else { if (lhs && rhs) { return *lhs == *rhs; } else if (!lhs && !rhs) { return true; } return false; } };
        return (compareEQ(std::get<I>(lh), std::get<I>(rh)) &&  ...);
    }
}
#define DEFINE_DEEP_POINTER_COMPARISION(Type)\
friend bool operator==(const Type& lhs, const Type& rhs){ return tser::detail::compareTuples(lhs.members(), rhs.members(), std::make_index_sequence<std::tuple_size_v<decltype(lhs.members())>>{});}
//if a complex type doesn't have a hash function and your too lazy to implement one, you could use this ugly hack
#define DEFINE_HASHABLE(Type) \
namespace std { \
        template<> \
        struct hash<Type> { \
            size_t operator()(const Type& t) const { \
                tser::BinaryArchive bs; \
                bs & t; \
                return std::hash<std::string_view>()(bs.get_buffer()); \
        } \
    }; \
}

TEST(binaryArchive, readBits)
{
    tser::BinaryArchive binaryArchive;
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
    tser::BinaryArchive binaryArchive;
    binaryArchive.save(15);
    binaryArchive.save(256);
    binaryArchive.save(-256);
    binaryArchive.reset();
    ASSERT_TRUE(binaryArchive.load<int>() == 15);
    ASSERT_TRUE(binaryArchive.load<int>() == 256);
    ASSERT_TRUE(binaryArchive.load<int>() == -256);
}

enum class SomeEnum { A, B, C };
TEST(binaryArchive, readPair)
{
    tser::BinaryArchive binaryArchive;
    auto somePair = std::pair(SomeEnum::A, std::string("A"));
    binaryArchive & somePair;
    ASSERT_EQ(binaryArchive.load<decltype(somePair)>(), somePair);
}

TEST(binaryArchive, readTuple)
{
    tser::BinaryArchive binaryArchive;
    auto someTuple = std::make_tuple(1, 2.0, std::string("Hello"));
    auto someTrivialTuple = std::make_tuple(1, 2.0);
    binaryArchive & someTuple;
    binaryArchive & someTrivialTuple;
    ASSERT_TRUE(binaryArchive.load<decltype(someTuple)>() == someTuple);
    ASSERT_EQ(binaryArchive.load<decltype(someTrivialTuple)>(), someTrivialTuple);
}

TEST(binaryArchive, readArray)
{
    tser::BinaryArchive binaryArchive;
    auto someArray = std::array<std::string, 3>{"abc", "def", "ghi"};
    binaryArchive & someArray;
    auto loadedRawArray = binaryArchive.load<decltype(someArray)>();
    ASSERT_EQ(loadedRawArray, someArray);
}

TEST(binaryArchive, readRawPtr)
{
    tser::BinaryArchive binaryArchive;
    auto someArray = new std::string("Hello World!");
    binaryArchive & someArray;
    ASSERT_TRUE(*std::unique_ptr<std::string>(binaryArchive.load<decltype(someArray)>()) == "Hello World!");
}

TEST(binaryArchive, readCArray)
{
    tser::BinaryArchive binaryArchive;
    int someInts[4] = { 1,2,3,4 };
    binaryArchive & someInts;
    int loadedInts[4];
    binaryArchive.load(loadedInts);
    ASSERT_TRUE(std::equal(someInts, someInts + 4, loadedInts, loadedInts + 4));
}


struct CustomPointWithMacro
{
    DEFINE_SERIALIZABLE(CustomPointWithMacro, x, y)
    void save(tser::BinaryArchive& ba) const {
        ba.save(x + y);
    }
    void load(tser::BinaryArchive& ba) {
        x = ba.load<int>();
    }
    int x = 1, y = 1;
};

TEST(binaryArchive, customSerialization)
{
    tser::BinaryArchive ba;
    ba.save(CustomPointWithMacro{ 3,4 });
    auto loadedPoint = ba.load<CustomPointWithMacro>();
    ASSERT_EQ(loadedPoint.x, 7);
    ASSERT_EQ(loadedPoint.y, 1);
}

struct CustomPointNoMacro
{
    void save(tser::BinaryArchive& ba) const {
        ba.save(x + y);
    }
    void load(tser::BinaryArchive& ba) {
        x = ba.load<int>();
    }
    int x = 1, y = 2;
};

TEST(binaryArchive, customSerializationNoMacro)
{
    tser::BinaryArchive ba;
    ba.save(CustomPointNoMacro{ 5,6 });
    auto loadedPoint = ba.load<CustomPointNoMacro>();
    ASSERT_EQ(loadedPoint.x, 11);
    ASSERT_EQ(loadedPoint.y, 2);
}
struct Point
{
    DEFINE_SERIALIZABLE(Point, x, y)
    short x = 0, y = 0;
};

TEST(binaryArchive, readPoints)
{
    tser::BinaryArchive binaryArchive;
    binaryArchive & Point{ 1,2 };
    binaryArchive & Point{ 5,6 };
    ASSERT_TRUE((binaryArchive.load<Point>() == Point{ 1,2 }));
    ASSERT_TRUE((binaryArchive.load<Point>() == Point{ 5,6 }));
}


//if we don't want pointer types to break our comparision operator we have to define them here
DEFINE_HASHABLE(Point)

struct PointerWrapper
{
    DEFINE_SERIALIZABLE(PointerWrapper, intPtr, unique, shared)
    DEFINE_DEEP_POINTER_COMPARISION(PointerWrapper)
    ~PointerWrapper() { delete intPtr; }
    int* intPtr = nullptr;
    std::unique_ptr<Point> unique;
    std::shared_ptr<Point> shared;
};

TEST(binaryArchive, smartPointerOfCustom)
{
    tser::BinaryArchive binaryArchive;
    PointerWrapper smartWrapper{ new int(5), std::unique_ptr<Point>(new Point{1,2}), std::shared_ptr<Point>(new Point{1,2}) };
    //the content of a raw pointer is serialized, not the address
    binaryArchive & (&smartWrapper);
    //the serialized layout of shared_ptr<T>, unique_ptr<T>, optional<T> and T* are all the same
    //so if I really wanted to I could load T* as any of them
    auto loadedSmartWrapper = binaryArchive.load<std::unique_ptr<PointerWrapper>>();
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
    std::vector<char> ints = { '1','2','3' };
    std::unordered_map<char, char> mapping{ {'a', 'b'},  {'b', 'c'} };
    std::unordered_set<Point> sets{ Point{1,2}, Point{3,4} };

    std::optional<Point> opt;
};

TEST(binaryArchive, complexType)
{
    tser::BinaryArchive binaryArchive;
    ComplexType c(Point{ 1,2 }, Point{ 3, 4 });
    c.ints.push_back('4');
    c.opt = std::make_optional(Point{ 7,8 });
    binaryArchive & c;
    auto str = tser::encode_base64(binaryArchive.get_buffer());
    tser::BinaryArchive readStream;
    readStream.initialize(tser::decode_base64(str));
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

TEST(VLE, unsigned_encode_decode_up_to_513)
{
    std::string somebuffer(10, '\0');
    for (size_t i = 0; i < 513; ++i)
    {
        tser::encode_varint(i, somebuffer.data());
        size_t decoded;
        tser::decode_varint(decoded, somebuffer.data());
        ASSERT_EQ(i, decoded);
    }
}

TEST(VLE, signed_encode_decode_up_513)
{
    std::string somebuffer(10, '\0');
    for (int i = -256; i < 257; ++i)
    {
        tser::encode_varint(i, somebuffer.data());
        int decoded;
        tser::decode_varint(decoded, somebuffer.data());
        ASSERT_EQ(i, decoded);
    }
}
