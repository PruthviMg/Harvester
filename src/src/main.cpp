#include "farmscene.hpp"

using namespace Harvestor;

// ---------------- Main ----------------
int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    // Get screen resolution
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    float width = (float)desktop.width;
    float height = (float)desktop.height;

    // Config setup
    Config::uiPanelWidth = width * 0.15f;  // 15% for UI
    Config::grassTileSize = std::min((width - Config::uiPanelWidth) / Config::cols, height / Config::rows);
    Config::landTileSize = Config::grassTileSize / 5.f;
    Config::cropButtonWidth = Config::uiPanelWidth - 20.f;

    // Create window
    sf::RenderWindow window(sf::VideoMode((unsigned)width, (unsigned)height), "Farm Simulation");
    window.setFramerateLimit(60);

    // Load grass texture
    sf::Texture grassTexture;
    if (!grassTexture.loadFromFile("textures/grass.png")) {
        std::cerr << "Failed to load textures/grass.png\n";
        return -1;
    }

    // Initialize managers
    GrassManager grassManager(grassTexture, Config::grassTileSize);
    FarmScene farm(window, grassManager, width, height);

    // Clock for delta time
    sf::Clock clock;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            // Window close
            if (event.type == sf::Event::Closed) window.close();

            // Mouse pressed
            else if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Right) {
                } else if (event.mouseButton.button == sf::Mouse::Left) {
                    farm.handleMousePressed(sf::Mouse::getPosition(window));
                    farm.handleClick(sf::Mouse::getPosition(window));
                }
            }

            else if (event.type == sf::Event::MouseButtonReleased) {
                farm.handleMouseReleased(sf::Mouse::getPosition(window));
            }

            else if (event.type == sf::Event::MouseMoved) {
                farm.handleMouseMoved(sf::Mouse::getPosition(window));
            }

            // Mouse wheel scroll (dropdowns)
            else if (event.type == sf::Event::MouseWheelScrolled) {
                farm.handleMouseWheelScroll(event.mouseWheelScroll.delta);
            }

            // Keyboard events
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) window.close();  // Exit

                if (event.key.code == sf::Keyboard::C) {
                    farm.clearSelection();
                }
            }
        }

        // Update simulation
        float dt = clock.restart().asSeconds();
        farm.update(dt);

        // Render
        window.clear(sf::Color(50, 50, 50));
        farm.draw();  // Draw farm & UI
        window.display();
    }

    return 0;
}