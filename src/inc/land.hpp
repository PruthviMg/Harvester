#ifndef LAND_HPP_
#define LAND_HPP_

#include <SFML/Graphics/Color.hpp>

#include "common.hpp"

namespace Harvestor {
// ---------------- Land ----------------
class Land {
   public:
    sf::ConvexShape shape;
    sf::Vector2f center;
    float radius;
    std::vector<Tile> tiles;
    float tileSize;
    sf::Texture farmlandTexture;
    static sf::Texture wheatTexture;
    static sf::Texture sugarcaneTexture;
    static sf::Texture tomatoTexture;
    bool loaded = false;

    Land(float tileSize = Config::landTileSize) : tileSize(tileSize) {
        // Load farmland texture once
        if (!loaded) {
            sf::Image img;
            if (!img.loadFromFile("resources/combined.png")) {
                std::cerr << "Failed to load farmland texture!" << std::endl;
            } else {
                // Crop center 5x5 area
                sf::Vector2u size = img.getSize();
                unsigned int cropSize = 5 * tileSize;  // 5x5 tiles in pixels
                unsigned int x = (size.x - cropSize) / 2;
                unsigned int y = (size.y - cropSize) / 2;

                sf::IntRect centerRect(x, y, cropSize, cropSize);
                sf::Image cropped;
                cropped.create(cropSize, cropSize, sf::Color::Transparent);
                cropped.copy(img, 0, 0, centerRect, true);

                farmlandTexture.loadFromImage(cropped);
                farmlandTexture.setRepeated(true);
                farmlandTexture.setSmooth(true);
                loaded = true;
            }
            if (!wheatTexture.loadFromFile("resources/wheat1.png")) {
                std::cerr << "Failed to load water texture!" << std::endl;
            } else {
                wheatTexture.setRepeated(true);
                wheatTexture.setSmooth(true);
            }
            if (!sugarcaneTexture.loadFromFile("resources/sugarcane.png")) {
                std::cerr << "Failed to load water texture!" << std::endl;
            } else {
                sugarcaneTexture.setRepeated(true);
                sugarcaneTexture.setSmooth(true);
            }
            if (!tomatoTexture.loadFromFile("resources/tomato.png")) {
                std::cerr << "Failed to load water texture!" << std::endl;
            } else {
                tomatoTexture.setRepeated(true);
                tomatoTexture.setSmooth(true);
            }
        }

    }

    void generateTiles(std::vector<Tile> ts) {
        for (auto &t : ts) {
            t.size = Config::landTileSize;
            t.isInsideLand = true;
            t.hasCrop = false;
            t.waterLevel = 0.f;
            tiles.push_back(t);
        }
    }

    void generateTiles(const std::vector<std::array<float, 9>> &soilMatrix = {}) {
        tiles.clear();

        if (soilMatrix.empty()) {
            std::cerr << "Soil matrix is empty! Cannot generate tiles.\n";
            return;
        }

        // Step 1: Find min/max of x and y from CSV
        float minX = std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float maxY = std::numeric_limits<float>::lowest();

        for (const auto &vals : soilMatrix) {
            minX = std::min(minX, vals[0]);
            minY = std::min(minY, vals[1]);
            maxX = std::max(maxX, vals[0]);
            maxY = std::max(maxY, vals[1]);
        }

        float rangeX = maxX - minX;
        float rangeY = maxY - minY;

        if (rangeX == 0 || rangeY == 0) {
            std::cerr << "Invalid CSV coordinates, cannot scale.\n";
            return;
        }

        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        float screenW = (float)desktop.width;
        float screenH = (float)desktop.height;

        // Reserve 15% for UI → usable area is 85%
        float usableW = screenW * 0.85f;
        float uiOffsetX = screenW * 0.15f;

        float scaleX = usableW / rangeX;
        float scaleY = screenH / rangeY;

        // Keep aspect ratio
        float scale = std::min(scaleX, scaleY);

        // Step 3: Generate tiles + surrounding 32
        for (const auto &vals : soilMatrix) {
            // Base normalized position
            float normX = (vals[0] - minX) * scale;
            float normY = (vals[1] - minY) * scale;

            float offsetX = uiOffsetX + (usableW - (rangeX * scale)) / 2.f;
            float offsetY = (screenH - (rangeY * scale)) / 2.f;

            float baseX = normX + offsetX;
            float baseY = normY + offsetY;

            float tileSize = Config::landTileSize;

            // Generate original + 32 surrounding tiles
            int radius = 0;  // 3 tiles in each direction → 7x7 = 49 tiles (48 neighbors + center)
            // If you need **exactly** 32, we can restrict this, but usually 3x3 or 7x7 grids are natural.

            for (int dx = -radius; dx <= radius; ++dx) {
                for (int dy = -radius; dy <= radius; ++dy) {
                    float px = baseX + dx * tileSize;
                    float py = baseY + dy * tileSize;

                    // --- Prevent going into UI area    (left 15%) ---
                    if (px < uiOffsetX) continue;

                    // Also keep within screen bounds
                    if (px + tileSize > screenW) continue;
                    if (py < 0 || py + tileSize > screenH) continue;

                    Tile t;
                    t.position = sf::Vector2f(px, py);
                    t.size = tileSize;
                    t.isInsideLand = true;
                    t.hasCrop = false;

                    // Soil attributes
                    t.soilBaseQuality = vals[2];
                    t.sunlight = vals[3];
                    t.nutrients = vals[4];
                    t.pH = vals[5];
                    t.organicMatter = vals[6];
                    t.compaction = vals[7];
                    t.salinity = vals[8];

                    t.waterLevel = 0.f;

                    tiles.push_back(t);
                }
            }
        }

        std::cout << "Generated " << tiles.size() << " tiles (with 32 neighbors each).\n";
    }

    float computeSoilQuality(const Tile &tile, const CropType &crop) {
        float waterFactor = 1.f - std::abs(tile.waterLevel - crop.optimalWater) / crop.tolerance;
        waterFactor = std::clamp(waterFactor, 0.f, 1.f);

        float quality = 0.f;
        quality += 0.25f * tile.soilBaseQuality;
        quality += 0.15f * tile.sunlight;
        quality += 0.15f * tile.nutrients;
        quality += 0.1f * tile.pH;
        quality += 0.15f * tile.organicMatter;
        quality += 0.1f * (1.f - tile.compaction);
        quality += 0.1f * (1.f - tile.salinity);
        quality += 0.1f * waterFactor;  // water now included in weighted sum

        return std::clamp(quality, 0.f, 1.f);
    }

    // ---------------- Update Tile Water ----------------
    void updateTileWater(Tile &tile, const std::vector<Pond> &ponds, float dt, const CropType &cropType) {
        float targetWater = 0.f;

        for (const auto &pond : ponds) {
            for (const auto &ptile : pond.tiles) {
                // Compute center-to-center distance
                float dx = (tile.position.x + tile.size / 2.f) - (ptile.getPosition().x + ptile.getSize().x / 2.f);
                float dy = (tile.position.y + tile.size / 2.f) - (ptile.getPosition().y + ptile.getSize().y / 2.f);
                float dist = std::sqrt(dx * dx + dy * dy);

                // Water contribution decreases with distance (max distance = 5 * tile size)
                float waterFactor = std::max(0.f, 1.f - dist / (ptile.getSize().x * 5.f));

                // Take the maximum water factor among all pond tiles
                targetWater = std::max(targetWater, waterFactor * cropType.optimalWater);
            }
        }

        // Smoothly approach target water level
        float waterSpeed = 0.5f;  // change rate per second
        tile.waterLevel += (targetWater - tile.waterLevel) * waterSpeed * dt;

        // Evaporation
        float evaporation = 0.01f * dt;
        tile.waterLevel = std::max(tile.waterLevel - evaporation, 0.f);
    }

    void plantCrops(const CropType &cropType) {
        for (auto &tile : tiles) {
            tile.hasCrop = true;
            tile.cropType = cropType;  // <--- assign the crop type
            tile.crop.growth = 0.f;
            tile.crop.originalSize = sf::Vector2f(tile.size, tile.size);

            sf::Vector2f initialSize = tile.crop.originalSize * Config::cropInitialScale;
            tile.crop.shape.setSize(initialSize);
            tile.crop.shape.setPosition(tile.position.x + (tile.size - initialSize.x) / 2, tile.position.y + (tile.size - initialSize.y) / 2);
            tile.crop.shape.setFillColor(cropType.baseColor);

            // INITIAL WATER: set close to crop optimal
            tile.waterLevel = cropType.optimalWater;

            // Compute initial soil quality
            tile.soilQuality = computeSoilQuality(tile, cropType);
        }
    }

    void updateGrowth(float dt, const std::vector<Pond> &ponds, bool simulate, bool raining, float simTime) {
        if (!simulate) return;

        static std::mt19937 rng(12345);                          // fixed seed for reproducibility
        std::uniform_real_distribution<float> dist(0.9f, 1.1f);  // small variability

        for (auto &tile : tiles) {
            if (!tile.hasCrop) continue;

            const CropType &cropType = tile.cropType;  // Use the actual crop planted in this tile

            // ---------------- Rain ----------------
            if (raining) {
                tile.waterLevel += Config::rainIntensity * dt;
                tile.waterLevel = std::clamp(tile.waterLevel, 0.f, 1.f);
            }

            // ---------------- Water from Ponds ----------------
            float targetWater = 0.f;
            for (const auto &pond : ponds) {
                for (const auto &ptile : pond.tiles) {
                    // Compute center-to-center distance between the land tile and pond tile
                    float dx = (tile.position.x + tile.size / 2.f) - (ptile.getPosition().x + ptile.getSize().x / 2.f);
                    float dy = (tile.position.y + tile.size / 2.f) - (ptile.getPosition().y + ptile.getSize().y / 2.f);
                    float dist = std::sqrt(dx * dx + dy * dy);

                    // Water contribution decreases with distance (max distance = 5 * pond tile size)
                    float waterFactor = std::max(0.f, 1.f - dist / (ptile.getSize().x * 5.f));

                    // Take the maximum contribution from all pond tiles
                    targetWater = std::max(targetWater, waterFactor * cropType.optimalWater);
                }
            }

            // Smoothly approach target water level
            float waterSpeed = 0.5f;  // rate per second
            tile.waterLevel += (targetWater - tile.waterLevel) * waterSpeed * dt;

            // Evaporation
            float evaporation = 0.01f * dt;
            tile.waterLevel = std::clamp(tile.waterLevel - evaporation, 0.f, 1.f);

            // ---------------- Soil Quality ----------------
            tile.soilQuality = computeSoilQuality(tile, cropType);

            // ---------------- Water Stress ----------------
            float waterDiff = tile.waterLevel - cropType.optimalWater;
            float waterStress = std::exp(-(waterDiff * waterDiff) / (2.f * cropType.tolerance * cropType.tolerance));
            waterStress = std::clamp(waterStress, 0.f, 1.f);

            // ---------------- Growth ----------------
            float growthRate = Config::growthSpeed * cropType.baseGrowthRate * tile.soilQuality * waterStress;
            growthRate *= dist(rng);  // add variability

            tile.crop.growth += growthRate * dt;
            tile.crop.growth = std::clamp(tile.crop.growth, 0.f, 1.f);

            // Record time of maturity
            if (tile.crop.growth >= 1.f && tile.timeToMature < 0.f) {
                tile.timeToMature = simTime;
            }

            // ---------------- Visual Scaling ----------------
            sf::Vector2f size =
                tile.crop.originalSize * (Config::cropInitialScale + (Config::cropMaxScale - Config::cropInitialScale) * tile.crop.growth);
            tile.crop.shape.setSize(size);
            tile.crop.shape.setPosition(tile.position.x + (tile.size - size.x) / 2, tile.position.y + (tile.size - size.y) / 2);

            // ---------------- Color Adjustment ----------------
            sf::Color targetColor = cropType.baseColor;

            // Darken fully grown crops slightly
            if (tile.crop.growth >= 1.f) {
                float darkFactor = 0.5f + 0.5f * (1.f - tile.crop.growth);
                float h, s, l;
                ColorUtils::RGBtoHSL(targetColor, h, s, l);
                l *= darkFactor;
                targetColor = ColorUtils::HSLtoRGB(h, s, l);
            }

            tile.crop.shape.setFillColor(targetColor);
        }
    }

    float getCropGrowthPercentage() const {
        int total = 0, grown = 0;
        for (auto &tile : tiles)
            if (tile.hasCrop) {
                total++;
                if (tile.crop.growth >= 1.f) grown++;
            }
        if (total == 0) return 0.f;
        return (float)grown / total * 100.f;
    }

    void draw(sf::RenderWindow &window, const CropType &cropType) {
        // window.draw(shape);
        for (auto &tile : tiles) {
            // std::cout << tile.position.x << " " << tile.position.y << std::endl;
            sf::RectangleShape rect(sf::Vector2f(tile.size, tile.size));
            rect.setPosition(tile.position);

            rect.setTexture(&farmlandTexture);
            rect.setTextureRect(sf::IntRect(0, 0, static_cast<int>(tile.size), static_cast<int>(tile.size)));
            
                
            window.draw(rect);
            if(tile.hasCrop ){    
                window.draw(tile.crop.shape);}
                if(tile.crop.growth >= 1.f){                    
                    if(tile.cropType.name == "Barley")
                        tile.crop.shape.setTexture(&wheatTexture);
                    else if(tile.cropType.name == "Tomato")
                        tile.crop.shape.setTexture(&tomatoTexture);
                    else if(tile.cropType.name == "Sugarcane")
                        tile.crop.shape.setTexture(&sugarcaneTexture);
                }
            }
            
    }
};
sf::Texture Land::wheatTexture;
sf::Texture Land::sugarcaneTexture;
sf::Texture Land::tomatoTexture;

}  // namespace Harvestor

#endif