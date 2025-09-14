#include "Manager.hpp"

namespace Harvestor
{
    Manager::Manager()
    {
    }
    Manager::~Manager()
    {
    }

    void Manager::initialize()
    {
        Game::initialize();
        platform_ = std::make_unique<Platform>(Config::Rows, Config::Cols, Config::GridSize);
        soilTesting_.init();
    }

    void Manager::deInitialize() {}

    void Manager::run()
    {
        bool showPhValue = false;
        float lastPhValue = 0.0f;
        sf::Font font;
        bool fontLoaded = font.loadFromFile("images/DejaVuSans.ttf"); // Load font once

        while (Game::window_.isOpen())
        {
            sf::Event event;
            while (Game::window_.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                {
                    Game::window_.close();
                }
                else if (event.type == sf::Event::MouseButtonPressed &&
                         event.mouseButton.button == sf::Mouse::Left)
                {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(Game::window_);
                    soilTesting_.updateHover(mousePos);
                    if (soilTesting_.isHovered())
                    {
                        soilTesting_.toggleSelect();
                        showPhValue = false; // Hide until a cell is clicked
                    }

                    else if (soilTesting_.isSelected())
                    {
                        // Convert mouse position to grid coordinates
                        int row = mousePos.y / Config::GridSize;
                        int col = mousePos.x / Config::GridSize;

                        // Bounds check
                        if (row >= 0 && row < Config::Rows && col >= 0 && col < Config::Cols)
                        {
                            lastPhValue = platform_->getSoilQualityAt(row, col);
                            showPhValue = true;
                            std::cout << "Soil Quality at (" << row << "," << col << "): " << lastPhValue << std::endl;
                        }
                    }
                }
            }

            // Always update hover state (for highlight on hover)
            soilTesting_.updateHover(sf::Mouse::getPosition(Game::window_));

            Game::window_.clear(Colors::Background);

            platform_->draw();
            soilTesting_.draw();

            if (showPhValue && fontLoaded)
            {
                sf::Text phText;
                phText.setFont(font);
                phText.setString("Soil pH: " + std::to_string(lastPhValue));
                phText.setCharacterSize(Config::CharacterSize);
                phText.setFillColor(sf::Color::White);
                phText.setPosition(Config::PhValuePositionX, Config::PhValuePositionY);
                Game::window_.draw(phText);
            }

            Game::window_.display();
        }
    }
} // namespace Harvestor