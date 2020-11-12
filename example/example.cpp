// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
#include <cassert>
#include <optional>
#include <tser/tser.hpp>

enum class Item : char { NONE = 0, RADAR = 'R', TRAP = 'T', ORE = 'O' };
namespace x
{
    struct Point {
        DEFINE_SERIALIZABLE(Point,x,y)
        int x = 0, y = 0;
    };
}
struct Robot {
    DEFINE_SERIALIZABLE(Robot,point,item)
    x::Point point;
    std::optional<Item> item;
};

int main()
{
    auto robot = Robot{ x::Point{3,4}, Item::RADAR };
    std::cout << robot << '\n'; // prints { "Robot": {"point" : { "Point": {"x" : 3, "y" : 4}}, "item" : R}}
    std::cout << Robot() << '\n'; // prints { "Robot": {"point" : { "Point": {"x" : 3, "y" : 4}}, "item" : {null}}}
    tser::BinaryArchive ba;
    ba.save(robot);
    std::cout << ba; //prints BggBUg to the console via base64 encoding (base64 means only printable characters are used)
    //this way it's quickly possible to log entire objects to the console or logfiles

    //due to varint encoding only 6 printable characters are needed, although the struct is 12 bytes in size
    //e.g. a size_t in the range of 0-127 will only take 1 byte (before the base 64 encoding)
}

void test()
{
    //if we construct BinaryArchive with a string it will decode it and initialized it's internal buffer with it
    //so it's basically one or two lines of code to load a complex object into a test and start using it
    tser::BinaryArchive ba2("BggBUg");
    auto loadedRobot = ba2.load<Robot>();

    auto robot = Robot{ x::Point{3,4}, Item::RADAR };
    //all the comparision operators are implemented, so I could directly use std::set<Robot>
    bool areEqual = (robot == loadedRobot) && !(robot != loadedRobot) && !(robot < loadedRobot);
    (void)areEqual;
    std::cout << loadedRobot; //prints{ "Robot": {"point" : { "Point": {"x" : 3, "y" : 4}}, "item" : R} }
}
