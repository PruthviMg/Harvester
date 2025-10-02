#include "farmscene.hpp"

using namespace Harvestor;

// ---------------- Main ----------------
int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    // Get screen resolution
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    float width = desktop.width;
    float height = desktop.height;

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
            if (event.type == sf::Event::Closed) window.close();

            if (event.type == sf::Event::MouseButtonPressed) farm.handleClick(sf::Mouse::getPosition(window));

            // Scroll handling for crop dropdown
            if (event.type == sf::Event::MouseWheelScrolled && farm.cropDropdownActive) {
                if (event.mouseWheelScroll.delta > 0 && farm.cropScrollOffset > 0) {
                    farm.cropScrollOffset--;
                }
                if (event.mouseWheelScroll.delta < 0 && farm.cropScrollOffset + Config::maxVisibleCrops < farm.cropTypes.size()) {
                    farm.cropScrollOffset++;
                }
            }

            // Scroll handling for layout dropdown
            if (event.type == sf::Event::MouseWheelScrolled && farm.layoutDropdownActive) {
                if (event.mouseWheelScroll.delta > 0 && farm.layoutScrollOffset > 0) {
                    farm.layoutScrollOffset--;
                }
                if (event.mouseWheelScroll.delta < 0 && farm.layoutScrollOffset + Config::maxVisibleLayouts < (int)farm.layouts.size()) {
                    farm.layoutScrollOffset++;
                }
            }
            // Keyboard events
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::S) {
                    farm.writeSimulationOutput("simulation_output.csv");  // Save results
                }
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();  // Exit
                }
            }
        }

        // Update simulation
        float dt = clock.restart().asSeconds();
        farm.update(dt);

        // Render
        window.clear(sf::Color(50, 50, 50));
        farm.draw();
        window.display();
    }

    return 0;
}