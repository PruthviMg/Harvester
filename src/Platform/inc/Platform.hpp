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

        void draw(sf::RenderStates states = sf::RenderStates::Default) const;

        float getSoilQualityAt(const int &row, const int &col);

        void update(const float &dt);

        Grid &getGrid(int row, int col);

    private:
        std::vector<std::vector<Grid>> grids_;
    };
}

#endif