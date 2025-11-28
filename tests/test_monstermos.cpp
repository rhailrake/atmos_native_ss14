#include "test_common.h"

class MonstermosTest : public AtmosTestFixture {
protected:
    void SetUp() override {
        AtmosTestFixture::SetUp();
        config.monstermosEnabled = 1;
        config.excitedGroupsEnabled = 1;
    }
};

TEST_F(MonstermosTest, EqualizePressureBasic) {
    SetupLinearGrid(5);
    
    state->tiles[0].moles[GAS_OXYGEN] = 100.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 400.0f;
    
    atmos_add_active_tile(state, 0);
    
    AtmosResult result = atmos_equalize_pressure_zone(state, 0, &config);
    
    EXPECT_EQ(result.processingComplete, 1);
}

TEST_F(MonstermosTest, EqualizePressureZoneHighToLow) {
    SetupLinearGrid(3);
    
    state->tiles[0].moles[GAS_OXYGEN] = 200.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 800.0f;
    
    state->tiles[2].moles[GAS_OXYGEN] = 2.0f;
    state->tiles[2].moles[GAS_NITROGEN] = 8.0f;
    
    float totalBefore = 0.0f;
    for (int i = 0; i < 3; i++) {
        totalBefore += GetTotalMoles(&state->tiles[i]);
    }
    
    state->updateCounter++;
    atmos_equalize_pressure_zone(state, 0, &config);
    
    float totalAfter = 0.0f;
    for (int i = 0; i < 3; i++) {
        totalAfter += GetTotalMoles(&state->tiles[i]);
    }
    
    EXPECT_NEAR(totalAfter, totalBefore, totalBefore * 0.01f);
}

TEST_F(MonstermosTest, EqualizePressureMultipleCycles) {
    SetupLinearGrid(10);

    state->tiles[0].moles[GAS_OXYGEN] = 500.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 2000.0f;

    for (int cycle = 0; cycle < 20; cycle++) {
        state->updateCounter++;
        for (int i = 0; i < 10; i++) {
            state->tiles[i].lastCycle = 0;
        }
        atmos_equalize_pressure_zone(state, 0, &config);
    }

    float avgMoles = 0.0f;
    for (int i = 0; i < 10; i++) {
        avgMoles += GetTotalMoles(&state->tiles[i]);
    }
    avgMoles /= 10.0f;

    for (int i = 0; i < 10; i++) {
        EXPECT_NEAR(GetTotalMoles(&state->tiles[i]), avgMoles, avgMoles * 0.3f);
    }
}

TEST_F(MonstermosTest, ExplosiveDepressurizeBasic) {
    SetupLinearGrid(5);
    
    TileAtmosData spaceTile = CreateSpaceTile(5, 0);
    int32_t spaceIdx = atmos_add_tile(state, &spaceTile);
    SetupAdjacency(4, spaceIdx, ATMOS_DIR_EAST);
    
    state->tiles[0].moles[GAS_OXYGEN] = 100.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 400.0f;
    
    AtmosResult result = atmos_explosive_depressurize(state, 0, &config);
    
    EXPECT_EQ(result.processingComplete, 1);
}

TEST_F(MonstermosTest, ExplosiveDepressurizeRemovesMoles) {
    SetupLinearGrid(3);
    
    TileAtmosData spaceTile = CreateSpaceTile(3, 0);
    int32_t spaceIdx = atmos_add_tile(state, &spaceTile);
    SetupAdjacency(2, spaceIdx, ATMOS_DIR_EAST);
    
    float totalBefore = 0.0f;
    for (int i = 0; i < 3; i++) {
        totalBefore += GetTotalMoles(&state->tiles[i]);
    }
    
    state->updateCounter++;
    atmos_explosive_depressurize(state, 0, &config);
    
    float totalAfter = 0.0f;
    for (int i = 0; i < 3; i++) {
        totalAfter += GetTotalMoles(&state->tiles[i]);
    }
    
    EXPECT_LT(totalAfter, totalBefore);
}

TEST_F(MonstermosTest, ExplosiveDepressurizeSetsPressureDifference) {
    SetupLinearGrid(3);
    
    TileAtmosData spaceTile = CreateSpaceTile(3, 0);
    int32_t spaceIdx = atmos_add_tile(state, &spaceTile);
    SetupAdjacency(2, spaceIdx, ATMOS_DIR_EAST);
    
    state->tiles[0].moles[GAS_OXYGEN] = 100.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 400.0f;
    
    state->updateCounter++;
    atmos_explosive_depressurize(state, 0, &config);
    
    bool foundPressureDiff = false;
    for (int i = 0; i < 3; i++) {
        if (state->tiles[i].pressureDifference > 0.0f) {
            foundPressureDiff = true;
            break;
        }
    }
    
    EXPECT_TRUE(foundPressureDiff);
}

TEST_F(MonstermosTest, ExplosiveDepressurizeDisabled) {
    config.spacingEnabled = 0;
    
    SetupLinearGrid(3);
    
    TileAtmosData spaceTile = CreateSpaceTile(3, 0);
    int32_t spaceIdx = atmos_add_tile(state, &spaceTile);
    SetupAdjacency(2, spaceIdx, ATMOS_DIR_EAST);
    
    float totalBefore = 0.0f;
    for (int i = 0; i < 3; i++) {
        totalBefore += GetTotalMoles(&state->tiles[i]);
    }
    
    state->updateCounter++;
    atmos_explosive_depressurize(state, 0, &config);
    
    float totalAfter = 0.0f;
    for (int i = 0; i < 3; i++) {
        totalAfter += GetTotalMoles(&state->tiles[i]);
    }
    
    EXPECT_NEAR(totalAfter, totalBefore, 0.01f);
}

TEST_F(MonstermosTest, LargeGridEqualization) {
    SetupSquareGrid(20, 20);
    
    state->tiles[0].moles[GAS_OXYGEN] = 1000.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 4000.0f;
    
    state->tiles[399].moles[GAS_OXYGEN] = 0.1f;
    state->tiles[399].moles[GAS_NITROGEN] = 0.1f;
    
    float totalBefore = 0.0f;
    for (int i = 0; i < 400; i++) {
        totalBefore += GetTotalMoles(&state->tiles[i]);
    }
    
    for (int cycle = 0; cycle < 50; cycle++) {
        state->updateCounter++;
        
        for (int i = 0; i < 400; i++) {
            state->tiles[i].lastCycle = 0;
        }
        
        atmos_equalize_pressure_zone(state, 0, &config);
    }
    
    float totalAfter = 0.0f;
    for (int i = 0; i < 400; i++) {
        totalAfter += GetTotalMoles(&state->tiles[i]);
    }
    
    EXPECT_NEAR(totalAfter, totalBefore, totalBefore * 0.01f);
}

TEST_F(MonstermosTest, TileLimitRespected) {
    int largeSize = config.constants.monstermosTileLimit + 50;
    
    for (int i = 0; i < largeSize; i++) {
        TileAtmosData tile = CreateStandardTile(i, 0);
        atmos_add_tile(state, &tile);
    }
    for (int i = 0; i < largeSize - 1; i++) {
        SetupAdjacency(i, i + 1, ATMOS_DIR_EAST);
    }
    
    state->tiles[0].moles[GAS_OXYGEN] = 1000.0f;
    
    state->updateCounter++;
    AtmosResult result = atmos_equalize_pressure_zone(state, 0, &config);
    
    EXPECT_EQ(result.processingComplete, 1);
}

TEST_F(MonstermosTest, DisconnectedTilesNotAffected) {
    SetupLinearGrid(3);
    
    TileAtmosData disconnected = CreateStandardTile(10, 0);
    int32_t disconnectedIdx = atmos_add_tile(state, &disconnected);
    
    float disconnectedMoles = GetTotalMoles(&state->tiles[disconnectedIdx]);
    
    state->tiles[0].moles[GAS_OXYGEN] = 500.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 2000.0f;
    
    for (int cycle = 0; cycle < 20; cycle++) {
        state->updateCounter++;
        atmos_equalize_pressure_zone(state, 0, &config);
    }
    
    EXPECT_NEAR(GetTotalMoles(&state->tiles[disconnectedIdx]), disconnectedMoles, 0.01f);
}

TEST_F(MonstermosTest, EqualizationConvergence) {
    SetupLinearGrid(10);
    
    state->tiles[5].moles[GAS_OXYGEN] = 500.0f;
    state->tiles[5].moles[GAS_NITROGEN] = 2000.0f;
    
    for (int i = 0; i < 10; i++) {
        atmos_add_active_tile(state, i);
    }
    
    for (int cycle = 0; cycle < 100; cycle++) {
        state->updateCounter++;
        atmos_process(state, &config);
    }
    
    float avgMoles = 0.0f;
    for (int i = 0; i < 10; i++) {
        avgMoles += GetTotalMoles(&state->tiles[i]);
    }
    avgMoles /= 10.0f;
    
    float maxDeviation = 0.0f;
    for (int i = 0; i < 10; i++) {
        float deviation = std::abs(GetTotalMoles(&state->tiles[i]) - avgMoles);
        if (deviation > maxDeviation) {
            maxDeviation = deviation;
        }
    }
    
    EXPECT_LT(maxDeviation / avgMoles, 0.1f);
}

TEST_F(MonstermosTest, MonstermosDisabledUsesLinda) {
    config.monstermosEnabled = 0;
    
    SetupLinearGrid(5);
    
    state->tiles[0].moles[GAS_OXYGEN] = 100.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 400.0f;
    
    for (int i = 0; i < 5; i++) {
        atmos_add_active_tile(state, i);
    }
    
    for (int cycle = 0; cycle < 50; cycle++) {
        state->updateCounter++;
        atmos_process_active_tiles(state, &config);
    }
    
    EXPECT_GT(GetTotalMoles(&state->tiles[2]), GetTotalMoles(&state->tiles[4]) * 0.5f);
}

TEST_F(MonstermosTest, SpacingEscapeRatio) {
    config.spacingEscapeRatio = 0.5f;
    
    SetupLinearGrid(3);
    
    TileAtmosData spaceTile = CreateSpaceTile(3, 0);
    int32_t spaceIdx = atmos_add_tile(state, &spaceTile);
    SetupAdjacency(2, spaceIdx, ATMOS_DIR_EAST);
    
    state->tiles[0].moles[GAS_OXYGEN] = 100.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 400.0f;
    
    float totalBefore = 0.0f;
    for (int i = 0; i < 3; i++) {
        totalBefore += GetTotalMoles(&state->tiles[i]);
    }
    
    state->updateCounter++;
    atmos_explosive_depressurize(state, 0, &config);
    
    float totalAfter = 0.0f;
    for (int i = 0; i < 3; i++) {
        totalAfter += GetTotalMoles(&state->tiles[i]);
    }
    
    float removed = totalBefore - totalAfter;
    EXPECT_GT(removed, 0.0f);
}

TEST_F(MonstermosTest, SpacingMaxWind) {
    config.spacingMaxWind = 100.0f;
    
    SetupLinearGrid(3);
    
    TileAtmosData spaceTile = CreateSpaceTile(3, 0);
    int32_t spaceIdx = atmos_add_tile(state, &spaceTile);
    SetupAdjacency(2, spaceIdx, ATMOS_DIR_EAST);
    
    state->tiles[0].moles[GAS_OXYGEN] = 1000.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 4000.0f;
    
    state->updateCounter++;
    atmos_explosive_depressurize(state, 0, &config);
    
    for (int i = 0; i < 3; i++) {
        EXPECT_LE(state->tiles[i].pressureDifference, config.spacingMaxWind * 2.0f);
    }
}