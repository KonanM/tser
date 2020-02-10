# tser - Tiny Serialization for C++
[![Build status](https://ci.appveyor.com/api/projects/status/ggjg8clbh6ytklvv?svg=true)](https://ci.appveyor.com/project/KinanMahdi/tser)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/KonanM/tser/blob/master/LICENSE)
[![Try online](https://img.shields.io/badge/try-online-blue.svg)](https://wandbox.org/permlink/gdgbD3t8i8hOWK6L)
[![Try online](https://img.shields.io/badge/try-online-blue.svg)](https://godbolt.org/z/fmnm7r)
[![Average time to resolve an issue](http://isitmaintained.com/badge/resolution/konanM/tser.svg)](http://isitmaintained.com/project/konanM/tser "Average time to resolve an issue")
[![Percentage of issues still open](http://isitmaintained.com/badge/open/konanM/tser.svg)](http://isitmaintained.com/project/konanM/tser "Percentage of issues still open")
## Why another C++ serialization library?

I searched for a small C++ serialization library (<20 KB) for some competitive programming contest and didn't find anything that suited my needs. 

I wanted a library that was small, but allowed me to avoid as much boilerplate as possible. Especially if you are quickly prototyping you want to avoid implementing serialization, printing and comparision operators manually.

**tldr:** be quicker to serialize your object, print it to the console, compare it and load it from a string than to figure out how other serialization libraries work. 

If you need a battle tested, non-intrusive and feature rich serialization libary, please have a look at [Boost](https://www.boost.org/doc/libs/1_72_0/libs/serialization/doc/index.html), [Cereal](https://uscilab.github.io/cereal/), [Bitsery](https://github.com/fraillt/bitsery), [Protobuf](https://developers.google.com/protocol-buffers), [Flatbuffers](https://google.github.io/flatbuffers/), [Yas](https://github.com/niXman/yas). They all provide a better feature set and better flexibility. Tser is meant to be tiny - copy one small header put a macro into a few places and be good to go. 

## Design goals
* serialization of nearly all of the STL containers and types, as well as custom containers that follow STL conventions
* implement pretty printing to the console **automatically**, but allow for user defined implementations
* implement comparision operators (equal, non-equal, smaller) **automatically**, but allow for user defined implementations
* support printing the serialized representation of an object to the console via base64 encoding (this way only printable characters are used, allows for easily loading objects into the debugger via strings)
* use minimal set of includes (```array, string, string_view, tuple, type_traits, iostream```) and only ~ 350 lines of code (including 50 lines of comments)

## Features

* C++17 header only and single header (~ 350 LOC)
* Header only version is split so that an even more minimal subset of the libary can be used
* Cross compiler (supports gcc, clang, msvc) and warning free (W4, Wall, Wextra)
* Dependency-free
* Supports ```std::array, std::vector, std::list, std::deque, std::string, std::unique_ptr, std::shared_ptr, std::optional, std::tuple, std::map, std::set, std::unordered_map, std::unordered_set, std::multiset, std::multimap, std::unordered_multimap, std::unordered_multiset ```
* Supports serialization of user defined types that follow standard container/type conventions
* Supports recursive parsing of types (e.g. containers/pointers of serializable types)
* Supports pretty printing to the console
* Supports printing of the serialized representation via base64 encoding

## Basic Example

```Cpp
#include <cassert>
#include <optional>
#include <tser/tser.hpp>

enum class Item : char { NONE = 0, RADAR = 'R', TRAP = 'T', ORE = 'O' };

struct Point {
    DEFINE_SERIALIZABLE(Point, x, y)
    int x = 0, y = 0;
};

struct Robot {
    DEFINE_SERIALIZABLE(Robot, point, item)
    Point point;
    std::optional<Item> item;
};

int main()
{
    auto robot = Robot{ Point{3,4}, Item::RADAR };
    std::cout << robot; // prints Robot:{point=Point:{x=3, y=4}, item={R}}
    std::cout << Robot(); // prints Robot:{point=Point:{x=0, y=0}, item={null}}

    tser::BinaryArchive ba;
    ba.save(robot);
    std::cout << ba; //prints AwAAAAQAAAABUg to the console via base64 encoding (base64 means only printable characters are used)
    //this way it's quickly possible to log entire objects to the console or logfiles
    tser::BinaryArchive ba2;
    //if we pass the BinaryArchive a string via operator << it will decode it and initialized it's internal buffer with it
    ba2 << "AwAAAAQAAAABUg";
    //so it's basically three lines of code to load an object into a test and start using it
    auto loadedRobot = ba2.load<Robot>();
    //the comparision operators ==,!=,< are implemented, so I could directly use std::set<Robot>
    bool areEqual = (robot == loadedRobot) && !(robot !=loadedRobot) && !(robot < loadedRobot);
    (void)areEqual;
    std::cout << loadedRobot; // prints Robot:{point=Point:{x=3, y=4}, item={R}}
}
```
## Pretty printing example
Datastructures taken from [Cpp Serializer Benchmark](https://github.com/fraillt/cpp_serializers_benchmark/blob/master/testing_core/types.cpp)

```cpp
struct Vec3 {
    DEFINE_SERIALIZABLE(Vec3, x, y, z)
    float x, y, z;
    //the DEFINE_SERIALIZABLE detects custom comparision functions and will only provide the comparision operators
    //that aren't defined (!= is defined in terms of the equality operator !(lhs == rhs))
    friend bool operator==(const Vec3& lhs, const Vec3& rhs){
        constexpr float eps = 1e-6f;
        return abs(lhs.x - rhs.x) < eps && abs(lhs.y - rhs.y) < eps && abs(lhs.y - rhs.y) < eps;
    }
};

struct Weapon {
    DEFINE_SERIALIZABLE(Weapon, name, damage)
    std::string name;
    int16_t damage;
};

struct Monster {
    DEFINE_SERIALIZABLE(Monster, pos, mana, hp, name, inventory, color, weapons, equipped, path)
    Vec3 pos;
    int16_t mana;
    int16_t hp;
    std::string name;
    std::vector<int> inventory;
    Color color;
    std::vector<Weapon> weapons;
    Weapon equipped;
    std::vector<Vec3> path;
};
//see example2.cpp
int main(){
    Monster randomMonster = createRandomMonster();
    std::cout << randomMonster;
//will print something like
//Monster:{pos=Vec3:{x=-0.0304176, y=0.594657, z=-0.185052}, mana=64, hp=408, name=QF, 
//inventory=[9, 8, 1, 2, 10, 6, 8, 10], 
//color=Green, 
//weapons=[Weapon:{name=MKHPC, damage=19086}, Weapon:{name=BQ, damage=24510}, Weapon:{name=VBRM, damage=24933}, Weapon:{name=EZSI, damage=7630}, Weapon:{name=ISKMAZRQMJ, damage=9684}, Weapon:{name=TUBA, damage=27054}, Weapon:{name=OS, damage=14409}], 
//equipped=Weapon:{name=CEAJMS, damage=1203}, 
//path=[Vec3:{x=0.766801, y=0.701376, z=-0.210043}, Vec3:{x=-0.56087, y=-0.841724, z=-0.714148}, Vec3:{x=0.640182, y=0.64889, z=-0.272207}]}

}
```

## How do pointers bevave for serialization?
**tldr:** pointers behave like std::optional. When they are not nullptr the pointed to object is serialized.
```Cpp
struct PointerWrapper
{
    DEFINE_SERIALIZABLE(PointerWrapper, intPtr, unique, shared)
    ~PointerWrapper(){delete intPtr};
    int* intPtr = nullptr;
    std::unique_ptr<Point> unique;
    std::shared_ptr<Point> shared;
};

int main()
{
    BinaryArchive binaryArchive;
    PointerWrapper smartWrapper{new int(5), std::unique_ptr<Point>(new Point{1,2}), std::shared_ptr<Point>(new Point{1,2}) };
    //the content of a raw pointer is serialized, not the address
    binaryArchive.save(&smartWrapper);
    //the serialized layout of shared_ptr<T>, unique_ptr<T>, optional<T> and T* are all the same
    //so if I really wanted to I could load T* as optional
    auto loadedSmartWrapper =  binaryArchive.load<std::optional<PointerWrapper>>();
}
```

## How does it work?

Internally the macro exposes a tuple of the members and the name of the members. It would also be possible to implement this manually for every type.

```cpp
struct Point {
    int x = 0, y = 0;
    //DEFINE_SERIALIZABLE(Point, x, y) expands to:
    constexpr inline decltype(auto) members() const { return std::tie(x, y); }
    constexpr inline decltype(auto) members()       { return std::tie(x, y); }
    static constexpr std::string_view _typeName{"Point"};
    static constexpr std::array<std::string_view, 2> _memberNames{"x", "y"};
};
```
By provoding a function ```members()``` to iterate over the types we can now use SFINAE along with the detection idiom to detect how a type should be serialized.
The printing of types is realized with the static ```_memberNames``` field.

## Integration
tser.hpp is the single required file in the folder single_header/tser. Add the folder folder as include directory or copy it into one of your include directories.
If you really just want to use tser for single file rapid prototyping it's also small enough to just copy the contents of the file tser.hpp on top of the file your working on.

```#include <tser/tser.hpp>```

## Limitations
* Only supports default constructible types
* Is intrusive and uses a macro to be able to reflect over members of a given type
* No safety checks, no versioning, types need the same binary layout on a different platforms
* No support for ```std::variant``` (unless trivially copyable)
* No support for ```std::stack, std::priority_queue, std::string_view```
* Needs a recent compiler (constexpr std::string_view)

## Compiler support
See also https://godbolt.org/z/fmnm7r
* MSVC >= 19.22
* Clang >= 9.0
* Gcc >= 7.3

## Licensed under the [MIT License](LICENSE)
