#ifndef PLOUGH_HPP
#define PLOUGH_HPP

#include "Game.hpp"

namespace Harvestor
{
    class Plough
    {
    public:
        Plough() = default;
        ~Plough() = default;

        void init()
        {
            if (ploughTexture_.loadFromFile("resources/plough_icon.png"))
            {
                ploughIcon_.setTexture(ploughTexture_);
                ploughIcon_.setPosition(Config::PloughPositionX, Config::PloughPositionY);
                ploughIcon_.setScale(Config::PloughIconSize, Config::PloughIconSize);
            }
            else
            {
                std::cerr << "Error loading plough icon!" << std::endl;
            }
        }

        void deInit() {}

        void updateHover(sf::Vector2i mousePos)
        {
            hovered_ = ploughIcon_.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
        }

        void toggleSelect()
        {
            if (hovered_)
                selected_ = !selected_;
        }

        void draw()
        {
            if (selected_)
                ploughIcon_.setColor(Colors::StrongHighlight);
            else if (hovered_)
                ploughIcon_.setColor(Colors::LightHighlight);
            else
                ploughIcon_.setColor(sf::Color::White);

            Game::window_.draw(ploughIcon_);

            if (selected_)
            {
                sf::FloatRect bounds = ploughIcon_.getGlobalBounds();
                sf::RectangleShape border(sf::Vector2f(bounds.width, bounds.height));
                border.setPosition(bounds.left, bounds.top);
                border.setFillColor(sf::Color::Transparent);
                border.setOutlineColor(sf::Color::Yellow);
                border.setOutlineThickness(Config::OutlineThickness);
                Game::window_.draw(border);
            }
        }

        bool isHovered() const { return hovered_; }
        bool isSelected() const { return selected_; }

    private:
        sf::Sprite ploughIcon_;
        sf::Texture ploughTexture_;
        bool hovered_ = false;
        bool selected_ = false;
    };
} // namespace Harvestor

#endif