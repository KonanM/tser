# tser - Tiny Serialization for C++

## Why another C++ serialization library?

I needed a tiny C++ serialization library for some competitive programming contest and didn't find anything that suited my needs. 
The features it had to support:
* serialization of nearly all of the STL containers (array,vector,string,(unordered_) map,set), types (optional, unique_ptr, shared_ptr, tuple), as well as custom containers and types
* support printing the serialized representation of an object to the console via base64 encoding (this way only printable characters are used, allows for easily loading objects into the debugger via strings)
* automatically implement operator std::ostream '<<' (if not user provided) to be able to print a type and all of their members in a human readable format (close to json)
* automatically implement operators  '<', '==', '!=' (if not user provided) - they were needed for some of the containers (e.g. 'map') as well as for prototyping
* only use minimal set of includes (vector,array,string,string_view,type_traits,iostream) and only ~ 350 lines of code
* macro usage to list the members to avoid as much boilerplate as possible

## Features

* C++17
* Single Header (or header only)
* Dependency-free
* supports ```std::array, std::vector, std::unique_ptr, std::shared_ptr, std::optional, std::tuple, std::map, std::set, std::unordered_map, std::unordered_set ```
* supports user defined types that follow standard container conventions

## Example

```Cpp
#include <tser/tser.hpp>

enum class Item : char { NONE = 0, RADAR = 'R', TRAP = 'T', ORE = 'O' };

struct Point {
    DEFINE_SERIALIZABLE(Point, x, y)
    int x = 0, y = 0;
};

struct Robot {
    DEFINE_SERIALIZABLE(Robot, point, item)
    Point point;
    Item item = Item::NONE;
};

int main()
{
    auto robot = Robot{ Point{3,4}, Item::RADAR };
    std::cout << robot; // prints Robot:{ point=Point:{ x=3, y=4}, item=R}

    tser::BinaryArchive ba;
    ba.save(robot);
    std::cout << ba; //prints AwAAAAQAAABSLEQQ to the console via base64 encoding
    ba << "AwAAAAQAAABSLEQQ";

    auto loadedRobot = ba.load<Robot>();
    //the equality operators work as expected
    bool areEqual = (robot == loadedRobot) && !(robot !=loadedRobot) && !(robot < loadedRobot);
    assert(areEqual);
}
```


## Licensed under the [MIT License](LICENSE)
#