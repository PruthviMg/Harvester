#ifndef GRASS_MANAGER_HPP_
#define GRASS_MANAGER_HPP_

#include "config.hpp"

namespace Harvestor {
// ---------------- GrassManager ----------------
class GrassManager {
    sf::Texture &texture;
    float tileSize;
    std::vector<sf::Sprite> grassSprites;

   public:
    GrassManager(sf::Texture &tex, float tileSize) : texture(tex), tileSize(tileSize) {}
    void generate(float width, float height) {
        grassSprites.clear();
        for (float y = 0; y < height; y += tileSize)
            for (float x = Config::uiPanelWidth; x < width; x += tileSize) {
                sf::Sprite sprite;
                sprite.setTexture(texture);
                sf::Vector2u texSize = texture.getSize();
                int bottomHeight = texSize.y * 10 / 100;
                sprite.setTextureRect(sf::IntRect(0, texSize.y - bottomHeight, texSize.x, bottomHeight));
                sprite.setScale(tileSize / float(texSize.x), tileSize / float(bottomHeight));
                sprite.setPosition(x + rand() % 2, y + rand() % 2);
                grassSprites.push_back(sprite);
            }
    }
    void draw(sf::RenderWindow &window) {
        for (auto &g : grassSprites) window.draw(g);
    }
};
}  // namespace Harvestor

#endif