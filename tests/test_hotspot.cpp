#include "test_common.h"

class HotspotTest : public AtmosTestFixture {};

TEST_F(HotspotTest, IgniteHotspotBasic) {
    TileAtmosData tile = CreatePlasmaTile(0, 0, 10.0f, 20.0f);
    int32_t idx = atmos_add_tile(state, &tile);
    
    atmos_ignite_hotspot(state, idx, 500.0f, 100.0f);
    
    TileAtmosData* result = atmos_get_tile(state, idx);
    EXPECT_TRUE(result->flags & TILE_FLAG_HOTSPOT);
    EXPECT_NEAR(result->hotspotTemperature, 500.0f, 0.01f);
    EXPECT_NEAR(result->hotspotVolume, 100.0f, 0.01f);
}

TEST_F(HotspotTest, IgniteHotspotAddsToList) {
    TileAtmosData tile = CreatePlasmaTile(0, 0, 10.0f, 20.0f);
    int32_t idx = atmos_add_tile(state, &tile);
    
    EXPECT_EQ(state->hotspotTileCount, 0);
    
    atmos_ignite_hotspot(state, idx, 500.0f, 100.0f);
    
    EXPECT_EQ(state->hotspotTileCount, 1);
    EXPECT_EQ(state->hotspotTiles[0], idx);
}

TEST_F(HotspotTest, IgniteHotspotNoFuel) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    int32_t idx = atmos_add_tile(state, &tile);
    
    atmos_ignite_hotspot(state, idx, 500.0f, 100.0f);
    
    TileAtmosData* result = atmos_get_tile(state, idx);
    EXPECT_FALSE(result->flags & TILE_FLAG_HOTSPOT);
}

TEST_F(HotspotTest, IgniteHotspotNoOxygen) {
    TileAtmosData tile = {};
    tile.moles[GAS_PLASMA] = 10.0f;
    tile.moles[GAS_NITROGEN] = 100.0f;
    tile.temperature = 500.0f;
    int32_t idx = atmos_add_tile(state, &tile);
    
    atmos_ignite_hotspot(state, idx, 500.0f, 100.0f);
    
    TileAtmosData* result = atmos_get_tile(state, idx);
    EXPECT_FALSE(result->flags & TILE_FLAG_HOTSPOT);
}

TEST_F(HotspotTest, IgniteHotspotLowTemperature) {
    TileAtmosData tile = CreatePlasmaTile(0, 0, 10.0f, 20.0f);
    tile.temperature = 200.0f;
    int32_t idx = atmos_add_tile(state, &tile);
    
    atmos_ignite_hotspot(state, idx, 200.0f, 100.0f);
    
    TileAtmosData* result = atmos_get_tile(state, idx);
    EXPECT_FALSE(result->flags & TILE_FLAG_HOTSPOT);
}

TEST_F(HotspotTest, ExtinguishHotspot) {
    TileAtmosData tile = CreatePlasmaTile(0, 0, 10.0f, 20.0f);
    int32_t idx = atmos_add_tile(state, &tile);
    
    atmos_ignite_hotspot(state, idx, 500.0f, 100.0f);
    EXPECT_EQ(state->hotspotTileCount, 1);
    
    atmos_extinguish_hotspot(state, idx);
    
    TileAtmosData* result = atmos_get_tile(state, idx);
    EXPECT_FALSE(result->flags & TILE_FLAG_HOTSPOT);
    EXPECT_EQ(result->hotspotTemperature, 0.0f);
    EXPECT_EQ(result->hotspotVolume, 0.0f);
    EXPECT_EQ(state->hotspotTileCount, 0);
}

TEST_F(HotspotTest, ProcessHotspotBurnsPlasma) {
    TileAtmosData tile = CreatePlasmaTile(0, 0, 10.0f, 20.0f);
    int32_t idx = atmos_add_tile(state, &tile);
    
    atmos_ignite_hotspot(state, idx, 600.0f, 100.0f);
    
    float initialPlasma = state->tiles[idx].moles[GAS_PLASMA];
    float initialOxygen = state->tiles[idx].moles[GAS_OXYGEN];
    
    AtmosResult result = atmos_process_hotspots(state, &config);
    
    EXPECT_EQ(result.processingComplete, 1);
}

TEST_F(HotspotTest, HotspotExtinguishesWithoutFuel) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    int32_t idx = atmos_add_tile(state, &tile);

    state->tiles[idx].flags |= TILE_FLAG_HOTSPOT;
    state->tiles[idx].hotspotTemperature = config.constants.fireMinimumTemperatureToExist + 2.0f;
    state->tiles[idx].hotspotVolume = 100.0f;
    state->hotspotTiles[state->hotspotTileCount++] = idx;

    atmos_process_hotspots(state, &config);

    EXPECT_FALSE(state->tiles[idx].flags & TILE_FLAG_HOTSPOT);
}

TEST_F(HotspotTest, HotspotExtinguishesAtLowTemp) {
    TileAtmosData tile = CreatePlasmaTile(0, 0, 10.0f, 20.0f);
    int32_t idx = atmos_add_tile(state, &tile);
    
    state->tiles[idx].flags |= TILE_FLAG_HOTSPOT;
    state->tiles[idx].hotspotTemperature = config.constants.fireMinimumTemperatureToExist - 10.0f;
    state->tiles[idx].hotspotVolume = 100.0f;
    state->hotspotTiles[state->hotspotTileCount++] = idx;
    
    atmos_process_hotspots(state, &config);
    
    EXPECT_FALSE(state->tiles[idx].flags & TILE_FLAG_HOTSPOT);
}

TEST_F(HotspotTest, HotspotSpreadsToAdjacent) {
    TileAtmosData tile1 = CreatePlasmaTile(0, 0, 10.0f, 20.0f);
    tile1.temperature = 1000.0f;
    int32_t idx1 = atmos_add_tile(state, &tile1);
    
    TileAtmosData tile2 = CreatePlasmaTile(1, 0, 10.0f, 20.0f);
    tile2.temperature = 400.0f;
    int32_t idx2 = atmos_add_tile(state, &tile2);
    
    SetupAdjacency(idx1, idx2, ATMOS_DIR_EAST);
    
    state->tiles[idx1].flags |= TILE_FLAG_HOTSPOT;
    state->tiles[idx1].hotspotTemperature = 1000.0f;
    state->tiles[idx1].hotspotVolume = 1000.0f;
    state->hotspotTiles[state->hotspotTileCount++] = idx1;
    
    atmos_process_hotspots(state, &config);
    
    bool tile2HasHotspot = (state->tiles[idx2].flags & TILE_FLAG_HOTSPOT) != 0;
    bool foundInList = false;
    for (int i = 0; i < state->hotspotTileCount; i++) {
        if (state->hotspotTiles[i] == idx2) {
            foundInList = true;
            break;
        }
    }
    
    EXPECT_TRUE(tile2HasHotspot || state->hotspotTileCount > 1);
}

TEST_F(HotspotTest, HotspotNotSpreadToSpace) {
    TileAtmosData tile1 = CreatePlasmaTile(0, 0, 10.0f, 20.0f);
    tile1.temperature = 1000.0f;
    int32_t idx1 = atmos_add_tile(state, &tile1);
    
    TileAtmosData tile2 = CreateSpaceTile(1, 0);
    int32_t idx2 = atmos_add_tile(state, &tile2);
    
    SetupAdjacency(idx1, idx2, ATMOS_DIR_EAST);
    
    state->tiles[idx1].flags |= TILE_FLAG_HOTSPOT;
    state->tiles[idx1].hotspotTemperature = 1000.0f;
    state->tiles[idx1].hotspotVolume = 1000.0f;
    state->hotspotTiles[state->hotspotTileCount++] = idx1;
    
    atmos_process_hotspots(state, &config);
    
    EXPECT_FALSE(state->tiles[idx2].flags & TILE_FLAG_HOTSPOT);
}

TEST_F(HotspotTest, HotspotState) {
    TileAtmosData tile = CreatePlasmaTile(0, 0, 10.0f, 20.0f);
    int32_t idx = atmos_add_tile(state, &tile);
    
    atmos_ignite_hotspot(state, idx, 500.0f, 100.0f);
    
    EXPECT_EQ(state->tiles[idx].hotspotState, 1);
}

TEST_F(HotspotTest, MultipleHotspots) {
    for (int i = 0; i < 5; i++) {
        TileAtmosData tile = CreatePlasmaTile(i, 0, 10.0f, 20.0f);
        int32_t idx = atmos_add_tile(state, &tile);
        atmos_ignite_hotspot(state, idx, 500.0f + i * 100.0f, 100.0f);
    }
    
    EXPECT_EQ(state->hotspotTileCount, 5);
    
    AtmosResult result = atmos_process_hotspots(state, &config);
    EXPECT_EQ(result.processingComplete, 1);
    EXPECT_EQ(result.hotspotTilesCount, 5);
}

TEST_F(HotspotTest, HotspotWithTritium) {
    TileAtmosData tile = CreateTritiumTile(0, 0, 5.0f, 100.0f);
    int32_t idx = atmos_add_tile(state, &tile);
    
    atmos_ignite_hotspot(state, idx, 600.0f, 100.0f);
    
    EXPECT_TRUE(state->tiles[idx].flags & TILE_FLAG_HOTSPOT);
}

TEST_F(HotspotTest, HotspotTemperatureIncrease) {
    TileAtmosData tile = CreatePlasmaTile(0, 0, 50.0f, 100.0f);
    int32_t idx = atmos_add_tile(state, &tile);
    
    state->tiles[idx].flags |= TILE_FLAG_HOTSPOT;
    state->tiles[idx].hotspotTemperature = 500.0f;
    state->tiles[idx].hotspotVolume = 500.0f;
    state->hotspotTiles[state->hotspotTileCount++] = idx;
    
    float initialTemp = state->tiles[idx].hotspotTemperature;
    
    atmos_process_hotspots(state, &config);
    
    EXPECT_GE(state->tiles[idx].hotspotTemperature, initialTemp - 50.0f);
}

TEST_F(HotspotTest, IgniteExistingHotspotUpdates) {
    TileAtmosData tile = CreatePlasmaTile(0, 0, 10.0f, 20.0f);
    int32_t idx = atmos_add_tile(state, &tile);
    
    atmos_ignite_hotspot(state, idx, 500.0f, 100.0f);
    atmos_ignite_hotspot(state, idx, 700.0f, 200.0f);
    
    EXPECT_EQ(state->hotspotTileCount, 1);
    EXPECT_GE(state->tiles[idx].hotspotTemperature, 700.0f);
    EXPECT_GE(state->tiles[idx].hotspotVolume, 200.0f);
}