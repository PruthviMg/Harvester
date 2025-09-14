#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include "Grid.hpp"
#include <vector>
#include "Game.hpp"

namespace Harvestor
{
    class Platform
    {
    public:
        Platform(const size_t &rows, const size_t &cols, const float &gridSize);

        ~Platform();

        const std::vector<std::vector<Grid>> &getGrids() const;

        void draw(sf::RenderStates states = sf::RenderStates::Default) const;

        float getSoilQualityAt(const int &row, const int &col);

    private:
        std::vector<std::vector<Grid>> grids_;
    };
}

#endif