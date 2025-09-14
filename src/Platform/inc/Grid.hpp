#ifndef GRID_HPP
#define GRID_HPP

#include "CommonTypes.hpp"

namespace Harvestor
{
    class Grid : public sf::RectangleShape
    {
    public:
        Grid(const sf::Vector2f &position, const size_t &size)
            : sf::RectangleShape(sf::Vector2f(size, size))
        {
            setPosition(position);
            setFillColor(Colors::Soil);
            setOutlineColor(Colors::FarmPlot);
            setOutlineThickness(1.f);

            // Need to generate random quality between 0.0 and 1.0
            quality_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        }

        float getQuality()
        {
            return quality_;
        }

    private:
        float quality_;
    };
}

#endif