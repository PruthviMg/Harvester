import cv2
import numpy as np
import csv
import random
import math

# --- Configuration ---
input_file = 'map_satellite.jpg'
output_file = 'map_3color.png'
land_csv = 'land.csv'
water_csv = 'water.csv'
target_csv_lines = 10000  # Max lines in CSV

# Load image
img = cv2.imread(input_file)
if img is None:
    raise ValueError("Image not found!")

height, width, _ = img.shape
total_pixels = width * height

# Compute downscale factor
scale_factor = math.ceil(math.sqrt(total_pixels / target_csv_lines))
down_width = max(1, width // scale_factor)
down_height = max(1, height // scale_factor)
print(f"Original: {width}x{height}, downscaled: {down_width}x{down_height}, scale_factor={scale_factor}")

# Downscale for CSV generation
img_small = cv2.resize(img, (down_width, down_height), interpolation=cv2.INTER_AREA)

# Convert to HSV for classification
hsv = cv2.cvtColor(img_small, cv2.COLOR_BGR2HSV)
terrain_img = np.zeros_like(img_small)

# Water mask
lower_blue = np.array([100,50,50])
upper_blue = np.array([140,255,255])
water_mask = cv2.inRange(hsv, lower_blue, upper_blue)
terrain_img[water_mask>0] = [255,0,0]

# Land mask
lower_green = np.array([40,40,40])
upper_green = np.array([80,255,255])
land_mask = cv2.inRange(hsv, lower_green, upper_green)
terrain_img[land_mask>0] = [0,255,0]

# Mountain/other
mountain_mask = cv2.bitwise_not(cv2.bitwise_or(water_mask, land_mask))
terrain_img[mountain_mask>0] = [42,42,165]

# Save pixelated 3-color map
cv2.imwrite(output_file, cv2.resize(terrain_img, (width, height), interpolation=cv2.INTER_NEAREST))
print(f"Pixelated 3-color map saved as {output_file}")

# --- Save CSV ---
with open(land_csv, 'w', newline='') as lf, open(water_csv, 'w', newline='') as wf:
    land_writer = csv.writer(lf)
    water_writer = csv.writer(wf)
    
    land_writer.writerow([
        "x", "y",
        "soilBaseQuality","sunlight","nutrients","pH",
        "organicMatter","compaction","salinity"
    ])
    water_writer.writerow(["x","y"])
    
    small_h, small_w, _ = terrain_img.shape
    for y in range(small_h):
        for x in range(small_w):
            b,g,r = terrain_img[y,x]
            if (b,g,r) == (255,0,0):  # WATER
                water_writer.writerow([x,y])
            elif (b,g,r) == (0,255,0):  # LAND
                land_writer.writerow([
                    x,y,
                    round(random.uniform(0,1),3),
                    round(random.uniform(0,1),3),
                    round(random.uniform(0,1),3),
                    round(random.uniform(0,1),3),
                    round(random.uniform(0,1),3),
                    round(random.uniform(0,1),3),
                    round(random.uniform(0,1),3)
                ])

print(f"CSV files saved: {land_csv}, {water_csv}")
