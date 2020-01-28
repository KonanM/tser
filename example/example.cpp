// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
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
    std::cout << ba; //prints AwAAAAQAAAABUg== to the console via base64 encoding (base64 means only printable characters are used)
    //this way it's quickly possible to log entire objects to the console or logfiles
    tser::BinaryArchive ba2;
    //if we pass the BinaryArchive a string via operator << it will decode it and initialized it's internal buffer with it
    ba2 << "AwAAAAQAAAABUg==";
    //so it's basically three lines of code to load an object into a test and start using it
    auto loadedRobot = ba2.load<Robot>();
    //all the comparision operator are implemented, so I could directly use std::set<Robot>
    bool areEqual = (robot == loadedRobot) && !(robot !=loadedRobot) && !(robot < loadedRobot);
    (void)areEqual;
    std::cout << loadedRobot; // prints Robot:{point=Point:{x=3, y=4}, item={R}}
}

