#!/bin/bash
set -euo pipefail  # safer scripting

# Colors
GREEN="\033[0;32m"
BLUE="\033[0;34m"
YELLOW="\033[1;33m"
RED="\033[0;31m"
NC="\033[0m" # No Color

BUILD_DIR="build"

echo -e "${BLUE}📍 Step 1: Generating map...${NC}"
(
  cd report
  python3 map_generator.py
)
echo -e "${GREEN}✅ Map generated successfully.${NC}\n"

echo -e "${BLUE}📍 Step 2: Preparing build directory...${NC}"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
echo -e "${GREEN}✅ Build directory ready.${NC}\n"

echo -e "${BLUE}📍 Step 3: Running CMake...${NC}"
cmake ..
echo -e "${GREEN}✅ CMake configuration done.${NC}\n"

echo -e "${BLUE}📍 Step 4: Compiling (using $(nproc) cores)...${NC}"
make -j"$(nproc)"
echo -e "${GREEN}✅ Build completed.${NC}\n"

echo -e "${YELLOW}🚀 Step 5: Running Harvestor...${NC}"
./Harvestor
echo -e "${GREEN}✅ Execution finished.${NC}"
