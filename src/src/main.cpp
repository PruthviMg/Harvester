#include "farmscene.hpp"

using namespace Harvestor;

// ---------------- Main ----------------
int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    // Get screen resolution
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    float width = desktop.width;
    float height = desktop.height;

    Config::uiPanelWidth = width * 0.15f;  // 15% for UI
    Config::grassTileSize = std::min((width - Config::uiPanelWidth) / Config::cols, height / Config::rows);
    Config::landTileSize = Config::grassTileSize / 5.f;
    Config::cropButtonWidth = Config::uiPanelWidth - 20.f;

    sf::RenderWindow window(sf::VideoMode((unsigned)width, (unsigned)height), "Farm Simulation");
    window.setFramerateLimit(60);

    sf::Texture grassTexture;
    if (!grassTexture.loadFromFile("textures/grass.png")) {
        std::cerr << "Failed to load grass.png\n";
        return -1;
    }

    GrassManager grassManager(grassTexture, Config::grassTileSize);
    FarmScene farm(window, grassManager, width, height);

    sf::Clock clock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::MouseButtonPressed) farm.handleClick(sf::Mouse::getPosition(window));
            if (event.type == sf::Event::MouseWheelScrolled && farm.cropDropdownActive) {
                if (event.mouseWheelScroll.delta > 0 && farm.cropScrollOffset > 0) {
                    farm.cropScrollOffset--;
                }
                if (event.mouseWheelScroll.delta < 0 && farm.cropScrollOffset + Config::maxVisibleCrops < farm.cropTypes.size()) {
                    farm.cropScrollOffset++;
                }
            }
            // Press S to save simulation data
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::S) {
                    farm.writeSimulationOutput("simulation_output.csv");
                }
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();  // Close the window when ESC is pressed
                }
            }
        }

        float dt = clock.restart().asSeconds();
        farm.update(dt);

        window.clear(sf::Color(50, 50, 50));
        farm.draw();
        window.display();
    }

    return 0;
}