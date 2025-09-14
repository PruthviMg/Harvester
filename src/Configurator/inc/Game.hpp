#ifndef GAME_HPP
#define GAME_HPP

#include "CommonTypes.hpp"

namespace Harvestor
{
    class Game
    {
    public:
        static sf::RenderWindow window_;

        static void initialize()
        {
            window_.create(sf::VideoMode(Config::WindowWidth, Config::WindowHeight), "Harvestor - SFML Farm Game");
            window_.setVerticalSyncEnabled(false);
            window_.setFramerateLimit(Config::FramerateLimit);
        }
    };
}

#endif