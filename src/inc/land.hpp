#ifndef LAND_HPP_
#define LAND_HPP_

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

    Land(float cx, float cy, float r, float tileSize = Config::landTileSize) : center(cx, cy), radius(r), tileSize(tileSize) {
        shape = BlobGenerator::generate(cx, cy, r, 25, 0.25f);

        // --- Load farmland texture ---
        static sf::Texture farmlandTexture;
        static bool loaded = false;
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
        }

        // Apply cropped texture
        shape.setTexture(&farmlandTexture);

        // Tile across the blob
        shape.setTextureRect(sf::IntRect(0, 0, static_cast<int>(2 * radius), static_cast<int>(2 * radius)));
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

        for (const auto &vals : soilMatrix) {
            // vals = {soilBaseQuality, sunlight, nutrients, pH, organicMatter, compaction, salinity}
            // We'll also read x,y from the file for tile position if needed

            Tile t;
            // If you stored x,y in the CSV as the first two columns:
            t.position = sf::Vector2f(vals[0], vals[1]);  // <-- adjust if x,y are separate
            t.size = Config::landTileSize;
            t.isInsideLand = true;
            t.hasCrop = false;

            // Soil attributes from CSV
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

        std::cout << "Generated " << tiles.size() << " tiles directly from soil matrix.\n";
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
        // Gradually move water level toward optimal water from nearby ponds
        float targetWater = 0.f;

        for (auto &pond : ponds) {
            float dx = tile.position.x - pond.center.x;
            float dy = tile.position.y - pond.center.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Water contribution decreases with distance
            float waterFactor = std::max(0.f, 1.f - dist / (pond.radius * 5.f));
            targetWater = std::max(targetWater, waterFactor * cropType.optimalWater);
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
                float dx = tile.position.x - pond.center.x;
                float dy = tile.position.y - pond.center.y;
                float distToPond = std::sqrt(dx * dx + dy * dy);

                // Water contribution decreases with distance
                float waterFactor = std::max(0.f, 1.f - distToPond / (pond.radius * 5.f));
                targetWater = std::max(targetWater, waterFactor * cropType.optimalWater);
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
        window.draw(shape);
        for (auto &tile : tiles)
            if (tile.hasCrop) window.draw(tile.crop.shape);
    }
};

}  // namespace Harvestor

#endif