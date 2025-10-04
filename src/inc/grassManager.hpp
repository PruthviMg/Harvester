#ifndef GRASS_MANAGER_HPP_
#define GRASS_MANAGER_HPP_

#include "config.hpp"

namespace Harvestor {
// ---------------- GrassManager ----------------
class GrassManager {
    sf::Texture texture;
    float tileSize;
    std::vector<sf::Sprite> grassSprites;

   public:
    GrassManager(const std::string &filePath, float tileSize) : tileSize(tileSize) {
        sf::Image img;
        if (!img.loadFromFile(filePath)) {
            std::cerr << "Failed to load grass texture from " << filePath << std::endl;
            return;
        }

        // Crop left 10% of the texture
        unsigned int cropWidth = img.getSize().x / 10;  // 10% width
        unsigned int cropHeight = img.getSize().y;      // full height
        sf::IntRect cropRect(0, 0, cropWidth, cropHeight);

        sf::Image cropped;
        cropped.create(cropWidth, cropHeight, sf::Color::Transparent);
        cropped.copy(img, 0, 0, cropRect, true);

        if (!texture.loadFromImage(cropped)) {
            std::cerr << "Failed to load cropped texture!" << std::endl;
        }

        texture.setRepeated(true);
        texture.setSmooth(true);
    }

    void generate(float width, float height) {
        grassSprites.clear();
        for (float y = 0; y < height; y += tileSize) {
            for (float x = Config::uiPanelWidth; x < width; x += tileSize) {
                sf::Sprite sprite;
                sprite.setTexture(texture);

                // Use full texture (cropped left 10%)
                sf::Vector2u texSize = texture.getSize();
                sprite.setTextureRect(sf::IntRect(0, 0, texSize.x, texSize.y));
                sprite.setScale(tileSize / float(texSize.x), tileSize / float(texSize.y));
                sprite.setPosition(x + rand() % 2, y + rand() % 2);

                grassSprites.push_back(sprite);
            }
        }
    }

    void draw(sf::RenderWindow &window) {
        for (auto &g : grassSprites) window.draw(g);
    }
};

}  // namespace Harvestor

#endif