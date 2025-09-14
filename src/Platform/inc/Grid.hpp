#ifndef GRID_HPP
#define GRID_HPP

#include "CommonTypes.hpp"
#include <cmath>

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

        void startAnimation()
        {
            animating_ = true;
            animTime_ = 0.f;
        }

        void update(float dt)
        {
            if (animating_)
            {
                animTime_ += dt;
                float pulse = 0.5f + 0.5f * std::sin(animTime_ * 6.0f); // Fast pulse
                setFillColor(sf::Color(Colors::Soil.r * pulse, Colors::Soil.g * pulse, Colors::Soil.b * pulse));
                if (animTime_ > 0.5f) // Animation lasts 0.5s
                {
                    animating_ = false;
                    setFillColor(Colors::Soil);
                }
            }
        }

    private:
        float quality_;
        // Animation state
        bool animating_ = false;
        float animTime_ = 0.f;
    };
}

#endif