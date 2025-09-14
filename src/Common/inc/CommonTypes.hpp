#ifndef COMMON_TYPES_HPP
#define COMMON_TYPES_HPP

#include <SFML/Graphics.hpp>
#include <iostream>

namespace Harvestor
{
    namespace Colors
    {
        const sf::Color Soil(139, 69, 19);         // Brown color for soil
        const sf::Color Water(0, 191, 255);        // Deep sky blue for water
        const sf::Color Crop(34, 139, 34);         // Forest green for crops
        const sf::Color Background(100, 149, 237); // Cornflower blue for background
        const sf::Color FarmPlot(100, 149, 237);   // Green color for farm plot

        // Soil sensor
        const sf::Color StrongHighlight(255, 200, 50);
        const sf::Color LightHighlight(255, 255, 180);
    }

    namespace Config
    {
        constexpr auto WindowWidth = 800;
        constexpr auto WindowHeight = 800;
        constexpr auto FramerateLimit = 60;

        // platform
        constexpr auto GridSize = 50;
        constexpr auto Rows = 12;
        constexpr auto Cols = 12;

        // Soil sensor
        constexpr auto SoilPositionX = WindowWidth - 110;
        constexpr auto SoilPositionY = 10;
        constexpr auto SoilIconSize = 0.5f;
        constexpr auto PhValuePositionX = WindowWidth - 500;
        constexpr auto PhValuePositionY = WindowHeight - 50;
        constexpr auto OutlineThickness = 3.0f;
        constexpr auto CharacterSize = 24;

        // Plough
        constexpr auto PloughPositionX = WindowWidth - 110;
        constexpr auto PloughPositionY = 150;
        constexpr auto PloughIconSize = 0.5f;
    }
}

#endif