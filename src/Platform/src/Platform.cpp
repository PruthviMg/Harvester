#include "Platform.hpp"

namespace Harvestor
{
    Platform::Platform(const size_t &rows, const size_t &cols, const float &gridSize)
    {
        grids_.resize(rows);
        for (size_t i = 0; i < rows; ++i)
        {
            grids_[i].reserve(cols);
            for (size_t j = 0; j < cols; ++j)
            {
                grids_[i].emplace_back(sf::Vector2f(j * gridSize, i * gridSize), gridSize);
            }
        }
    }

    Platform::~Platform()
    {
    }

    float Platform::getSoilQualityAt(const int &row, const int &col)
    {
        return grids_[row][col].getQuality();
    }

    void Platform::update(const float &dt)
    {
        for (auto &row : grids_)
            for (auto &grid : row)
                grid.update(dt);
    }

    Grid &Platform::getGrid(int row, int col) { return grids_[row][col]; }

    void Platform::draw(sf::RenderStates states) const
    {
        for (const auto &row : grids_)
        {
            for (const auto &grid : row)
            {
                Game::window_.draw(grid, states);
            }
        }
    }
} // namespace Harvestor