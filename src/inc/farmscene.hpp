#ifndef FARM_SCENE_HPP_
#define FARM_SCENE_HPP_

#include <curl/curl.h>

#include <nlohmann/json.hpp>

#include "common.hpp"
#include "evaluator.hpp"
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
    Evaluator evaluator;

   public:
    // World data
    std::vector<Land> lands;
    std::vector<Pond> ponds;

    // Crops
    std::vector<CropType> cropTypes;
    int selectedCropIndex = -1;
    bool cropDropdownActive = false;
    int cropScrollOffset = 0;
    std::string bestCrop{""};

    // Layouts (new)
    std::vector<std::string> layouts;  // filenames (just name.ext)
    int selectedLayoutIndex = -1;      // currently chosen layout (index in layouts), -1 = none
    bool layoutDropdownActive = false;
    int layoutScrollOffset = 0;

    // Simulation controls
    bool simulate = false;
    bool alreadySelectionInProgress = false;
    bool selectAreaActive = false;

    // Rain
    bool rainActive = false;
    sf::Clock rainClock;
    std::vector<RainDrop> raindrops;
    std::vector<Splash> splashes;
    std::vector<Ripple> ripples;

    // selection area
    enum class SelectionState { None, Clicked, Selecting, Selected, Done };
    SelectionState selectionState = SelectionState::None;
    sf::Vector2f selectionStart;
    sf::Vector2f selectionEnd;
    sf::RectangleShape selectionRect;
    bool showAnalysisPopup = false;
    int selectedTilesCount = 0;

    FarmScene(sf::RenderWindow &win, GrassManager &gm, float w, float h) : window(win), grassManager(gm), width(w), height(h) {
        if (!font.loadFromFile(Config::fontPath)) std::cerr << "Failed to load font from: " << Config::fontPath << "\n";

        cropTypes = CropLoader::loadFromFile(Config::cropsFile);

        // Load available layouts safely
        loadAvailableLayouts("layouts");
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
                layouts.push_back(removeExtension(entry.path().filename().string()));  // just filename
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
        for (auto &land : lands) land.updateGrowth(dt, ponds, simulate, rainActive, simTime);

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
                        bool hit = false;

                        for (auto &ptile : pond.tiles) {
                            // Compute distance from the raindrop to the center of the pond tile
                            float dx = rd.position.x - (ptile.getPosition().x + ptile.getSize().x / 2.f);
                            float dy = rd.position.y - (ptile.getPosition().y + ptile.getSize().y / 2.f);
                            float dist = std::sqrt(dx * dx + dy * dy);

                            // Check if the raindrop hits the pond tile (inside tile bounds)
                            if (dist < ptile.getSize().x / 2.f) {
                                hit = true;
                                Splash s;
                                s.position = rd.position;
                                splashes.push_back(s);
                                Ripple r;
                                r.position = rd.position;
                                ripples.push_back(r);
                                break;  // Stop checking other tiles in this pond
                            }
                        }

                        if (hit) break;  // Stop checking other ponds if a hit is detected
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
        drawSelection();
    }

    void centerTextInButton(sf::Text &text, const sf::RectangleShape &button) {
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setOrigin(textBounds.left + textBounds.width / 2.f, textBounds.top + textBounds.height / 2.f);
        text.setPosition(button.getPosition().x + button.getSize().x / 2.f, button.getPosition().y + button.getSize().y / 2.f);
    }

    void drawSimulationInfo() {
        // HUD positioning
        float padding = 16.f;
        float barHeight = 16.f;
        float radius = barHeight / 2.f;  // rounded ends
        float lineSpacing = 8.f;         // space between lines
        float textHeight = 18.f;         // approximate text height
        float titleHeight = 28.f;

        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        float screenW = (float)desktop.width;
        float hudWidth = 300.f;
        float hudX = screenW - hudWidth - padding;  // top-right with 6.f gap
        float hudY = padding;

        // Count elements: title + time + rain + 3 bars
        int elements = 1 + 2 + 3;  // 1 title, 2 text lines, 3 bars
        float boxHeight = titleHeight + elements * (barHeight + textHeight + lineSpacing) + 3 * lineSpacing;
        sf::RectangleShape bgBox(sf::Vector2f(hudWidth, boxHeight));
        bgBox.setPosition(hudX, hudY);
        bgBox.setFillColor(sf::Color(40, 40, 40, 200));
        bgBox.setOutlineColor(sf::Color::White);
        bgBox.setOutlineThickness(2.f);
        window.draw(bgBox);

        float currentY = hudY + padding;

        // Title
        sf::Text title("Farm Status", font, 20);
        title.setPosition(hudX + padding, currentY);
        title.setFillColor(sf::Color(255, 215, 0));
        window.draw(title);

        currentY += titleHeight + lineSpacing;

        // Time
        float elapsed = simulate ? simClock.getElapsedTime().asSeconds() : 0.f;
        sf::Text timeText("Time: " + std::to_string(int(elapsed)) + "s", font, 16);
        timeText.setPosition(hudX + padding, currentY);
        timeText.setFillColor(sf::Color::Cyan);
        window.draw(timeText);

        currentY += textHeight + lineSpacing;

        // Raining info
        sf::Text rainText("Raining: " + std::string(rainActive ? "Yes" : "No"), font, 16);
        rainText.setPosition(hudX + padding, currentY);
        rainText.setFillColor(rainActive ? sf::Color::Blue : sf::Color(180, 180, 180));
        window.draw(rainText);

        currentY += textHeight + 2 * lineSpacing;  // extra spacing before bars

        // Compute averages
        float avgSoil = 0.f, avgWater = 0.f, avgGrowth = 0.f;
        int cropTiles = 0, totalTiles = 0;
        for (auto &land : lands) {
            totalTiles += (int)land.tiles.size();
            for (auto &tile : land.tiles) {
                avgSoil += land.computeSoilQuality(tile, tile.cropType);
                avgWater += tile.waterLevel;
                if (tile.hasCrop) {
                    avgGrowth += tile.crop.growth;
                    cropTiles++;
                }
            }
        }
        if (totalTiles > 0) avgSoil /= totalTiles;
        if (totalTiles > 0) avgWater /= totalTiles;
        if (cropTiles > 0) avgGrowth /= cropTiles;

        auto drawRoundedBar = [&](float y, const std::string &label, float pct, sf::Color fgColor) {
            pct = std::clamp(pct, 0.f, 1.f);
            // Label
            sf::Text txt(label, font, 16);
            txt.setPosition(hudX + padding, y);
            txt.setFillColor(sf::Color::White);
            window.draw(txt);

            float barWidth = hudWidth - 2 * padding;

            // Background bar
            sf::RectangleShape bg(sf::Vector2f(barWidth, barHeight));
            bg.setPosition(hudX + padding, y + textHeight);
            bg.setFillColor(sf::Color(80, 80, 80));
            window.draw(bg);

            // Foreground bar
            sf::RectangleShape fg(sf::Vector2f(barWidth * pct, barHeight));
            fg.setPosition(hudX + padding, y + textHeight);
            fg.setFillColor(fgColor);
            window.draw(fg);

            // Rounded ends using circles
            sf::CircleShape leftCap(radius);
            leftCap.setFillColor(fgColor);
            leftCap.setPosition(hudX + padding - radius, y + textHeight - radius + barHeight / 2.f);
            window.draw(leftCap);

            sf::CircleShape rightCap(radius);
            rightCap.setFillColor(fgColor);
            rightCap.setPosition(hudX + padding + barWidth * pct - radius, y + textHeight - radius + barHeight / 2.f);
            window.draw(rightCap);

            // Percentage text
            sf::Text pctText(std::to_string(int(pct * 100)) + "%", font, 14);
            pctText.setPosition(hudX + hudWidth - padding - 40, y + textHeight);
            pctText.setFillColor(sf::Color::White);
            window.draw(pctText);
        };

        drawRoundedBar(currentY, "Avg Soil", avgSoil, sf::Color::Green);
        currentY += barHeight + textHeight + 2 * lineSpacing;

        drawRoundedBar(currentY, "Water Level", avgWater, sf::Color::Blue);
        currentY += barHeight + textHeight + 2 * lineSpacing;

        drawRoundedBar(currentY, "Crop Growth", avgGrowth, sf::Color(255, 165, 0));
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

    std::string removeExtension(const std::string &filename) {
        size_t dotPos = filename.find_last_of('.');
        if (dotPos != std::string::npos) {
            return filename.substr(0, dotPos);
        }
        return filename;  // no dot found
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

    void centerTextInButton(sf::Text &text, const sf::FloatRect &btnBounds) {
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setOrigin(textBounds.left + textBounds.width / 2.f, textBounds.top + textBounds.height / 2.f);
        text.setPosition(btnBounds.left + btnBounds.width / 2.f, btnBounds.top + btnBounds.height / 2.f);
    }

    // Generic button drawer
    void drawButton(float x, float &y, const std::string &label, sf::Color baseColor, sf::Font &font, float width = -1, float height = -1,
                    bool hovered = false, bool pressed = false) {
        if (width < 0) width = Config::cropButtonWidth;
        if (height < 0) height = Config::cropButtonHeight * Config::bigButtonHeightMultiplier;

        float radius = 8.f;  // corner radius
        int cornerPoints = 16;
        float borderThickness = 2.f;

        // Adjust color for hover/press
        sf::Color color = baseColor;
        if (hovered) color = sf::Color(std::min(255, color.r + 40), std::min(255, color.g + 40), std::min(255, color.b + 40));
        if (pressed) color = sf::Color(std::max(0, color.r - 50), std::max(0, color.g - 50), std::max(0, color.b - 50));

        sf::Color borderColor(30, 30, 30);  // dark border

        // ---- Draw main rectangles ----
        sf::RectangleShape center(sf::Vector2f(width - 2 * radius, height - 2 * radius));
        center.setPosition(x + radius, y + radius);
        center.setFillColor(color);
        center.setOutlineColor(borderColor);
        center.setOutlineThickness(borderThickness);
        window.draw(center);

        // Top/Bottom
        sf::RectangleShape top(sf::Vector2f(width - 2 * radius, radius));
        top.setPosition(x + radius, y);
        top.setFillColor(color);
        top.setOutlineColor(borderColor);
        top.setOutlineThickness(borderThickness);
        window.draw(top);

        sf::RectangleShape bottom(sf::Vector2f(width - 2 * radius, radius));
        bottom.setPosition(x + radius, y + height - radius);
        bottom.setFillColor(color);
        bottom.setOutlineColor(borderColor);
        bottom.setOutlineThickness(borderThickness);
        window.draw(bottom);

        // Left/Right
        sf::RectangleShape left(sf::Vector2f(radius, height - 2 * radius));
        left.setPosition(x, y + radius);
        left.setFillColor(color);
        left.setOutlineColor(borderColor);
        left.setOutlineThickness(borderThickness);
        window.draw(left);

        sf::RectangleShape right(sf::Vector2f(radius, height - 2 * radius));
        right.setPosition(x + width - radius, y + radius);
        right.setFillColor(color);
        right.setOutlineColor(borderColor);
        right.setOutlineThickness(borderThickness);
        window.draw(right);

        // ---- Draw corners with border ----
        sf::CircleShape corner(radius, cornerPoints);
        corner.setFillColor(color);
        corner.setOutlineColor(borderColor);
        corner.setOutlineThickness(borderThickness);

        // Top-left
        corner.setPosition(x, y);
        window.draw(corner);
        // Top-right
        corner.setPosition(x + width - 2 * radius, y);
        window.draw(corner);
        // Bottom-left
        corner.setPosition(x, y + height - 2 * radius);
        window.draw(corner);
        // Bottom-right
        corner.setPosition(x + width - 2 * radius, y + height - 2 * radius);
        window.draw(corner);

        // ---- Draw text ----
        sf::Text txt(label, font, Config::bigButtonFontSize);
        txt.setFillColor(sf::Color::White);
        centerTextInButton(txt, sf::FloatRect(x, y, width, height));
        window.draw(txt);

        // Move y for next button
        y += height + Config::cropButtonSpacing + Config::ButtonInsideSpacing;
    }

    // ---------------- drawUI ----------------
    void drawUI() {
        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        float screenW = (float)desktop.width;
        float leftPanelW = screenW * 0.15f;  // 15% left side

        float x = Config::uiPadding;
        float y = 10.f;  // top padding

        // Crop dropdown
        std::vector<std::string> cropNames;
        for (auto &c : cropTypes) cropNames.push_back(c.name);
        std::string cropLabel =
            (selectedCropIndex >= 0 && selectedCropIndex < (int)cropTypes.size()) ? cropTypes[selectedCropIndex].name : "Select Crop";
        drawDropdown(x, y, cropLabel, cropNames, selectedCropIndex, cropDropdownActive, cropScrollOffset, Config::maxVisibleCrops);

        // Layout dropdown
        std::string layoutLabel =
            (selectedLayoutIndex >= 0 && selectedLayoutIndex < (int)layouts.size()) ? layouts[selectedLayoutIndex] : "Select Layout";
        drawDropdown(x, y, layoutLabel, layouts, selectedLayoutIndex, layoutDropdownActive, layoutScrollOffset, Config::maxVisibleLayouts);

        // Buttons start below dropdowns
        float startY = y + Config::ButtonPadding;
        if (cropDropdownActive) startY += (std::min(Config::maxVisibleCrops, (int)cropTypes.size()) + 1) * Config::cropButtonHeight;
        if (layoutDropdownActive) startY += (std::min(Config::maxVisibleLayouts, (int)layouts.size()) + 1) * Config::cropButtonHeight;

        // Shrink both width and height by 50%
        float btnWidth = (leftPanelW - 5.f * Config::uiPadding) * 0.75f;                         // half width
        float btnHeight = Config::cropButtonHeight * Config::bigButtonHeightMultiplier * 0.75f;  // half height
        float btnX = (leftPanelW - btnWidth) / 2.f;                                              // center horizontally
        float btnY = startY;

        sf::Color buttonBaseColor = sf::Color(50, 150, 200);
        // Draw buttons
        drawButton(btnX, btnY, simulate ? "Simulating" : "Start", simulate ? sf::Color(0, 255, 0) : buttonBaseColor, font, btnWidth, btnHeight);

        drawButton(btnX, btnY, "Reset", buttonBaseColor, font, btnWidth, btnHeight);
        drawButton(btnX, btnY, "Rain", rainActive ? sf::Color(100, 180, 255) : buttonBaseColor, font, btnWidth, btnHeight);
        drawButton(btnX, btnY, "Submit", buttonBaseColor, font, btnWidth, btnHeight);
        drawButton(btnX, btnY, "Load Layout", buttonBaseColor, font, btnWidth, btnHeight);

        sf::Color selectAreaColor = selectAreaActive ? sf::Color(150, 150, 150) : buttonBaseColor;
        std::string selectText = selectAreaActive ? "Deselect" : "Select Area";
        drawButton(btnX, btnY, selectText, selectAreaColor, font, btnWidth, btnHeight);

        drawButton(btnX, btnY, "Plant Crops", buttonBaseColor, font, btnWidth, btnHeight);
        drawButton(btnX, btnY, "Clear Results", buttonBaseColor, font, btnWidth, btnHeight);
        drawButton(btnX, btnY, "Analyse", buttonBaseColor, font, btnWidth, btnHeight);
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

    void clearSimulationResults() {
        std::ofstream out("simulation_output.csv", std::ios::trunc);
        out.close();

        std::cout << "Simulation results cleared.\n";
    }

    // ---------------- handleClick ----------------
    void handleClick(sf::Vector2i mousePos) {
        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        float screenW = (float)desktop.width;
        float leftPanelW = screenW * 0.15f;

        float x = Config::uiPadding;
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

        // Buttons start below dropdowns
        float startY = y + Config::ButtonPadding;
        if (cropDropdownActive) startY += (std::min(Config::maxVisibleCrops, (int)cropTypes.size()) + 1) * Config::cropButtonHeight;
        if (layoutDropdownActive) startY += (std::min(Config::maxVisibleLayouts, (int)layouts.size()) + 1) * Config::cropButtonHeight;

        float btnWidth = (leftPanelW - 5.f * Config::uiPadding) * 0.75f;  // same as drawUI
        float btnHeight = Config::cropButtonHeight * Config::bigButtonHeightMultiplier * 0.75f;
        float btnX = (leftPanelW - btnWidth) / 2.f;

        float extraSpacing = 6.f;  // same as drawButton spacing
        float currentY = startY;

        auto checkButtonClick = [&](const std::string &label, std::function<void()> callback) {
            sf::FloatRect rect(btnX, currentY, btnWidth, btnHeight);
            if (rect.contains((float)mousePos.x, (float)mousePos.y)) {
                callback();
                return true;
            }
            currentY += btnHeight + Config::cropButtonSpacing + extraSpacing;
            return false;
        };

        // Sequentially check each button
        if (checkButtonClick(simulate ? "Simulating" : "Start", [&]() {
                if (!simulate) simClock.restart();
                simulate = !simulate;
            }))
            return;
        if (checkButtonClick("Reset", [&]() { reset(); })) return;
        if (checkButtonClick("Rain", [&]() { startRain(); })) return;
        if (checkButtonClick("Submit", [&]() {
                writeSimulationOutput("simulation_output.csv");
                evaluator.updateSoilData();
                evaluator.updateCropData();
            }))
            return;
        if (checkButtonClick("Load Layout", [&]() {
                if (selectedLayoutIndex >= 0 && selectedLayoutIndex < (int)layouts.size()) {
                    std::string layoutPath = getLayoutFullPath(selectedLayoutIndex);
                    std::cout << "Loading layout: " << layoutPath << "\n";
                    generateFromFile(layoutPath + ".txt");
                }
            }))
            return;
        if (checkButtonClick(selectAreaActive ? "Deselect" : "Select Area", [&]() {
                handleSelectButtonClick();
                showAnalysisPopup = false;
            }))
            return;
        if (checkButtonClick("Plant Crops", [&]() { plantCropsInSelection(); })) return;
        if (checkButtonClick("Clear Results", [&]() { clearSimulationResults(); })) return;
        if (checkButtonClick("Analyse", [&]() { analyzeSelectedArea(); })) return;
    }

    void plantCropsInSelection() {
        if (selectedCropIndex < 0 || selectedCropIndex >= (int)cropTypes.size()) {
            std::cerr << "No crop selected for planting.\n";
            return;
        }
        if (selectionState != SelectionState::Selected && selectionState != SelectionState::Done) {
            std::cerr << "No area selected to plant crops.\n";
            return;
        }

        const CropType &chosenCrop = cropTypes[selectedCropIndex];

        // 1. Reset ALL crops in the farm
        if (!alreadySelectionInProgress) {
            for (auto &land : lands) {
                for (auto &tile : land.tiles) {
                    tile.hasCrop = true;
                    tile.crop = Crop();          // reset growth
                    tile.cropType = chosenCrop;  // store the actual crop type
                    tile.waterLevel = chosenCrop.optimalWater;
                    tile.soilQuality = land.computeSoilQuality(tile, chosenCrop);
                }
            }
        }

        // 2. Plant crops only in the selected area, using existing logic
        sf::FloatRect selRect = selectionRect.getGlobalBounds();
        int plantedCount = 0;
        alreadySelectionInProgress = true;

        for (auto &land : lands) {
            for (auto &tile : land.tiles) {
                sf::FloatRect tileRect(tile.position, sf::Vector2f(tile.size, tile.size));
                if (selRect.intersects(tileRect)) {
                    // Reuse your plantCrops logic on just this tile
                    float soilFactor = tile.soilBaseQuality;
                    tile.hasCrop = true;
                    tile.cropType = chosenCrop;
                    tile.crop.growth = 0.f;

                    tile.crop.originalSize = sf::Vector2f(tile.size, tile.size);

                    sf::Vector2f initialSize = tile.crop.originalSize * Config::cropInitialScale;
                    tile.crop.shape.setSize(initialSize);

                    tile.crop.shape.setPosition(tile.position.x + (tile.size - initialSize.x) / 2, tile.position.y + (tile.size - initialSize.y) / 2);

                    tile.crop.shape.setFillColor(chosenCrop.baseColor);

                    tile.waterLevel = chosenCrop.optimalWater;
                    tile.soilQuality = land.computeSoilQuality(tile, chosenCrop);

                    plantedCount++;
                } else {
                    // Outside selection, clear crop
                    tile.hasCrop = false;
                }
            }
        }

        std::cout << "Reset all crops and planted " << plantedCount << " crops of type " << chosenCrop.name << " in selection.\n";

        clearSelection();
    }

    void handleSelectButtonClick() {
        selectAreaActive = !selectAreaActive;
        if (selectionState != SelectionState::Clicked) {
            // Activate selection mode
            selectionState = SelectionState::Clicked;
        } else {
            // Deselect / cancel
            clearSelection();
        }
    }

    // ---------------- Mouse handling ----------------
    void handleMousePressed(sf::Vector2i mousePos) {
        // Only start the rectangle if we are waiting for the next click
        if (selectionState == SelectionState::Clicked) {
            selectionStart = sf::Vector2f((float)mousePos.x, (float)mousePos.y);
            selectionEnd = selectionStart;
            selectionRect.setPosition(selectionStart);
            selectionRect.setSize(sf::Vector2f(0.f, 0.f));  // Start with zero size
            selectionRect.setFillColor(sf::Color(0, 0, 255, 50));
            selectionRect.setOutlineColor(sf::Color::Blue);
            selectionRect.setOutlineThickness(2.f);
            selectionState = SelectionState::Selecting;
            std::cout << "Selection started at: " << selectionStart.x << "," << selectionStart.y << "\n";
        }
    }

    void handleMouseMoved(sf::Vector2i mousePos) {
        if (selectionState == SelectionState::Selecting) {
            selectionEnd = sf::Vector2f((float)mousePos.x, (float)mousePos.y);
            sf::Vector2f size = selectionEnd - selectionStart;
            selectionRect.setSize(sf::Vector2f(std::abs(size.x), std::abs(size.y)));
            selectionRect.setPosition(sf::Vector2f(std::min(selectionStart.x, selectionEnd.x), std::min(selectionStart.y, selectionEnd.y)));
        }
    }

    void handleMouseReleased(sf::Vector2i mousePos) {
        if (selectionState == SelectionState::Selecting) {
            selectionEnd = sf::Vector2f((float)mousePos.x, (float)mousePos.y);
            selectionState = SelectionState::Selected;
            showAnalysisPopup = true;

            // TODO: perform analysis on selected region here
            // analyzeSelectedArea();
        }
    }

    // ---------------- Draw selection rectangle ----------------
    void drawSelection() {
        if (selectionState == SelectionState::Selecting || selectionState == SelectionState::Selected) {
            window.draw(selectionRect);
        }

        if (showAnalysisPopup) {
            // Draw popup
            sf::RectangleShape popup(sf::Vector2f(250.f, 80.f));
            popup.setFillColor(sf::Color(50, 50, 50, 230));
            popup.setOutlineColor(sf::Color::White);
            popup.setOutlineThickness(2.f);
            popup.setPosition(selectionRect.getPosition().x, selectionRect.getPosition().y - 130.f);  // above selection
            window.draw(popup);

            sf::Text text("Area analyzed!", font, 16);
            text.setFillColor(sf::Color::White);
            text.setPosition(popup.getPosition().x + 10.f, popup.getPosition().y + 10.f);
            window.draw(text);

            // ---------------- Collect tiles inside selection ----------------
            sf::FloatRect selRect = selectionRect.getGlobalBounds();

            std::vector<std::tuple<int, int>> selectedTiles;  // landIndex, tileIndex, cropName

            for (int landIdx = 0; landIdx < (int)lands.size(); ++landIdx) {
                const auto &land = lands[landIdx];
                for (int tileIdx = 0; tileIdx < (int)land.tiles.size(); ++tileIdx) {
                    const auto &tile = land.tiles[tileIdx];
                    sf::FloatRect tileRect(tile.position, sf::Vector2f(tile.size, tile.size));

                    if (selRect.intersects(tileRect)) {
                        selectedTiles.emplace_back(landIdx, tileIdx);
                    }
                }
            }

            // Optional: show count on popup
            sf::Text countText("Tiles: " + std::to_string(selectedTiles.size()), font, 14);
            countText.setFillColor(sf::Color::White);
            countText.setPosition(popup.getPosition().x + 10.f, popup.getPosition().y + 40.f);
            window.draw(countText);

            // sf::Text cropText("BestCrop: " + bestCrop, font, 14);
            // cropText.setFillColor(sf::Color::White);
            // cropText.setPosition(popup.getPosition().x + 10.f, popup.getPosition().y + 80.f);
            // window.draw(cropText);
        }
    }

    // ---------------- Analyze selected area ----------------
    void analyzeSelectedArea() {
        sf::FloatRect area(selectionRect.getPosition(), selectionRect.getSize());

        // TODO: Arka api
        Point topLeft{area.left, area.top};
        Point bottomRight{area.left + area.width, area.top + area.height};

        std::cout << "Selected area: Topleft={" << topLeft.x << "," << topLeft.y << "}, BottomRight={" << bottomRight.x << "," << bottomRight.y
                  << "}\n";

        bestCrop = evaluator.getBestCropForArea(topLeft, bottomRight);
    }

    void clearSelection() {
        selectAreaActive = false;
        selectionState = SelectionState::None;              // Reset state
        showAnalysisPopup = false;                          // Hide popup
        selectionRect.setSize(sf::Vector2f(0.f, 0.f));      // Clear rectangle
        selectionRect.setPosition(sf::Vector2f(0.f, 0.f));  // Reset position
    }

    void writeSimulationOutput(const std::string &filename) {
        std::ofstream out(filename, std::ios::app);  // <-- append mode
        if (!out.is_open()) {
            std::cerr << "Failed to open output file: " << filename << "\n";
            return;
        }

        // Write header only if file is empty
        static bool headerWritten = false;
        if (!headerWritten) {
            out << "LandIndex,TileX,TileY,CropName,Growth,TimeToMature,SoilQuality\n";
            headerWritten = true;
        }

        for (int landIdx = 0; landIdx < (int)lands.size(); ++landIdx) {
            const auto &land = lands[landIdx];
            for (const auto &tile : land.tiles) {
                if (!tile.hasCrop) continue;
                float maturity = (tile.timeToMature >= 0.f) ? tile.timeToMature : simClock.getElapsedTime().asSeconds();
                if (tile.crop.growth >= 1.f) {
                    out << landIdx << "," << tile.position.x << "," << tile.position.y << "," << tile.cropType.name << "," << std::fixed
                        << std::setprecision(2) << tile.crop.growth << "," << maturity << "," << tile.soilQuality << "\n";
                }
            }
        }

        out.close();
        std::cout << "Simulation output appended to " << filename << "\n";
    }

    void reset() {
        simulate = false;
        rainActive = false;
        alreadySelectionInProgress = false;
        raindrops.clear();
        splashes.clear();
        ripples.clear();

        clearSelection();

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
            // land.plantCrops(cropTypes[selectedCropIndex]);
        }
        simClock.restart();
    }
};

}  // namespace Harvestor

#endif