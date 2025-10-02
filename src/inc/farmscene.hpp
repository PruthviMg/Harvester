#ifndef FARM_SCENE_HPP_
#define FARM_SCENE_HPP_

#include "common.hpp"
#include "grassManager.hpp"
#include "loader.hpp"

namespace Harvestor {
// ---------------- FarmScene ----------------
class FarmScene {
    sf::RenderWindow &window;
    GrassManager &grassManager;
    float width, height;
    sf::Font font;
    sf::Clock simClock;

   public:
    std::vector<Land> lands;
    std::vector<Pond> ponds;
    std::vector<CropType> cropTypes;
    int selectedCropIndex = 0;
    bool simulate = false;
    bool cropDropdownActive = false;
    int cropScrollOffset = 0;

    // Rain
    bool rainActive = false;
    sf::Clock rainClock;
    std::vector<RainDrop> raindrops;
    std::vector<Splash> splashes;
    std::vector<Ripple> ripples;

    FarmScene(sf::RenderWindow &win, GrassManager &gm, float w, float h) : window(win), grassManager(gm), width(w), height(h) {
        if (!font.loadFromFile(Config::fontPath)) std::cerr << "Failed to load font\n";

        cropTypes = CropLoader::loadFromFile(Config::cropsFile);
    }

    void generateFromFile(const std::string &filePath) {
        lands.clear();
        ponds.clear();
        FarmLoader::loadFromFile(filePath, lands, ponds, cropTypes, selectedCropIndex);

        grassManager.generate(width, height);
        simClock.restart();
    }

    void startRain() {
        rainActive = true;
        rainClock.restart();
        raindrops.clear();
        splashes.clear();
        for (int i = 0; i < Config::numRaindrops; i++) {
            RainDrop rd;
            rd.position = sf::Vector2f(Config::uiPanelWidth + rand() % int(width - Config::uiPanelWidth), rand() % int(height));
            rd.speed = 200.f + rand() % 150;
            raindrops.push_back(rd);
        }
    }

    void update(float dt) {
        float simTime = simClock.getElapsedTime().asSeconds();
        for (auto &land : lands) land.updateGrowth(dt, ponds, cropTypes[selectedCropIndex], simulate, rainActive, simTime);

        if (rainActive) {
            float elapsed = rainClock.getElapsedTime().asSeconds();
            if (elapsed > Config::rainDuration) {
                rainActive = false;
                raindrops.clear();
            } else {
                // Boost crop growth
                for (auto &land : lands) {
                    for (auto &tile : land.tiles) {
                        if (tile.hasCrop) {
                            // Increase water level due to rain
                            tile.waterLevel += dt * 0.3f;  // adjust 0.3f as rain intensity
                            tile.waterLevel = std::clamp(tile.waterLevel, 0.f, 1.f);

                            // Boost growth
                            tile.crop.growth += dt * Config::rainGrowthBoost;
                            if (tile.crop.growth > 1.f) tile.crop.growth = 1.f;
                        }
                    }
                }

                // Update raindrops
                for (auto &rd : raindrops) {
                    rd.position.y += rd.speed * dt;

                    bool hitPond = false;
                    for (auto &pond : ponds) {
                        float dx = rd.position.x - pond.center.x;
                        float dy = rd.position.y - pond.center.y;
                        float dist = std::sqrt(dx * dx + dy * dy);
                        if (dist < pond.radius) {
                            hitPond = true;
                            Splash s;
                            s.position = rd.position;
                            splashes.push_back(s);

                            Ripple r;
                            r.position = rd.position;
                            ripples.push_back(r);

                            break;
                        }
                    }

                    if (rd.position.y > height || hitPond) {
                        rd.position.y = 0.f;
                        rd.position.x = Config::uiPanelWidth + rand() % int(width - Config::uiPanelWidth);
                    }
                }
            }
        }

        // Update splashes
        for (size_t i = 0; i < splashes.size();) {
            if (!splashes[i].update(dt))
                splashes.erase(splashes.begin() + i);
            else
                ++i;
        }

        for (size_t i = 0; i < ripples.size();) {
            if (!ripples[i].update(dt))
                ripples.erase(ripples.begin() + i);
            else
                ++i;
        }
    }

    void draw() {
        sf::RectangleShape uiPanel(sf::Vector2f(Config::uiPanelWidth, height));
        uiPanel.setFillColor(sf::Color(70, 70, 70));
        window.draw(uiPanel);

        grassManager.draw(window);
        for (auto &land : lands) land.draw(window, cropTypes[selectedCropIndex]);
        for (auto &pond : ponds) pond.draw(window);

        // Draw raindrops
        if (rainActive) {
            for (auto &rd : raindrops) {
                sf::RectangleShape drop(sf::Vector2f(2.f, 8.f));  // thickness, length
                drop.setFillColor(sf::Color(0, 150, 255));
                drop.setPosition(rd.position);
                window.draw(drop);
            }

            for (auto &s : splashes) s.draw(window);

            for (auto &r : ripples) r.draw(window);
        }

        drawUI();
        drawSimulationInfo();
    }

    void centerTextInButton(sf::Text &text, const sf::RectangleShape &button) {
        sf::FloatRect textBounds = text.getLocalBounds();  // get text size
        text.setOrigin(textBounds.left + textBounds.width / 2.f,
                       textBounds.top + textBounds.height / 2.f);  // set origin to center
        text.setPosition(button.getPosition().x + button.getSize().x / 2.f, button.getPosition().y + button.getSize().y / 2.f);
    }

    void drawSimulationInfo() {
        if (!simulate) return;

        // Simulation time
        float elapsed = simClock.getElapsedTime().asSeconds();
        std::stringstream ss;
        ss << "Time: " << std::fixed << std::setprecision(1) << elapsed << "s";

        sf::Text txt(ss.str(), font, 16);
        txt.setPosition(10, height - 60);
        txt.setFillColor(sf::Color::White);
        window.draw(txt);

        // Crop growth percentage per land
        for (int i = 0; i < lands.size(); i++) {
            float pct = lands[i].getCropGrowthPercentage();
            std::stringstream ssp;
            ssp << "Land " << i + 1 << ": " << std::fixed << std::setprecision(0) << pct << "%";

            sf::Text landTxt(ssp.str(), font, 14);
            landTxt.setPosition(lands[i].center.x - lands[i].radius / 2, lands[i].center.y - lands[i].radius - 20);
            landTxt.setFillColor(sf::Color::White);
            window.draw(landTxt);
        }
    }

    void drawUI() {
        // Main crop dropdown button
        sf::RectangleShape mainBtn(sf::Vector2f(Config::cropButtonWidth, Config::cropButtonHeight));
        mainBtn.setPosition(10, 10);
        mainBtn.setFillColor(Config::cropButtonColor);
        window.draw(mainBtn);

        sf::Text mainTxt(cropTypes[selectedCropIndex].name, font, 16);
        mainTxt.setPosition(15, 15);
        mainTxt.setFillColor(sf::Color::Black);
        window.draw(mainTxt);

        // Dropdown arrow (triangle)
        sf::ConvexShape arrow;
        arrow.setPointCount(3);
        float arrowSize = 8.f;
        float arrowX = 10 + Config::cropButtonWidth - 15;  // right side of button
        float arrowY = 10 + Config::cropButtonHeight / 2.f;

        // Draw dropdown items if active
        if (cropDropdownActive) {
            // Upward triangle
            arrow.setPoint(0, sf::Vector2f(arrowX - arrowSize, arrowY + arrowSize / 2));
            arrow.setPoint(1, sf::Vector2f(arrowX + arrowSize, arrowY + arrowSize / 2));
            arrow.setPoint(2, sf::Vector2f(arrowX, arrowY - arrowSize / 2));

            int visibleCount = std::min(Config::maxVisibleCrops, (int)cropTypes.size());
            for (int i = 0; i < visibleCount; i++) {
                int cropIndex = cropScrollOffset + i;
                if (cropIndex >= cropTypes.size()) break;

                sf::RectangleShape itemBtn(sf::Vector2f(Config::cropButtonWidth, Config::cropButtonHeight));
                itemBtn.setPosition(10, 10 + (i + 1) * Config::cropButtonHeight);
                itemBtn.setFillColor(cropIndex == selectedCropIndex ? Config::cropButtonSelectedColor : Config::cropButtonColor);
                window.draw(itemBtn);

                sf::Text itemTxt(cropTypes[cropIndex].name, font, 16);
                itemTxt.setPosition(15, 15 + (i + 1) * Config::cropButtonHeight);
                itemTxt.setFillColor(sf::Color::Black);
                window.draw(itemTxt);
            }

            // Draw scroll indicators if needed
            if (cropScrollOffset > 0) {
                sf::Text upTxt("▲", font, 14);
                upTxt.setPosition(10 + Config::cropButtonWidth + 5, 10 + Config::cropButtonHeight);
                upTxt.setFillColor(sf::Color::Black);
                window.draw(upTxt);
            }
            if (cropScrollOffset + visibleCount < cropTypes.size()) {
                sf::Text downTxt("▼", font, 14);
                downTxt.setPosition(10 + Config::cropButtonWidth + 5, 10 + visibleCount * Config::cropButtonHeight);
                downTxt.setFillColor(sf::Color::Black);
                window.draw(downTxt);
            }
        } else {
            // Downward triangle
            arrow.setPoint(0, sf::Vector2f(arrowX - arrowSize, arrowY - arrowSize / 2));
            arrow.setPoint(1, sf::Vector2f(arrowX + arrowSize, arrowY - arrowSize / 2));
            arrow.setPoint(2, sf::Vector2f(arrowX, arrowY + arrowSize / 2));
        }

        arrow.setFillColor(sf::Color::Black);
        window.draw(arrow);

        float startY = Config::ButtonPadding + (cropDropdownActive ? (cropTypes.size() + 1) * Config::cropButtonHeight : Config::cropButtonHeight);

        float bigButtonHeight = Config::cropButtonHeight * Config::bigButtonHeightMultiplier;

        // Simulate button
        sf::RectangleShape simBtn(sf::Vector2f(Config::cropButtonWidth, bigButtonHeight));
        simBtn.setPosition(Config::uiPadding, startY);
        simBtn.setFillColor(simulate ? Config::simulateButtonActiveColor : Config::simulateButtonColor);
        window.draw(simBtn);

        sf::Text simTxt(simulate ? "Simulating" : "Start", font, Config::bigButtonFontSize);
        centerTextInButton(simTxt, simBtn);
        window.draw(simTxt);

        // Reset button
        sf::RectangleShape resetBtn(sf::Vector2f(Config::cropButtonWidth, bigButtonHeight));
        resetBtn.setPosition(Config::uiPadding, startY + bigButtonHeight + Config::cropButtonSpacing);
        resetBtn.setFillColor(Config::resetButtonColor);
        window.draw(resetBtn);

        sf::Text resetTxt("Reset", font, Config::bigButtonFontSize);
        centerTextInButton(resetTxt, resetBtn);
        window.draw(resetTxt);

        // Rain button
        sf::RectangleShape rainBtn(sf::Vector2f(Config::cropButtonWidth, bigButtonHeight));
        rainBtn.setPosition(Config::uiPadding, startY + 2 * bigButtonHeight + 2 * Config::cropButtonSpacing);
        rainBtn.setFillColor(rainActive ? Config::simulateButtonActiveColor : Config::rainButtonColor);
        window.draw(rainBtn);

        sf::Text rainTxt("Rain", font, Config::bigButtonFontSize);
        centerTextInButton(rainTxt, rainBtn);
        window.draw(rainTxt);

        // Load Layout button
        sf::RectangleShape loadBtn(sf::Vector2f(Config::cropButtonWidth, bigButtonHeight));
        loadBtn.setPosition(Config::uiPadding, startY + 3 * bigButtonHeight + 3 * Config::cropButtonSpacing);
        loadBtn.setFillColor(sf::Color(150, 150, 250));
        window.draw(loadBtn);

        sf::Text loadTxt("Load Layout", font, Config::bigButtonFontSize);
        centerTextInButton(loadTxt, loadBtn);
        window.draw(loadTxt);
    }

    void handleClick(sf::Vector2i mousePos) {
        // Main crop dropdown button
        sf::FloatRect mainBtn(10, 10, Config::cropButtonWidth, Config::cropButtonHeight);
        if (mainBtn.contains(mousePos.x, mousePos.y)) {
            cropDropdownActive = !cropDropdownActive;
            return;  // stop further click processing
        }

        // Dropdown items (only if open)
        if (cropDropdownActive) {
            int visibleCount = std::min(Config::maxVisibleCrops, (int)cropTypes.size());
            for (int i = 0; i < visibleCount; i++) {
                int cropIndex = cropScrollOffset + i;
                if (cropIndex >= cropTypes.size()) break;

                sf::FloatRect itemRect(10, 10 + (i + 1) * Config::cropButtonHeight, Config::cropButtonWidth, Config::cropButtonHeight);
                if (itemRect.contains(mousePos.x, mousePos.y)) {
                    selectedCropIndex = cropIndex;
                    cropDropdownActive = false;
                    return;
                }
            }

            // Check scroll arrows
            sf::FloatRect upRect(10 + Config::cropButtonWidth + 5, 10 + Config::cropButtonHeight, 20, 20);
            sf::FloatRect downRect(10 + Config::cropButtonWidth + 5, 10 + visibleCount * Config::cropButtonHeight, 20, 20);

            if (upRect.contains(mousePos.x, mousePos.y) && cropScrollOffset > 0) {
                cropScrollOffset--;
                return;
            }
            if (downRect.contains(mousePos.x, mousePos.y) && cropScrollOffset + visibleCount < cropTypes.size()) {
                cropScrollOffset++;
                return;
            }

            cropDropdownActive = false;  // click outside closes
        }

        float startY = Config::ButtonPadding + (cropDropdownActive ? (cropTypes.size() + 1) * Config::cropButtonHeight : Config::cropButtonHeight);

        float bigButtonHeight = Config::cropButtonHeight * Config::bigButtonHeightMultiplier;

        sf::FloatRect simBtnRect(Config::uiPadding, startY, Config::cropButtonWidth, bigButtonHeight);
        sf::FloatRect resetBtnRect(Config::uiPadding, startY + bigButtonHeight + Config::cropButtonSpacing, Config::cropButtonWidth, bigButtonHeight);
        sf::FloatRect rainBtnRect(Config::uiPadding, startY + 2 * bigButtonHeight + 2 * Config::cropButtonSpacing, Config::cropButtonWidth,
                                  bigButtonHeight);
        sf::FloatRect loadBtnRect(Config::uiPadding, startY + 3 * bigButtonHeight + 3 * Config::cropButtonSpacing, Config::cropButtonWidth,
                                  bigButtonHeight);

        if (loadBtnRect.contains(mousePos.x, mousePos.y)) {
            generateFromFile(Config::layoutFile);  // reload from file
            return;
        }

        if (simBtnRect.contains(mousePos.x, mousePos.y)) {
            simulate = !simulate;
            return;
        }
        if (resetBtnRect.contains(mousePos.x, mousePos.y)) {
            reset();
            return;
        }
        if (rainBtnRect.contains(mousePos.x, mousePos.y)) {
            startRain();
            return;
        }
    }

    void writeSimulationOutput(const std::string &filename) {
        std::ofstream out(filename);
        if (!out.is_open()) {
            std::cerr << "Failed to open output file: " << filename << "\n";
            return;
        }

        out << "LandIndex,TileX,TileY,CropName,Growth,TimeToMature,SoilQuality\n";

        for (int landIdx = 0; landIdx < lands.size(); ++landIdx) {
            const auto &land = lands[landIdx];
            for (const auto &tile : land.tiles) {
                if (!tile.hasCrop) continue;

                float maturity = (tile.timeToMature >= 0.f) ? tile.timeToMature : simClock.getElapsedTime().asSeconds();

                out << landIdx << "," << tile.position.x << "," << tile.position.y << "," << cropTypes[selectedCropIndex].name << "," << std::fixed
                    << std::setprecision(2) << tile.crop.growth << "," << std::fixed << std::setprecision(2) << maturity << "," << std::fixed
                    << std::setprecision(2) << tile.soilQuality << "\n";
            }
        }

        out.close();
        std::cout << "Simulation output written to " << filename << "\n";
    }

    void reset() {
        simulate = false;
        rainActive = false;
        raindrops.clear();
        splashes.clear();

        for (auto &land : lands) {
            for (auto &tile : land.tiles) {
                tile.hasCrop = false;
                tile.crop.growth = 0.f;
                tile.crop.shape.setSize(sf::Vector2f(tile.size, tile.size) * Config::cropInitialScale);
                tile.crop.shape.setPosition(tile.position.x + (tile.size - tile.size * Config::cropInitialScale) / 2,
                                            tile.position.y + (tile.size - tile.size * Config::cropInitialScale) / 2);

                // Reset tile-specific environmental factors
                tile.waterLevel = 0.f;
                tile.soilQuality = 0.f;
            }

            land.plantCrops(cropTypes[selectedCropIndex]);  // now truly fresh
        }

        simClock.restart();
    }
};
}  // namespace Harvestor

#endif