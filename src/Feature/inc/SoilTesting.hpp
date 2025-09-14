#ifndef SOIL_TESTING_HPP
#define SOIL_TESTING_HPP

#include "Game.hpp"

namespace Harvestor
{
    class SoilTesting
    {
    public:
        SoilTesting() = default;
        ~SoilTesting() = default;

        void init()
        {
            if (soilSensorTexture_.loadFromFile("images/soil_sensor_icon.png"))
            {
                soilSensorIcon_.setTexture(soilSensorTexture_);
                soilSensorIcon_.setPosition(Config::SoilPositionX, Config::SoilPositionY);
                soilSensorIcon_.setScale(Config::SoilIconSize, Config::SoilIconSize);
            }
            else
            {
                std::cerr << "Error loading soil sensor icon!" << std::endl;
            }
        }

        void deInit() {}

        // Call this on mouse move to update hover state
        void updateHover(sf::Vector2i mousePos)
        {
            hovered_ = soilSensorIcon_.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
        }

        // Call this on click to toggle selection
        void toggleSelect()
        {
            if (hovered_)
                selected_ = !selected_;
        }

        void draw()
        {
            if (selected_)
                soilSensorIcon_.setColor(Colors::StrongHighlight); // Strong highlight for selected
            else if (hovered_)
                soilSensorIcon_.setColor(Colors::LightHighlight); // Light highlight for hover
            else
                soilSensorIcon_.setColor(Colors::White); // Normal

            Game::window_.draw(soilSensorIcon_);

            // Optional: Draw a border if selected
            if (selected_)
            {
                sf::FloatRect bounds = soilSensorIcon_.getGlobalBounds();
                sf::RectangleShape border(sf::Vector2f(bounds.width, bounds.height));
                border.setPosition(bounds.left, bounds.top);
                border.setFillColor(sf::Color::Transparent);
                border.setOutlineColor(sf::Color::Yellow);
                border.setOutlineThickness(3.f);
                Game::window_.draw(border);
            }
        }

        bool isHovered() const { return hovered_; }
        bool isSelected() const { return selected_; }

    private:
        sf::Sprite soilSensorIcon_;
        sf::Texture soilSensorTexture_;
        bool hovered_ = false;
        bool selected_ = false;
    };
} // namespace Harvestor

#endif