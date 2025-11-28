#pragma once

#include <gtest/gtest.h>
#include "atmos_api.h"
#include "atmos_internal.h"
#include <cmath>
#include <chrono>
#include <random>

class AtmosTestFixture : public ::testing::Test {
protected:
    GridAtmosState* state = nullptr;
    AtmosConfig config;

    void SetUp() override {
        state = atmos_create_grid(256);
        atmos_config_init_default(&config);
    }

    void TearDown() override {
        if (state) {
            atmos_destroy_grid(state);
            state = nullptr;
        }
    }

    TileAtmosData CreateStandardTile(int x, int y) {
        TileAtmosData tile = {};
        tile.gridX = x;
        tile.gridY = y;
        tile.temperature = config.constants.T20C;
        tile.moles[GAS_OXYGEN] = 21.0f;
        tile.moles[GAS_NITROGEN] = 79.0f;
        tile.thermalConductivity = 0.5f;
        tile.heatCapacity = 10000.0f;
        for (int i = 0; i < ATMOS_DIRECTIONS; i++)
            tile.adjacentIndices[i] = -1;
        tile.excitedGroupId = -1;
        tile.currentTransferDirection = -1;
        return tile;
    }

    TileAtmosData CreateSpaceTile(int x, int y) {
        TileAtmosData tile = {};
        tile.gridX = x;
        tile.gridY = y;
        tile.temperature = config.constants.TCMB;
        tile.flags = TILE_FLAG_SPACE | TILE_FLAG_IMMUTABLE;
        tile.thermalConductivity = 0.0f;
        tile.heatCapacity = config.constants.spaceHeatCapacity;
        for (int i = 0; i < ATMOS_DIRECTIONS; i++)
            tile.adjacentIndices[i] = -1;
        tile.excitedGroupId = -1;
        tile.currentTransferDirection = -1;
        return tile;
    }

    TileAtmosData CreateHighPressureTile(int x, int y) {
        TileAtmosData tile = CreateStandardTile(x, y);
        tile.moles[GAS_OXYGEN] = 210.0f;
        tile.moles[GAS_NITROGEN] = 790.0f;
        return tile;
    }

    TileAtmosData CreateLowPressureTile(int x, int y) {
        TileAtmosData tile = CreateStandardTile(x, y);
        tile.moles[GAS_OXYGEN] = 2.1f;
        tile.moles[GAS_NITROGEN] = 7.9f;
        return tile;
    }

    TileAtmosData CreateHotTile(int x, int y, float temperature) {
        TileAtmosData tile = CreateStandardTile(x, y);
        tile.temperature = temperature;
        return tile;
    }

    TileAtmosData CreatePlasmaTile(int x, int y, float plasma, float oxygen) {
        TileAtmosData tile = CreateStandardTile(x, y);
        tile.moles[GAS_OXYGEN] = oxygen;
        tile.moles[GAS_NITROGEN] = 0.0f;
        tile.moles[GAS_PLASMA] = plasma;
        tile.temperature = config.constants.plasmaMinimumBurnTemperature + 100.0f;
        return tile;
    }

    TileAtmosData CreateTritiumTile(int x, int y, float tritium, float oxygen) {
        TileAtmosData tile = CreateStandardTile(x, y);
        tile.moles[GAS_OXYGEN] = oxygen;
        tile.moles[GAS_NITROGEN] = 0.0f;
        tile.moles[GAS_TRITIUM] = tritium;
        tile.temperature = config.constants.plasmaMinimumBurnTemperature + 100.0f;
        return tile;
    }

    void SetupAdjacency(int32_t tile1, int32_t tile2, int direction) {
        atmos_set_adjacency(state, tile1, direction, tile2);
        atmos_set_adjacency(state, tile2, opposite_dir(direction), tile1);
    }

    void SetupLinearGrid(int count) {
        for (int i = 0; i < count; i++) {
            TileAtmosData tile = CreateStandardTile(i, 0);
            atmos_add_tile(state, &tile);
        }
        for (int i = 0; i < count - 1; i++) {
            SetupAdjacency(i, i + 1, ATMOS_DIR_EAST);
        }
    }

    void SetupSquareGrid(int width, int height) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                TileAtmosData tile = CreateStandardTile(x, y);
                atmos_add_tile(state, &tile);
            }
        }
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = y * width + x;
                if (x < width - 1)
                    SetupAdjacency(idx, idx + 1, ATMOS_DIR_EAST);
                if (y < height - 1)
                    SetupAdjacency(idx, idx + width, ATMOS_DIR_SOUTH);
            }
        }
    }

    float GetTotalMoles(const TileAtmosData* tile) {
        return tile_total_moles(tile);
    }

    float GetPressure(const TileAtmosData* tile) {
        return tile_pressure(tile, config.constants.R, config.constants.cellVolume);
    }

    bool FloatNear(float a, float b, float epsilon = 0.001f) {
        return std::abs(a - b) < epsilon;
    }

    bool FloatRelativeNear(float a, float b, float relEpsilon = 0.01f) {
        if (std::abs(a) < 0.0001f && std::abs(b) < 0.0001f)
            return true;
        float maxVal = std::max(std::abs(a), std::abs(b));
        return std::abs(a - b) / maxVal < relEpsilon;
    }
};

class PerformanceTimer {
public:
    void Start() {
        startTime = std::chrono::high_resolution_clock::now();
    }

    double ElapsedMs() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(now - startTime).count();
    }

    double ElapsedUs() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::micro>(now - startTime).count();
    }

private:
    std::chrono::high_resolution_clock::time_point startTime;
};