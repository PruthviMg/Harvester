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
        plough_.init();
    }

    void Manager::deInitialize()
    {
        soilTesting_.deInit();
        plough_.deInit();
    }

    void Manager::run()
    {
        bool showPhValue = false;
        float lastPhValue = 0.0f;
        sf::Font font;
        bool fontLoaded = font.loadFromFile("resources/DejaVuSans.ttf");

        sf::Clock clock;

        while (Game::window_.isOpen())
        {
            float deltaTime = clock.restart().asSeconds();

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
                    plough_.updateHover(mousePos);

                    // Toggle soil testing tool
                    if (soilTesting_.isHovered())
                    {
                        soilTesting_.toggleSelect();
                        showPhValue = false;
                    }

                    // Toggle plough tool
                    if (plough_.isHovered())
                    {
                        plough_.toggleSelect();
                    }

                    // If soil testing tool is selected, test soil on grid click
                    if (soilTesting_.isSelected())
                    {
                        // Convert mouse position to grid coordinates
                        int col = mousePos.x / Config::GridSize;
                        int row = mousePos.y / Config::GridSize;
                        if (row >= 0 && row < Config::Rows && col >= 0 && col < Config::Cols)
                        {
                            lastPhValue = platform_->getSoilQualityAt(row, col);
                            showPhValue = true;
                        }
                    }
                    // If plough tool is selected, plough the grid and animate
                    else if (plough_.isSelected())
                    {
                        int col = mousePos.x / Config::GridSize;
                        int row = mousePos.y / Config::GridSize;
                        if (row >= 0 && row < Config::Rows && col >= 0 && col < Config::Cols)
                        {
                            // Animate the grid cell
                            platform_->getGrid(row, col).startAnimation();
                        }
                    }
                }
            }

            // Always update hover state for highlight on hover
            soilTesting_.updateHover(sf::Mouse::getPosition(Game::window_));
            plough_.updateHover(sf::Mouse::getPosition(Game::window_));

            // Update platform (for grid animations)
            platform_->update(deltaTime);

            Game::window_.clear(Colors::Background);

            platform_->draw();
            soilTesting_.draw();
            plough_.draw();

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