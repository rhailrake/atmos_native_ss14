#include "test_common.h"

class SuperconductivityTest : public AtmosTestFixture {
protected:
    void SetUp() override {
        AtmosTestFixture::SetUp();
        config.superconductionEnabled = 1;
    }
};

TEST_F(SuperconductivityTest, ConsiderSuperconductivityHotTile) {
    TileAtmosData tile = CreateHotTile(0, 0, config.constants.minimumTemperatureStartSuperConduction + 100.0f);
    tile.thermalConductivity = 0.5f;
    int32_t idx = atmos_add_tile(state, &tile);
    
    bool result = consider_superconductivity(state, idx, true, &config);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(state->tiles[idx].flags & TILE_FLAG_SUPERCONDUCT);
}

TEST_F(SuperconductivityTest, ConsiderSuperconductivityColdTile) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    tile.thermalConductivity = 0.5f;
    int32_t idx = atmos_add_tile(state, &tile);
    
    bool result = consider_superconductivity(state, idx, true, &config);
    
    EXPECT_FALSE(result);
    EXPECT_FALSE(state->tiles[idx].flags & TILE_FLAG_SUPERCONDUCT);
}

TEST_F(SuperconductivityTest, ConsiderSuperconductivityNoConduction) {
    TileAtmosData tile = CreateHotTile(0, 0, config.constants.minimumTemperatureStartSuperConduction + 100.0f);
    tile.thermalConductivity = 0.0f;
    int32_t idx = atmos_add_tile(state, &tile);
    
    bool result = consider_superconductivity(state, idx, true, &config);
    
    EXPECT_FALSE(result);
}

TEST_F(SuperconductivityTest, ConsiderSuperconductivityDisabled) {
    config.superconductionEnabled = 0;
    
    TileAtmosData tile = CreateHotTile(0, 0, config.constants.minimumTemperatureStartSuperConduction + 100.0f);
    tile.thermalConductivity = 0.5f;
    int32_t idx = atmos_add_tile(state, &tile);
    
    bool result = consider_superconductivity(state, idx, true, &config);
    
    EXPECT_FALSE(result);
}

TEST_F(SuperconductivityTest, SuperconductAddToList) {
    TileAtmosData tile = CreateHotTile(0, 0, config.constants.minimumTemperatureStartSuperConduction + 100.0f);
    tile.thermalConductivity = 0.5f;
    int32_t idx = atmos_add_tile(state, &tile);
    
    consider_superconductivity(state, idx, true, &config);
    
    EXPECT_EQ(state->superconductTileCount, 1);
    EXPECT_EQ(state->superconductTiles[0], idx);
}

TEST_F(SuperconductivityTest, SuperconductNoDuplicates) {
    TileAtmosData tile = CreateHotTile(0, 0, config.constants.minimumTemperatureStartSuperConduction + 100.0f);
    tile.thermalConductivity = 0.5f;
    int32_t idx = atmos_add_tile(state, &tile);
    
    consider_superconductivity(state, idx, true, &config);
    consider_superconductivity(state, idx, true, &config);
    consider_superconductivity(state, idx, true, &config);
    
    EXPECT_EQ(state->superconductTileCount, 1);
}

TEST_F(SuperconductivityTest, ProcessSuperconductivity) {
    TileAtmosData tile1 = CreateHotTile(0, 0, 1000.0f);
    tile1.thermalConductivity = 0.5f;
    tile1.heatCapacity = 10000.0f;
    int32_t idx1 = atmos_add_tile(state, &tile1);
    
    TileAtmosData tile2 = CreateStandardTile(1, 0);
    tile2.thermalConductivity = 0.5f;
    tile2.heatCapacity = 10000.0f;
    int32_t idx2 = atmos_add_tile(state, &tile2);
    
    SetupAdjacency(idx1, idx2, ATMOS_DIR_EAST);
    
    consider_superconductivity(state, idx1, true, &config);
    atmos_archive_tile(&state->tiles[idx1]);
    atmos_archive_tile(&state->tiles[idx2]);
    
    AtmosResult result = atmos_process_superconductivity(state, &config);
    
    EXPECT_EQ(result.processingComplete, 1);
}

TEST_F(SuperconductivityTest, SuperconductTransfersHeat) {
    TileAtmosData tile1 = CreateHotTile(0, 0, 1000.0f);
    tile1.thermalConductivity = 0.8f;
    tile1.heatCapacity = 10000.0f;
    int32_t idx1 = atmos_add_tile(state, &tile1);
    
    TileAtmosData tile2 = CreateStandardTile(1, 0);
    tile2.thermalConductivity = 0.8f;
    tile2.heatCapacity = 10000.0f;
    int32_t idx2 = atmos_add_tile(state, &tile2);
    
    SetupAdjacency(idx1, idx2, ATMOS_DIR_EAST);
    
    consider_superconductivity(state, idx1, true, &config);
    
    float initialTemp1 = state->tiles[idx1].temperature;
    float initialTemp2 = state->tiles[idx2].temperature;
    
    for (int i = 0; i < 10; i++) {
        atmos_archive_tile(&state->tiles[idx1]);
        atmos_archive_tile(&state->tiles[idx2]);
        atmos_process_superconductivity(state, &config);
    }
    
    EXPECT_LT(state->tiles[idx1].temperature, initialTemp1);
    EXPECT_GT(state->tiles[idx2].temperature, initialTemp2);
}

TEST_F(SuperconductivityTest, RadiateToSpace) {
    TileAtmosData tile = CreateHotTile(0, 0, 500.0f);
    tile.thermalConductivity = 0.5f;
    tile.heatCapacity = 10000.0f;
    
    float initialTemp = tile.temperature;
    
    atmos_archive_tile(&tile);
    radiate_to_space(&tile, &config);
    
    EXPECT_LT(tile.temperature, initialTemp);
}

TEST_F(SuperconductivityTest, RadiateToSpaceColdTile) {
    TileAtmosData tile = {};
    tile.temperature = config.constants.T0C - 10.0f;
    tile.thermalConductivity = 0.5f;
    tile.heatCapacity = 10000.0f;
    
    float initialTemp = tile.temperature;
    
    atmos_archive_tile(&tile);
    radiate_to_space(&tile, &config);
    
    EXPECT_NEAR(tile.temperature, initialTemp, 1.0f);
}

TEST_F(SuperconductivityTest, FinishSuperconductivityRemovesFromList) {
    TileAtmosData tile = CreateHotTile(0, 0, config.constants.minimumTemperatureStartSuperConduction + 100.0f);
    tile.thermalConductivity = 0.5f;
    int32_t idx = atmos_add_tile(state, &tile);
    
    consider_superconductivity(state, idx, true, &config);
    EXPECT_EQ(state->superconductTileCount, 1);
    
    finish_superconductivity(state, idx, config.constants.minimumTemperatureForSuperconduction - 10.0f, &config);
    
    EXPECT_EQ(state->superconductTileCount, 0);
    EXPECT_FALSE(state->tiles[idx].flags & TILE_FLAG_SUPERCONDUCT);
}

TEST_F(SuperconductivityTest, NeighborConductWithSource) {
    TileAtmosData tile1 = CreateHotTile(0, 0, 800.0f);
    tile1.thermalConductivity = 0.5f;
    tile1.heatCapacity = 10000.0f;
    int32_t idx1 = atmos_add_tile(state, &tile1);
    
    TileAtmosData tile2 = CreateStandardTile(1, 0);
    tile2.thermalConductivity = 0.5f;
    tile2.heatCapacity = 10000.0f;
    int32_t idx2 = atmos_add_tile(state, &tile2);
    
    SetupAdjacency(idx1, idx2, ATMOS_DIR_EAST);
    
    atmos_archive_tile(&state->tiles[idx1]);
    atmos_archive_tile(&state->tiles[idx2]);
    
    float initialTemp1 = state->tiles[idx1].temperature;
    
    neighbor_conduct_with_source(state, idx2, idx1, &config);
}

TEST_F(SuperconductivityTest, SuperconductivityChain) {
    for (int i = 0; i < 5; i++) {
        TileAtmosData tile = CreateStandardTile(i, 0);
        tile.thermalConductivity = 0.5f;
        tile.heatCapacity = 10000.0f;
        atmos_add_tile(state, &tile);
    }
    
    for (int i = 0; i < 4; i++) {
        SetupAdjacency(i, i + 1, ATMOS_DIR_EAST);
    }
    
    state->tiles[0].temperature = 1000.0f;
    consider_superconductivity(state, 0, true, &config);
    
    for (int cycle = 0; cycle < 50; cycle++) {
        for (int i = 0; i < 5; i++) {
            atmos_archive_tile(&state->tiles[i]);
        }
        atmos_process_superconductivity(state, &config);
    }
    
    EXPECT_LT(state->tiles[0].temperature, 1000.0f);
    EXPECT_GT(state->tiles[4].temperature, config.constants.T20C);
}

TEST_F(SuperconductivityTest, ImmutableTileNoChange) {
    TileAtmosData tile = CreateHotTile(0, 0, 1000.0f);
    tile.flags = TILE_FLAG_IMMUTABLE;
    tile.thermalConductivity = 0.5f;
    tile.heatCapacity = 10000.0f;
    
    float initialTemp = tile.temperature;
    
    atmos_archive_tile(&tile);
    radiate_to_space(&tile, &config);
    
    EXPECT_EQ(tile.temperature, initialTemp);
}

TEST_F(SuperconductivityTest, LowHeatCapacityNoSuperconduct) {
    TileAtmosData tile = CreateHotTile(0, 0, config.constants.minimumTemperatureStartSuperConduction + 100.0f);
    tile.thermalConductivity = 0.5f;
    tile.moles[GAS_OXYGEN] = 0.001f;
    tile.moles[GAS_NITROGEN] = 0.001f;
    int32_t idx = atmos_add_tile(state, &tile);
    
    bool result = consider_superconductivity(state, idx, true, &config);
    
    EXPECT_FALSE(result);
}

TEST_F(SuperconductivityTest, ContinueThreshold) {
    TileAtmosData tile = CreateHotTile(0, 0, config.constants.minimumTemperatureForSuperconduction + 10.0f);
    tile.thermalConductivity = 0.5f;
    int32_t idx = atmos_add_tile(state, &tile);
    
    bool resultStart = consider_superconductivity(state, idx, true, &config);
    EXPECT_FALSE(resultStart);
    
    bool resultContinue = consider_superconductivity(state, idx, false, &config);
    EXPECT_TRUE(resultContinue);
}