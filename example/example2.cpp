// Licensed under the MIT License <http://opensource.org/licenses/MIT>.
// SPDX-License-Identifier: MIT
#include <cassert>
#include <vector>
#include <tser/serialize.hpp>
#include <string_view>
#include <type_traits>
#include <cassert>
#include <random>
#include <functional>
#include <iostream>



//MIT License
//Copyright (c) 2017 Mindaugas Vinkelis
//taken from https://github.com/fraillt/cpp_serializers_benchmark/blob/master/testing_core/types.cpp
namespace cpp_serializers_benchmark
{
    enum class Color : uint8_t { Red, Green, Blue };

    std::ostream& operator <<(std::ostream& os, const Color& c) {
        return os << (c == Color::Blue ? "Blue" : c == Color::Green ? "Green" : "Blue");
    }

    using namespace::tser;
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
    struct random_char_dist : std::uniform_int_distribution<int16_t>
    {
        using uniform_int_distribution::uniform_int_distribution;
        template<typename Engine>
        char operator()(Engine& eng) { return static_cast<char>(uniform_int_distribution::operator()(eng)); }
    };

    static random_char_dist rand_char('A', 'Z');
    static std::uniform_int_distribution<> rand_len(1, 10);
    static std::uniform_int_distribution<int16_t> rand_nr(0);
    static std::uniform_real_distribution<float> rand_float(-1.0f, 1.0f);

    typedef std::mersenne_twister_engine<uint_fast32_t, 32, 624, 397, 31, 0x9908b0dfUL, 11, 0xffffffffUL, 7, 0x9d2c5680UL, 15, 0xefc60000UL, 18, 1812433253UL> engine;


    static Weapon createRandomWeapon(engine& e) {
        Weapon res;
        res.damage = rand_nr(e);
        std::generate_n(std::back_inserter(res.name), rand_len(e), std::bind(rand_char, std::ref(e)));
        return res;
    }

    Monster createRandomMonster(engine& e) {
        Monster res{};
        std::generate_n(std::back_inserter(res.name), rand_len(e), std::bind(rand_char, std::ref(e)));

        res.pos.x = rand_float(e);
        res.pos.y = rand_float(e);
        res.pos.z = rand_float(e);
        res.color = static_cast<Color>(rand_len(e) % static_cast<int>(3));
        res.hp = rand_nr(e) % 1000;
        res.mana = rand_nr(e) % 500;
        static_assert(std::is_copy_constructible<engine>::value, "");
        std::generate_n(std::back_inserter(res.inventory), rand_len(e), std::bind(rand_len, std::ref(e)));
        std::generate_n(std::back_inserter(res.path), rand_len(e), [&]() {
            return Vec3{ rand_float(e), rand_float(e), rand_float(e) };
            });
        res.equipped = createRandomWeapon(e);
        std::generate_n(std::back_inserter(res.weapons), rand_len(e), std::bind(createRandomWeapon, std::ref(e)));
        return res;
    }

    std::vector<Monster> createMonsters(size_t count) {
        std::vector<Monster> res{};
        //always the same seed
        std::seed_seq seed{ 1,2,3 };
        engine e{ seed };

        std::generate_n(std::back_inserter(res), count, std::bind(createRandomMonster, std::ref(e)));
        return res;
    }
}
#include <vector>
#include <tser/compare.hpp>
#include <tser/print.hpp>
#include <algorithm>
int main()
{
    tser::BinaryArchive ba;
    auto allTheMonsters = cpp_serializers_benchmark::createMonsters(20);
    ba & allTheMonsters;
    for (auto& m : allTheMonsters)
    {
        std::cout << m << "\n";
    }

    auto allTheLoadedMonsters =  ba.load<std::vector<cpp_serializers_benchmark::Monster>>();
    assert(allTheMonsters == allTheLoadedMonsters);
}

