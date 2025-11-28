#include "test_common.h"

class LindaTest : public AtmosTestFixture {};

TEST_F(LindaTest, ShareEqualTiles) {
    TileAtmosData receiver = CreateStandardTile(0, 0);
    TileAtmosData sharer = CreateStandardTile(1, 0);
    
    atmos_archive_tile(&receiver);
    atmos_archive_tile(&sharer);
    
    float initialReceiverMoles = GetTotalMoles(&receiver);
    float initialSharerMoles = GetTotalMoles(&sharer);
    
    atmos_share(&receiver, &sharer, 1, &config);
    
    float finalReceiverMoles = GetTotalMoles(&receiver);
    float finalSharerMoles = GetTotalMoles(&sharer);
    
    EXPECT_NEAR(finalReceiverMoles + finalSharerMoles, 
                initialReceiverMoles + initialSharerMoles, 0.01f);
}

TEST_F(LindaTest, ShareHighToLowPressure) {
    TileAtmosData receiver = CreateLowPressureTile(0, 0);
    TileAtmosData sharer = CreateHighPressureTile(1, 0);
    
    atmos_archive_tile(&receiver);
    atmos_archive_tile(&sharer);
    
    float initialReceiverMoles = GetTotalMoles(&receiver);
    float initialSharerMoles = GetTotalMoles(&sharer);
    
    atmos_share(&receiver, &sharer, 1, &config);
    
    float finalReceiverMoles = GetTotalMoles(&receiver);
    float finalSharerMoles = GetTotalMoles(&sharer);
    
    EXPECT_GT(finalReceiverMoles, initialReceiverMoles);
    EXPECT_LT(finalSharerMoles, initialSharerMoles);
    
    EXPECT_NEAR(finalReceiverMoles + finalSharerMoles,
                initialReceiverMoles + initialSharerMoles, 0.01f);
}

TEST_F(LindaTest, ShareWithMultipleAdjacent) {
    TileAtmosData receiver = CreateStandardTile(0, 0);
    TileAtmosData sharer = CreateHighPressureTile(1, 0);
    
    atmos_archive_tile(&receiver);
    atmos_archive_tile(&sharer);
    
    float initialTotal = GetTotalMoles(&receiver) + GetTotalMoles(&sharer);
    
    atmos_share(&receiver, &sharer, 4, &config);
    
    float finalTotal = GetTotalMoles(&receiver) + GetTotalMoles(&sharer);
    EXPECT_NEAR(finalTotal, initialTotal, 0.01f);
}

TEST_F(LindaTest, ShareTemperatureDifference) {
    TileAtmosData receiver = CreateStandardTile(0, 0);
    receiver.temperature = 300.0f;
    
    TileAtmosData sharer = CreateStandardTile(1, 0);
    sharer.temperature = 500.0f;
    
    atmos_archive_tile(&receiver);
    atmos_archive_tile(&sharer);
    
    atmos_share(&receiver, &sharer, 1, &config);
    
    EXPECT_GT(receiver.temperature, 300.0f);
    EXPECT_LT(sharer.temperature, 500.0f);
}

TEST_F(LindaTest, ShareImmutableReceiver) {
    TileAtmosData receiver = CreateStandardTile(0, 0);
    receiver.flags = TILE_FLAG_IMMUTABLE;
    float initialReceiverOxygen = receiver.moles[GAS_OXYGEN];
    
    TileAtmosData sharer = CreateHighPressureTile(1, 0);
    
    atmos_archive_tile(&receiver);
    atmos_archive_tile(&sharer);
    
    atmos_share(&receiver, &sharer, 1, &config);
    
    EXPECT_EQ(receiver.moles[GAS_OXYGEN], initialReceiverOxygen);
}

TEST_F(LindaTest, ShareImmutableSharer) {
    TileAtmosData receiver = CreateStandardTile(0, 0);
    TileAtmosData sharer = CreateHighPressureTile(1, 0);
    sharer.flags = TILE_FLAG_IMMUTABLE;
    float initialSharerOxygen = sharer.moles[GAS_OXYGEN];
    
    atmos_archive_tile(&receiver);
    atmos_archive_tile(&sharer);
    
    atmos_share(&receiver, &sharer, 1, &config);
    
    EXPECT_EQ(sharer.moles[GAS_OXYGEN], initialSharerOxygen);
}

TEST_F(LindaTest, TemperatureShareBasic) {
    TileAtmosData receiver = CreateStandardTile(0, 0);
    receiver.temperature = 300.0f;
    
    TileAtmosData sharer = CreateStandardTile(1, 0);
    sharer.temperature = 500.0f;
    
    atmos_archive_tile(&receiver);
    atmos_archive_tile(&sharer);
    
    float result = atmos_temperature_share(&receiver, &sharer, 
                                           config.constants.openHeatTransferCoefficient, &config);
    
    EXPECT_GT(result, 0.0f);
    EXPECT_GT(receiver.temperature, 300.0f);
    EXPECT_LT(sharer.temperature, 500.0f);
}

TEST_F(LindaTest, TemperatureShareEqualTemps) {
    TileAtmosData receiver = CreateStandardTile(0, 0);
    TileAtmosData sharer = CreateStandardTile(1, 0);
    
    float initialTemp = receiver.temperature;
    
    atmos_archive_tile(&receiver);
    atmos_archive_tile(&sharer);
    
    atmos_temperature_share(&receiver, &sharer, 
                           config.constants.openHeatTransferCoefficient, &config);
    
    EXPECT_NEAR(receiver.temperature, initialTemp, 0.1f);
    EXPECT_NEAR(sharer.temperature, initialTemp, 0.1f);
}

TEST_F(LindaTest, TemperatureShareConservesEnergy) {
    TileAtmosData receiver = CreateStandardTile(0, 0);
    receiver.temperature = 300.0f;
    
    TileAtmosData sharer = CreateStandardTile(1, 0);
    sharer.temperature = 500.0f;
    
    atmos_archive_tile(&receiver);
    atmos_archive_tile(&sharer);
    
    float initialEnergy = atmos_get_thermal_energy(&receiver, config.gasSpecificHeats) +
                          atmos_get_thermal_energy(&sharer, config.gasSpecificHeats);
    
    atmos_temperature_share(&receiver, &sharer, 
                           config.constants.openHeatTransferCoefficient, &config);
    
    float finalEnergy = atmos_get_thermal_energy(&receiver, config.gasSpecificHeats) +
                        atmos_get_thermal_energy(&sharer, config.gasSpecificHeats);
    
    EXPECT_NEAR(finalEnergy, initialEnergy, initialEnergy * 0.01f);
}

TEST_F(LindaTest, ProcessCellActivatesNeighbors) {
    SetupLinearGrid(3);
    
    state->tiles[0].moles[GAS_OXYGEN] = 100.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 400.0f;
    
    atmos_add_active_tile(state, 0);
    
    AtmosResult result = atmos_process_active_tiles(state, &config);
    
    EXPECT_GT(result.activeTilesCount, 1);
}

TEST_F(LindaTest, ProcessCellEqualizesOverTime) {
    SetupLinearGrid(5);
    
    state->tiles[0].moles[GAS_OXYGEN] = 100.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 400.0f;
    
    float initialMoles0 = GetTotalMoles(&state->tiles[0]);
    
    atmos_add_active_tile(state, 0);
    
    for (int i = 0; i < 50; i++) {
        state->updateCounter++;
        atmos_process_active_tiles(state, &config);
    }
    
    float finalMoles0 = GetTotalMoles(&state->tiles[0]);
    EXPECT_LT(finalMoles0, initialMoles0);
    
    float avgMoles = 0.0f;
    for (int i = 0; i < 5; i++) {
        avgMoles += GetTotalMoles(&state->tiles[i]);
    }
    avgMoles /= 5.0f;
    
    for (int i = 0; i < 5; i++) {
        EXPECT_NEAR(GetTotalMoles(&state->tiles[i]), avgMoles, avgMoles * 0.3f);
    }
}

TEST_F(LindaTest, ProcessCellImmutableSkipped) {
    SetupLinearGrid(3);
    
    state->tiles[1].flags = TILE_FLAG_IMMUTABLE;
    float initialMoles1 = GetTotalMoles(&state->tiles[1]);
    
    state->tiles[0].moles[GAS_OXYGEN] = 100.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 400.0f;
    
    atmos_add_active_tile(state, 0);
    
    for (int i = 0; i < 10; i++) {
        state->updateCounter++;
        atmos_process_active_tiles(state, &config);
    }
    
    EXPECT_EQ(GetTotalMoles(&state->tiles[1]), initialMoles1);
}

TEST_F(LindaTest, LastShareUpdated) {
    TileAtmosData receiver = CreateLowPressureTile(0, 0);
    TileAtmosData sharer = CreateHighPressureTile(1, 0);
    
    atmos_archive_tile(&receiver);
    atmos_archive_tile(&sharer);
    
    atmos_share(&receiver, &sharer, 1, &config);
    
    EXPECT_GT(receiver.lastShare, 0.0f);
}

TEST_F(LindaTest, TemperatureMinimumTCMB) {
    TileAtmosData receiver = CreateStandardTile(0, 0);
    receiver.temperature = config.constants.TCMB + 1.0f;
    
    TileAtmosData sharer = CreateStandardTile(1, 0);
    sharer.temperature = config.constants.TCMB;
    
    atmos_archive_tile(&receiver);
    atmos_archive_tile(&sharer);
    
    for (int i = 0; i < 100; i++) {
        atmos_temperature_share(&receiver, &sharer,
                               config.constants.openHeatTransferCoefficient, &config);
        atmos_archive_tile(&receiver);
        atmos_archive_tile(&sharer);
    }
    
    EXPECT_GE(receiver.temperature, config.constants.TCMB);
    EXPECT_GE(sharer.temperature, config.constants.TCMB);
}

TEST_F(LindaTest, SquareGridEqualization) {
    SetupSquareGrid(5, 5);
    
    state->tiles[12].moles[GAS_OXYGEN] = 200.0f;
    state->tiles[12].moles[GAS_NITROGEN] = 800.0f;
    
    atmos_add_active_tile(state, 12);
    
    for (int i = 0; i < 100; i++) {
        state->updateCounter++;
        atmos_process_active_tiles(state, &config);
    }
    
    float minMoles = GetTotalMoles(&state->tiles[0]);
    float maxMoles = minMoles;
    for (int i = 1; i < 25; i++) {
        float moles = GetTotalMoles(&state->tiles[i]);
        if (moles < minMoles) minMoles = moles;
        if (moles > maxMoles) maxMoles = moles;
    }
    
    EXPECT_LT(maxMoles - minMoles, maxMoles * 0.5f);
}

TEST_F(LindaTest, ConservationOfMass) {
    SetupSquareGrid(5, 5);
    
    float initialTotal = 0.0f;
    for (int i = 0; i < 25; i++) {
        initialTotal += GetTotalMoles(&state->tiles[i]);
    }
    
    state->tiles[0].moles[GAS_OXYGEN] = 500.0f;
    state->tiles[24].moles[GAS_NITROGEN] = 1000.0f;
    
    float newTotal = 0.0f;
    for (int i = 0; i < 25; i++) {
        newTotal += GetTotalMoles(&state->tiles[i]);
    }
    
    for (int i = 0; i < 25; i++) {
        atmos_add_active_tile(state, i);
    }
    
    for (int i = 0; i < 50; i++) {
        state->updateCounter++;
        atmos_process_active_tiles(state, &config);
    }
    
    float finalTotal = 0.0f;
    for (int i = 0; i < 25; i++) {
        finalTotal += GetTotalMoles(&state->tiles[i]);
    }
    
    EXPECT_NEAR(finalTotal, newTotal, newTotal * 0.001f);
}