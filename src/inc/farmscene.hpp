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
    // World data
    std::vector<Land> lands;
    std::vector<Pond> ponds;

    // Crops
    std::vector<CropType> cropTypes;
    int selectedCropIndex = -1;
    bool cropDropdownActive = false;
    int cropScrollOffset = 0;

    // Layouts (new)
    std::vector<std::string> layouts;  // filenames (just name.ext)
    int selectedLayoutIndex = -1;      // currently chosen layout (index in layouts), -1 = none
    bool layoutDropdownActive = false;
    int layoutScrollOffset = 0;

    // Simulation controls
    bool simulate = false;

    // Rain
    bool rainActive = false;
    sf::Clock rainClock;
    std::vector<RainDrop> raindrops;
    std::vector<Splash> splashes;
    std::vector<Ripple> ripples;

    FarmScene(sf::RenderWindow &win, GrassManager &gm, float w, float h) : window(win), grassManager(gm), width(w), height(h) {
        if (!font.loadFromFile(Config::fontPath)) std::cerr << "Failed to load font from: " << Config::fontPath << "\n";

        cropTypes = CropLoader::loadFromFile(Config::cropsFile);

        // Load available layouts safely
        loadAvailableLayouts("layouts");

        // // Select first layout automatically if available
        // if (!layouts.empty()) {
        //     // selectedLayoutIndex = 0;  // select first layout
        //     generateFromFile(getLayoutFullPath(selectedLayoutIndex));
        // } else {
        //     std::cerr << "No layouts found! Please check the 'layouts' folder.\n";
        // }
    }
    // Scan a folder for .txt layout files
    void loadAvailableLayouts(const std::string &folder) {
        layouts.clear();

        std::filesystem::path layoutsPath(folder);

        if (!std::filesystem::exists(layoutsPath) || !std::filesystem::is_directory(layoutsPath)) {
            std::cerr << "Layout folder does not exist: " << folder << "\n";
            return;
        }

        for (const auto &entry : std::filesystem::directory_iterator(layoutsPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                layouts.push_back(entry.path().filename().string());  // just filename
                std::cout << "Found layout: " << entry.path().filename().string() << "\n";
            }
        }

        std::sort(layouts.begin(), layouts.end());

        if (layouts.empty()) std::cerr << "No .txt layout files found in folder: " << folder << "\n";
    }

    std::string getLayoutFullPath(int index) const {
        if (index < 0 || index >= (int)layouts.size()) return "";
        return "layouts/" + layouts[index];
    }

    void generateFromFile(const std::string &filePath) {
        if (filePath.empty()) {
            std::cerr << "generateFromFile: empty file path, aborting.\n";
            return;
        }

        lands.clear();
        ponds.clear();
        FarmLoader::loadFromFile(filePath, lands, ponds, cropTypes, selectedCropIndex);
        std::cout << "Loaded lands: " << lands.size() << ", ponds: " << ponds.size() << "\n";

        grassManager.generate(width, height);
        simClock.restart();
    }

    void startRain() {
        rainActive = true;
        rainClock.restart();
        raindrops.clear();
        splashes.clear();
        ripples.clear();
        std::uniform_int_distribution<int> posXDist((int)Config::uiPanelWidth, (int)width - 1);
        std::uniform_int_distribution<int> posYDist(0, (int)height - 1);
        std::uniform_int_distribution<int> speedDist(200, 350);
        static std::mt19937 rng((unsigned)time(nullptr));
        for (int i = 0; i < Config::numRaindrops; i++) {
            RainDrop rd;
            rd.position = sf::Vector2f((float)posXDist(rng), (float)posYDist(rng));
            rd.speed = (float)speedDist(rng);
            raindrops.push_back(rd);
        }
    }

    // Called from main loop with dt
    void update(float dt) {
        float simTime = simClock.getElapsedTime().asSeconds();
        for (auto &land : lands) land.updateGrowth(dt, ponds, cropTypes[selectedCropIndex], simulate, rainActive, simTime);

        if (rainActive) {
            float elapsed = rainClock.getElapsedTime().asSeconds();
            if (elapsed > Config::rainDuration) {
                rainActive = false;
                raindrops.clear();
            } else {
                // locally boost water/growth for tiles (keeps pond logic consistent)
                for (auto &land : lands) {
                    for (auto &tile : land.tiles) {
                        if (!tile.hasCrop) continue;
                        tile.waterLevel += dt * 0.3f;
                        tile.waterLevel = std::clamp(tile.waterLevel, 0.f, 1.f);

                        tile.crop.growth += dt * Config::rainGrowthBoost;
                        tile.crop.growth = std::clamp(tile.crop.growth, 0.f, 1.f);
                    }
                }

                // update raindrops animation
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
                        rd.position.x = (float)(Config::uiPanelWidth + (rand() % (int)(width - Config::uiPanelWidth)));
                    }
                }
            }
        }

        // Update splashes and ripples (safe erase)
        splashes.erase(std::remove_if(splashes.begin(), splashes.end(), [dt](Splash &s) { return !s.update(dt); }), splashes.end());
        ripples.erase(std::remove_if(ripples.begin(), ripples.end(), [dt](Ripple &r) { return !r.update(dt); }), ripples.end());
    }

    void draw() {
        // UI panel
        sf::RectangleShape uiPanel(sf::Vector2f(Config::uiPanelWidth, height));
        uiPanel.setFillColor(sf::Color(70, 70, 70));
        window.draw(uiPanel);

        // World
        grassManager.draw(window);
        for (auto &land : lands) land.draw(window, cropTypes[selectedCropIndex]);
        for (auto &pond : ponds) pond.draw(window);

        // Rain visuals
        if (rainActive) {
            for (auto &rd : raindrops) {
                sf::RectangleShape drop(sf::Vector2f(2.f, 8.f));
                drop.setFillColor(sf::Color(0, 150, 255));
                drop.setPosition(rd.position);
                window.draw(drop);
            }
            for (auto &s : splashes) s.draw(window);
            for (auto &r : ripples) r.draw(window);
        }

        // UI + info
        drawUI();
        drawSimulationInfo();
    }

    void centerTextInButton(sf::Text &text, const sf::RectangleShape &button) {
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setOrigin(textBounds.left + textBounds.width / 2.f, textBounds.top + textBounds.height / 2.f);
        text.setPosition(button.getPosition().x + button.getSize().x / 2.f, button.getPosition().y + button.getSize().y / 2.f);
    }

    void drawSimulationInfo() {
        if (!simulate) return;

        float elapsed = simClock.getElapsedTime().asSeconds();
        std::stringstream ss;
        ss << "Time: " << std::fixed << std::setprecision(1) << elapsed << "s";
        sf::Text txt(ss.str(), font, 16);
        txt.setPosition(10, height - 60);
        txt.setFillColor(sf::Color::White);
        window.draw(txt);

        for (int i = 0; i < (int)lands.size(); ++i) {
            float pct = lands[i].getCropGrowthPercentage();
            std::stringstream ssp;
            ssp << "Land " << i + 1 << ": " << std::fixed << std::setprecision(0) << pct << "%";
            sf::Text landTxt(ssp.str(), font, 14);
            landTxt.setPosition(lands[i].center.x - lands[i].radius / 2, lands[i].center.y - lands[i].radius - 20);
            landTxt.setFillColor(sf::Color::White);
            window.draw(landTxt);
        }
    }

    // New: handle mouse wheel scroll (call from main loop)
    void handleMouseWheelScroll(float delta) {
        if (cropDropdownActive) {
            if (delta > 0 && cropScrollOffset > 0) cropScrollOffset--;
            if (delta < 0 && cropScrollOffset + Config::maxVisibleCrops < (int)cropTypes.size()) cropScrollOffset++;
        }
        if (layoutDropdownActive) {
            if (delta > 0 && layoutScrollOffset > 0) layoutScrollOffset--;
            if (delta < 0 && layoutScrollOffset + Config::maxVisibleCrops < (int)layouts.size()) layoutScrollOffset++;
        }
    }

    // Generic dropdown drawer
    void drawDropdown(float x, float &y, const std::string &mainLabel, const std::vector<std::string> &items, int selectedIndex, bool &active,
                      int scrollOffset, int maxVisible) {
        float btnHeight = Config::cropButtonHeight;

        // --- Main dropdown button ---
        sf::RectangleShape mainBtn(sf::Vector2f(Config::cropButtonWidth, btnHeight));
        mainBtn.setPosition(x, y);
        mainBtn.setFillColor(sf::Color(180, 180, 250));
        window.draw(mainBtn);

        sf::Text mainTxt(mainLabel, font, 16);
        mainTxt.setPosition(x + 5, y + 5);
        mainTxt.setFillColor(sf::Color::Black);
        window.draw(mainTxt);

        // --- Dropdown arrow (triangle) ---
        sf::ConvexShape arrow;
        arrow.setPointCount(3);
        float arrowSize = 8.f;
        float arrowX = x + Config::cropButtonWidth - 15;
        float arrowY = y + btnHeight / 2.f;

        if (active) {
            // Upward triangle
            arrow.setPoint(0, sf::Vector2f(arrowX - arrowSize, arrowY + arrowSize / 2));
            arrow.setPoint(1, sf::Vector2f(arrowX + arrowSize, arrowY + arrowSize / 2));
            arrow.setPoint(2, sf::Vector2f(arrowX, arrowY - arrowSize / 2));
        } else {
            // Downward triangle
            arrow.setPoint(0, sf::Vector2f(arrowX - arrowSize, arrowY - arrowSize / 2));
            arrow.setPoint(1, sf::Vector2f(arrowX + arrowSize, arrowY - arrowSize / 2));
            arrow.setPoint(2, sf::Vector2f(arrowX, arrowY + arrowSize / 2));
        }
        arrow.setFillColor(sf::Color::Black);
        window.draw(arrow);

        y += btnHeight;  // Move y down for items or next button

        // --- Draw dropdown items ---
        if (active) {
            int visibleCount = std::min(maxVisible, (int)items.size());
            for (int i = 0; i < visibleCount; i++) {
                int idx = scrollOffset + i;
                if (idx >= items.size()) break;

                sf::RectangleShape itemBtn(sf::Vector2f(Config::cropButtonWidth, btnHeight));
                itemBtn.setPosition(x, y + i * btnHeight);
                itemBtn.setFillColor(idx == selectedIndex ? Config::cropButtonSelectedColor : Config::cropButtonColor);
                window.draw(itemBtn);

                sf::Text itemTxt(items[idx], font, 16);
                itemTxt.setPosition(x + 5, y + 5 + i * btnHeight);
                itemTxt.setFillColor(sf::Color::Black);
                window.draw(itemTxt);
            }

            // --- Scroll arrows ---
            if (scrollOffset > 0) {
                sf::Text upTxt("▲", font, 14);
                upTxt.setPosition(x + Config::cropButtonWidth + 5, y);
                upTxt.setFillColor(sf::Color::Black);
                window.draw(upTxt);
            }
            if (scrollOffset + visibleCount < items.size()) {
                sf::Text downTxt("▼", font, 14);
                downTxt.setPosition(x + Config::cropButtonWidth + 5, y + visibleCount * btnHeight);
                downTxt.setFillColor(sf::Color::Black);
                window.draw(downTxt);
            }

            y += visibleCount * btnHeight;  // Move y after dropdown
        }
    }

    // Generic button drawer
    void drawButton(float x, float &y, const std::string &label, sf::Color color, sf::Font &font, float width = -1, float height = -1) {
        if (width < 0) width = Config::cropButtonWidth;
        if (height < 0) height = Config::cropButtonHeight * Config::bigButtonHeightMultiplier;

        sf::RectangleShape btn(sf::Vector2f(width, height));
        btn.setPosition(x, y);
        btn.setFillColor(color);
        window.draw(btn);

        sf::Text txt(label, font, Config::bigButtonFontSize);
        centerTextInButton(txt, btn);
        window.draw(txt);

        y += height + Config::cropButtonSpacing;  // move y for next button
    }

    // ---------------- drawUI ----------------
    void drawUI() {
        float x = 10.f;
        float y = 10.f;

        std::vector<std::string> cropNames;
        for (auto &c : cropTypes) cropNames.push_back(c.name);

        std::string cropLabel =
            (selectedCropIndex >= 0 && selectedCropIndex < (int)cropTypes.size()) ? cropTypes[selectedCropIndex].name : "Select Crop";
        drawDropdown(x, y, cropLabel, cropNames, selectedCropIndex, cropDropdownActive, cropScrollOffset, Config::maxVisibleCrops);

        std::string layoutLabel =
            (selectedLayoutIndex >= 0 && selectedLayoutIndex < (int)layouts.size()) ? layouts[selectedLayoutIndex] : "Select Layout";
        drawDropdown(x, y, layoutLabel, layouts, selectedLayoutIndex, layoutDropdownActive, layoutScrollOffset, Config::maxVisibleLayouts);

        // ---------------- Regular buttons ----------------
        float startY = Config::ButtonPadding + Config::cropButtonHeight;
        if (cropDropdownActive) startY += (std::min(Config::maxVisibleCrops, (int)cropTypes.size()) + 1) * Config::cropButtonHeight;
        if (layoutDropdownActive) startY += (std::min(Config::maxVisibleLayouts, (int)layouts.size()) + 1) * Config::cropButtonHeight;

        float bigButtonHeight = Config::cropButtonHeight * Config::bigButtonHeightMultiplier;
        float btnX = Config::uiPadding;
        float btnY = startY;

        // Add after your regular buttons

        drawButton(btnX, btnY, simulate ? "Simulating" : "Start", simulate ? Config::simulateButtonActiveColor : Config::simulateButtonColor, font,
                   Config::cropButtonWidth, bigButtonHeight);

        drawButton(btnX, btnY, "Reset", Config::resetButtonColor, font, Config::cropButtonWidth, bigButtonHeight);

        drawButton(btnX, btnY, "Rain", rainActive ? Config::simulateButtonActiveColor : Config::rainButtonColor, font, Config::cropButtonWidth,
                   bigButtonHeight);

        drawButton(btnX, btnY, "Save Results", sf::Color(150, 250, 150), font, Config::cropButtonWidth, bigButtonHeight);

        drawButton(btnX, btnY, "Load Layout", sf::Color(180, 250, 180), font, Config::cropButtonWidth, bigButtonHeight);
    }

    // Generic dropdown click handler
    template <typename T>
    bool handleDropdownClick(sf::Vector2i mousePos, float x, float &y, std::vector<T> &items, int &selectedIndex, bool &dropdownActive,
                             int &scrollOffset, int maxVisible) {
        float btnHeight = Config::cropButtonHeight;
        sf::FloatRect mainRect(x, y, Config::cropButtonWidth, btnHeight);
        if (mainRect.contains(mousePos.x, mousePos.y)) {
            dropdownActive = !dropdownActive;
            return true;
        }

        if (dropdownActive) {
            int visibleCount = std::min(maxVisible, (int)items.size());
            for (int i = 0; i < visibleCount; i++) {
                int idx = scrollOffset + i;
                if (idx >= items.size()) break;

                sf::FloatRect itemRect(x, y + (i + 1) * btnHeight, Config::cropButtonWidth, btnHeight);
                if (itemRect.contains(mousePos.x, mousePos.y)) {
                    selectedIndex = idx;
                    dropdownActive = false;
                    return true;
                }
            }

            sf::FloatRect upRect(x + Config::cropButtonWidth + 5, y + btnHeight, 20, 20);
            sf::FloatRect downRect(x + Config::cropButtonWidth + 5, y + visibleCount * btnHeight, 20, 20);

            if (upRect.contains(mousePos.x, mousePos.y) && scrollOffset > 0) {
                scrollOffset--;
                return true;
            }
            if (downRect.contains(mousePos.x, mousePos.y) && scrollOffset + visibleCount < items.size()) {
                scrollOffset++;
                return true;
            }

            dropdownActive = false;
        }

        y += btnHeight + Config::cropButtonSpacing;
        return false;
    }

    // ---------------- handleClick ----------------
    void handleClick(sf::Vector2i mousePos) {
        float x = 10.f;
        float y = 10.f;

        // Crop dropdown
        std::vector<std::string> cropNames;
        for (auto &c : cropTypes) cropNames.push_back(c.name);
        if (handleDropdownClick(mousePos, x, y, cropNames, selectedCropIndex, cropDropdownActive, cropScrollOffset, Config::maxVisibleCrops)) return;

        // Layout dropdown
        if (handleDropdownClick(mousePos, x, y, layouts, selectedLayoutIndex, layoutDropdownActive, layoutScrollOffset, Config::maxVisibleLayouts)) {
            if (selectedLayoutIndex >= 0 && selectedLayoutIndex < (int)layouts.size()) {
                std::string layoutPath = getLayoutFullPath(selectedLayoutIndex);
                std::cout << "Selected layout: " << layoutPath << "\n";
            }
            return;
        }

        // ---------------- Regular buttons ----------------
        float startY = Config::ButtonPadding + (cropDropdownActive ? (cropTypes.size() + 1) * Config::cropButtonHeight : Config::cropButtonHeight);
        startY += layoutDropdownActive ? (std::min(Config::maxVisibleLayouts, (int)layouts.size()) + 1) * Config::cropButtonHeight
                                       : Config::cropButtonHeight;

        float bigButtonHeight = Config::cropButtonHeight * Config::bigButtonHeightMultiplier;

        sf::FloatRect simBtnRect(Config::uiPadding, startY, Config::cropButtonWidth, bigButtonHeight);
        sf::FloatRect resetBtnRect(Config::uiPadding, startY + bigButtonHeight + Config::cropButtonSpacing, Config::cropButtonWidth, bigButtonHeight);
        sf::FloatRect rainBtnRect(Config::uiPadding, startY + 2 * bigButtonHeight + 2 * Config::cropButtonSpacing, Config::cropButtonWidth,
                                  bigButtonHeight);
        sf::FloatRect saveBtnRect(Config::uiPadding, startY + 3 * bigButtonHeight + 3 * Config::cropButtonSpacing, Config::cropButtonWidth,
                                  bigButtonHeight);
        sf::FloatRect loadLayoutBtnRect(Config::uiPadding, startY + 4 * bigButtonHeight + 4 * Config::cropButtonSpacing, Config::cropButtonWidth,
                                        bigButtonHeight);

        if (loadLayoutBtnRect.contains(mousePos.x, mousePos.y)) {
            if (selectedLayoutIndex >= 0 && selectedLayoutIndex < (int)layouts.size()) {
                std::string layoutPath = getLayoutFullPath(selectedLayoutIndex);
                std::cout << "Loading layout: " << layoutPath << "\n";
                generateFromFile(layoutPath);
            }
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
        if (saveBtnRect.contains(mousePos.x, mousePos.y)) {
            writeSimulationOutput("simulation_output.csv");
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
        for (int landIdx = 0; landIdx < (int)lands.size(); ++landIdx) {
            const auto &land = lands[landIdx];
            for (const auto &tile : land.tiles) {
                if (!tile.hasCrop) continue;
                float maturity = (tile.timeToMature >= 0.f) ? tile.timeToMature : simClock.getElapsedTime().asSeconds();
                out << landIdx << "," << tile.position.x << "," << tile.position.y << "," << cropTypes[selectedCropIndex].name << "," << std::fixed
                    << std::setprecision(2) << tile.crop.growth << "," << maturity << "," << tile.soilQuality << "\n";
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
        ripples.clear();

        for (auto &land : lands) {
            for (auto &tile : land.tiles) {
                tile.hasCrop = false;
                tile.crop.growth = 0.f;
                tile.crop.shape.setSize(sf::Vector2f(tile.size, tile.size) * Config::cropInitialScale);
                tile.crop.shape.setPosition(tile.position.x + (tile.size - tile.size * Config::cropInitialScale) / 2,
                                            tile.position.y + (tile.size - tile.size * Config::cropInitialScale) / 2);
                tile.waterLevel = 0.f;
                tile.soilQuality = 0.f;
                tile.timeToMature = -1.f;
            }
            land.plantCrops(cropTypes[selectedCropIndex]);
        }
        simClock.restart();
    }
};
}  // namespace Harvestor

#endif