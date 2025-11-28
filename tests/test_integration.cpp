#include "test_common.h"

class IntegrationTest : public AtmosTestFixture {
protected:
    void SetUp() override {
        AtmosTestFixture::SetUp();
        config.monstermosEnabled = 1;
        config.excitedGroupsEnabled = 1;
        config.superconductionEnabled = 1;
    }
};

TEST_F(IntegrationTest, FullProcessingCycle) {
    SetupSquareGrid(10, 10);
    
    state->tiles[55].moles[GAS_OXYGEN] = 200.0f;
    state->tiles[55].moles[GAS_NITROGEN] = 800.0f;
    
    atmos_add_active_tile(state, 55);
    
    AtmosResult result = atmos_process(state, &config);
    
    EXPECT_EQ(result.processingComplete, 1);
    EXPECT_GT(result.activeTilesCount, 0);
}

TEST_F(IntegrationTest, MultipleProcessingCycles) {
    SetupSquareGrid(10, 10);
    
    state->tiles[0].moles[GAS_OXYGEN] = 500.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 2000.0f;
    
    float totalMolesBefore = 0.0f;
    for (int i = 0; i < 100; i++) {
        totalMolesBefore += GetTotalMoles(&state->tiles[i]);
    }
    
    atmos_add_active_tile(state, 0);
    
    for (int cycle = 0; cycle < 100; cycle++) {
        AtmosResult result = atmos_process(state, &config);
        EXPECT_EQ(result.processingComplete, 1);
    }
    
    float totalMolesAfter = 0.0f;
    for (int i = 0; i < 100; i++) {
        totalMolesAfter += GetTotalMoles(&state->tiles[i]);
    }
    
    EXPECT_NEAR(totalMolesAfter, totalMolesBefore, totalMolesBefore * 0.01f);
}

TEST_F(IntegrationTest, FireSpreadAndBurn) {
    SetupLinearGrid(5);
    
    for (int i = 0; i < 5; i++) {
        state->tiles[i].moles[GAS_OXYGEN] = 30.0f;
        state->tiles[i].moles[GAS_NITROGEN] = 0.0f;
        state->tiles[i].moles[GAS_PLASMA] = 20.0f;
        state->tiles[i].temperature = config.constants.plasmaMinimumBurnTemperature + 50.0f;
    }
    
    atmos_ignite_hotspot(state, 0, 600.0f, 100.0f);
    
    float initialPlasma = 0.0f;
    for (int i = 0; i < 5; i++) {
        initialPlasma += state->tiles[i].moles[GAS_PLASMA];
    }
    
    for (int cycle = 0; cycle < 50; cycle++) {
        AtmosResult result = atmos_process(state, &config);
    }
    
    float finalPlasma = 0.0f;
    for (int i = 0; i < 5; i++) {
        finalPlasma += state->tiles[i].moles[GAS_PLASMA];
    }
    
    EXPECT_LT(finalPlasma, initialPlasma);
}

TEST_F(IntegrationTest, SpaceVentingScenario) {
    SetupLinearGrid(10);
    
    TileAtmosData spaceTile = CreateSpaceTile(10, 0);
    int32_t spaceIdx = atmos_add_tile(state, &spaceTile);
    SetupAdjacency(9, spaceIdx, ATMOS_DIR_EAST);
    
    float totalMolesBefore = 0.0f;
    for (int i = 0; i < 10; i++) {
        totalMolesBefore += GetTotalMoles(&state->tiles[i]);
    }
    
    for (int i = 0; i < 10; i++) {
        atmos_add_active_tile(state, i);
    }
    
    for (int cycle = 0; cycle < 50; cycle++) {
        AtmosResult result = atmos_process(state, &config);
    }
    
    float totalMolesAfter = 0.0f;
    for (int i = 0; i < 10; i++) {
        totalMolesAfter += GetTotalMoles(&state->tiles[i]);
    }
    
    EXPECT_LT(totalMolesAfter, totalMolesBefore);
}

TEST_F(IntegrationTest, TemperatureEqualization) {
    SetupLinearGrid(5);
    
    state->tiles[0].temperature = 1000.0f;
    state->tiles[4].temperature = 200.0f;
    
    for (int i = 0; i < 5; i++) {
        atmos_add_active_tile(state, i);
    }
    
    for (int cycle = 0; cycle < 200; cycle++) {
        AtmosResult result = atmos_process(state, &config);
    }
    
    float minTemp = state->tiles[0].temperature;
    float maxTemp = state->tiles[0].temperature;
    for (int i = 1; i < 5; i++) {
        if (state->tiles[i].temperature < minTemp) minTemp = state->tiles[i].temperature;
        if (state->tiles[i].temperature > maxTemp) maxTemp = state->tiles[i].temperature;
    }
    
    EXPECT_LT(maxTemp - minTemp, 200.0f);
}

TEST_F(IntegrationTest, ComplexRoomConfiguration) {
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            TileAtmosData tile;
            if (x == 0 || x == 9 || y == 0 || y == 9) {
                tile = CreateSpaceTile(x, y);
            } else {
                tile = CreateStandardTile(x, y);
            }
            atmos_add_tile(state, &tile);
        }
    }
    
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            int idx = y * 10 + x;
            if (x < 9) {
                int rightIdx = y * 10 + (x + 1);
                if (!(state->tiles[idx].flags & TILE_FLAG_SPACE) || 
                    !(state->tiles[rightIdx].flags & TILE_FLAG_SPACE)) {
                    SetupAdjacency(idx, rightIdx, ATMOS_DIR_EAST);
                }
            }
            if (y < 9) {
                int bottomIdx = (y + 1) * 10 + x;
                if (!(state->tiles[idx].flags & TILE_FLAG_SPACE) || 
                    !(state->tiles[bottomIdx].flags & TILE_FLAG_SPACE)) {
                    SetupAdjacency(idx, bottomIdx, ATMOS_DIR_SOUTH);
                }
            }
        }
    }
    
    state->tiles[55].moles[GAS_OXYGEN] = 100.0f;
    state->tiles[55].moles[GAS_NITROGEN] = 400.0f;
    
    atmos_add_active_tile(state, 55);
    
    for (int cycle = 0; cycle < 100; cycle++) {
        AtmosResult result = atmos_process(state, &config);
    }
    
    bool foundHighPressure = false;
    for (int y = 1; y < 9; y++) {
        for (int x = 1; x < 9; x++) {
            int idx = y * 10 + x;
            if (GetTotalMoles(&state->tiles[idx]) > 10.0f) {
                foundHighPressure = true;
            }
        }
    }
    
    EXPECT_TRUE(foundHighPressure);
}

TEST_F(IntegrationTest, ReactionChain) {
    TileAtmosData tile = {};
    tile.moles[GAS_PLASMA] = 50.0f;
    tile.moles[GAS_OXYGEN] = 100.0f;
    tile.temperature = config.constants.plasmaUpperTemperature;
    tile.gridX = 0;
    tile.gridY = 0;
    tile.thermalConductivity = 0.5f;
    tile.heatCapacity = 10000.0f;
    for (int i = 0; i < ATMOS_DIRECTIONS; i++)
        tile.adjacentIndices[i] = -1;
    tile.excitedGroupId = -1;
    
    int32_t idx = atmos_add_tile(state, &tile);
    atmos_add_active_tile(state, idx);
    atmos_ignite_hotspot(state, idx, 600.0f, 500.0f);
    
    float initialPlasma = state->tiles[idx].moles[GAS_PLASMA];
    
    for (int cycle = 0; cycle < 100; cycle++) {
        AtmosResult result = atmos_process(state, &config);
    }
    
    EXPECT_LT(state->tiles[idx].moles[GAS_PLASMA], initialPlasma);
    EXPECT_GT(state->tiles[idx].moles[GAS_CO2], 0.0f);
    EXPECT_GT(state->tiles[idx].moles[GAS_WATER_VAPOR], 0.0f);
}

TEST_F(IntegrationTest, SuperconductionWithFire) {
    SetupLinearGrid(5);
    
    for (int i = 0; i < 5; i++) {
        state->tiles[i].thermalConductivity = 0.8f;
        state->tiles[i].heatCapacity = 10000.0f;
    }
    
    state->tiles[0].moles[GAS_PLASMA] = 20.0f;
    state->tiles[0].moles[GAS_OXYGEN] = 40.0f;
    state->tiles[0].temperature = 800.0f;
    
    atmos_ignite_hotspot(state, 0, 800.0f, 500.0f);
    
    for (int i = 0; i < 5; i++) {
        atmos_add_active_tile(state, i);
    }
    
    for (int cycle = 0; cycle < 50; cycle++) {
        AtmosResult result = atmos_process(state, &config);
    }
    
    EXPECT_GT(state->tiles[4].temperature, config.constants.T20C);
}

TEST_F(IntegrationTest, LargeScaleEqualization) {
    SetupSquareGrid(20, 20);
    
    state->tiles[0].moles[GAS_OXYGEN] = 1000.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 4000.0f;
    
    state->tiles[399].moles[GAS_OXYGEN] = 1.0f;
    state->tiles[399].moles[GAS_NITROGEN] = 4.0f;
    
    float totalBefore = 0.0f;
    for (int i = 0; i < 400; i++) {
        totalBefore += GetTotalMoles(&state->tiles[i]);
    }
    
    atmos_add_active_tile(state, 0);
    atmos_add_active_tile(state, 399);
    
    for (int cycle = 0; cycle < 200; cycle++) {
        AtmosResult result = atmos_process(state, &config);
    }
    
    float totalAfter = 0.0f;
    for (int i = 0; i < 400; i++) {
        totalAfter += GetTotalMoles(&state->tiles[i]);
    }
    
    EXPECT_NEAR(totalAfter, totalBefore, totalBefore * 0.01f);
}

TEST_F(IntegrationTest, MixedGasEqualization) {
    SetupLinearGrid(3);
    
    state->tiles[0].moles[GAS_OXYGEN] = 50.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 0.0f;
    state->tiles[0].moles[GAS_CO2] = 0.0f;
    
    state->tiles[1].moles[GAS_OXYGEN] = 0.0f;
    state->tiles[1].moles[GAS_NITROGEN] = 50.0f;
    state->tiles[1].moles[GAS_CO2] = 0.0f;
    
    state->tiles[2].moles[GAS_OXYGEN] = 0.0f;
    state->tiles[2].moles[GAS_NITROGEN] = 0.0f;
    state->tiles[2].moles[GAS_CO2] = 50.0f;
    
    for (int i = 0; i < 3; i++) {
        atmos_add_active_tile(state, i);
    }
    
    for (int cycle = 0; cycle < 100; cycle++) {
        AtmosResult result = atmos_process(state, &config);
    }
    
    for (int i = 0; i < 3; i++) {
        EXPECT_GT(state->tiles[i].moles[GAS_OXYGEN], 10.0f);
        EXPECT_GT(state->tiles[i].moles[GAS_NITROGEN], 10.0f);
        EXPECT_GT(state->tiles[i].moles[GAS_CO2], 10.0f);
    }
}

TEST_F(IntegrationTest, StabilityTest) {
    SetupSquareGrid(5, 5);
    
    for (int i = 0; i < 25; i++) {
        atmos_add_active_tile(state, i);
    }
    
    for (int cycle = 0; cycle < 1000; cycle++) {
        AtmosResult result = atmos_process(state, &config);
        EXPECT_EQ(result.processingComplete, 1);
        
        for (int i = 0; i < 25; i++) {
            EXPECT_GE(state->tiles[i].temperature, config.constants.TCMB);
            EXPECT_LE(state->tiles[i].temperature, config.constants.Tmax);
            
            for (int g = 0; g < ATMOS_GAS_COUNT; g++) {
                EXPECT_GE(state->tiles[i].moles[g], 0.0f);
            }
        }
    }
}