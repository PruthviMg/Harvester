import pandas as pd
import numpy as np

# -----------------------------
# Generate Soil Data (100 tiles)
# -----------------------------
np.random.seed(42)  # for reproducibility
num_tiles = 10000

soil_data = pd.DataFrame({
    'x': np.repeat(np.arange(100), 100),  # 10x10 grid
    'y': np.tile(np.arange(100), 100),
    'soilBaseQuality': np.round(np.random.uniform(0.3, 0.9, num_tiles), 2),
    'sunlight': np.round(np.random.uniform(0.4, 1.0, num_tiles), 2),
    'nutrients': np.round(np.random.uniform(0.3, 0.9, num_tiles), 2),
    'pH': np.round(np.random.uniform(0.2, 0.7, num_tiles), 2),
    'organicMatter': np.round(np.random.uniform(0.05, 0.6, num_tiles), 2),
    'compaction': np.round(np.random.uniform(0.2, 0.95, num_tiles), 2),
    'salinity': np.round(np.random.uniform(0.1, 0.85, num_tiles), 2),
})

soil_data.to_csv("soil_data.csv", index=False)
print("soil_data.csv created with 100 rows.")

# -----------------------------
# Generate Crop Data (100 tiles)
# -----------------------------
crops = ['Barley', 'Wheat', 'Corn', 'Soy', 'Rice']

crop_data = pd.DataFrame({
    'LandIndex': np.arange(1, num_tiles+1),
    'TileX': soil_data['x'],
    'TileY': soil_data['y'],
    'CropName': np.random.choice(crops, num_tiles),
    'Growth': np.round(np.random.uniform(0.5, 1.0, num_tiles), 2),
    'TimeToMature': np.round(np.random.uniform(60, 90, num_tiles), 1),
    'SoilQuality': np.round(np.random.uniform(0.3, 0.9, num_tiles), 2)
})

crop_data.to_csv("crop_data.csv", index=False)
print("crop_data.csv created with 100 rows.")
