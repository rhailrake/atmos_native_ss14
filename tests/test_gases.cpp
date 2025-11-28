#include "test_common.h"

class GasesTest : public AtmosTestFixture {};

TEST_F(GasesTest, HeatCapacityStandardAir) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    float hc = atmos_get_heat_capacity(&tile, config.gasSpecificHeats);
    
    float expectedHc = tile.moles[GAS_OXYGEN] * config.gasSpecificHeats[GAS_OXYGEN] +
                       tile.moles[GAS_NITROGEN] * config.gasSpecificHeats[GAS_NITROGEN];
    
    EXPECT_NEAR(hc, expectedHc, 0.01f);
    EXPECT_GT(hc, config.constants.minimumHeatCapacity);
}

TEST_F(GasesTest, HeatCapacityEmptyTile) {
    TileAtmosData tile = {};
    tile.flags = 0;
    float hc = atmos_get_heat_capacity(&tile, config.gasSpecificHeats);
    EXPECT_GE(hc, config.constants.minimumHeatCapacity);
}

TEST_F(GasesTest, HeatCapacitySpaceTile) {
    TileAtmosData tile = CreateSpaceTile(0, 0);
    float hc = atmos_get_heat_capacity(&tile, config.gasSpecificHeats);
    EXPECT_NEAR(hc, config.constants.spaceHeatCapacity, 1.0f);
}

TEST_F(GasesTest, HeatCapacityArchived) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    atmos_archive_tile(&tile);
    
    float hc = atmos_get_heat_capacity(&tile, config.gasSpecificHeats);
    float hcArchived = atmos_get_heat_capacity_archived(&tile, config.gasSpecificHeats);
    
    EXPECT_NEAR(hc, hcArchived, 0.01f);
}

TEST_F(GasesTest, HeatCapacityArchivedDiffers) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    atmos_archive_tile(&tile);
    
    tile.moles[GAS_OXYGEN] *= 2.0f;
    tile.moles[GAS_NITROGEN] *= 2.0f;
    
    float hc = atmos_get_heat_capacity(&tile, config.gasSpecificHeats);
    float hcArchived = atmos_get_heat_capacity_archived(&tile, config.gasSpecificHeats);
    
    EXPECT_GT(hc, hcArchived);
}

TEST_F(GasesTest, ThermalEnergy) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    float energy = atmos_get_thermal_energy(&tile, config.gasSpecificHeats);
    float hc = atmos_get_heat_capacity(&tile, config.gasSpecificHeats);
    
    EXPECT_NEAR(energy, tile.temperature * hc, 0.01f);
}

TEST_F(GasesTest, ThermalEnergyHighTemperature) {
    TileAtmosData tile = CreateHotTile(0, 0, 1000.0f);
    float energy = atmos_get_thermal_energy(&tile, config.gasSpecificHeats);
    float hc = atmos_get_heat_capacity(&tile, config.gasSpecificHeats);
    
    EXPECT_NEAR(energy, 1000.0f * hc, 0.01f);
}

TEST_F(GasesTest, MergeEqualTemperature) {
    TileAtmosData receiver = CreateStandardTile(0, 0);
    TileAtmosData giver = CreateStandardTile(1, 0);
    
    float initialOxygen = receiver.moles[GAS_OXYGEN];
    float initialNitrogen = receiver.moles[GAS_NITROGEN];
    float initialTemp = receiver.temperature;
    
    atmos_merge(&receiver, &giver, config.gasSpecificHeats);
    
    EXPECT_NEAR(receiver.moles[GAS_OXYGEN], initialOxygen + giver.moles[GAS_OXYGEN], 0.01f);
    EXPECT_NEAR(receiver.moles[GAS_NITROGEN], initialNitrogen + giver.moles[GAS_NITROGEN], 0.01f);
    EXPECT_NEAR(receiver.temperature, initialTemp, 1.0f);
}

TEST_F(GasesTest, MergeDifferentTemperature) {
    TileAtmosData receiver = CreateStandardTile(0, 0);
    receiver.temperature = 300.0f;
    
    TileAtmosData giver = CreateStandardTile(1, 0);
    giver.temperature = 500.0f;
    
    float receiverEnergy = atmos_get_thermal_energy(&receiver, config.gasSpecificHeats);
    float giverEnergy = atmos_get_thermal_energy(&giver, config.gasSpecificHeats);
    
    atmos_merge(&receiver, &giver, config.gasSpecificHeats);
    
    float newHc = atmos_get_heat_capacity(&receiver, config.gasSpecificHeats);
    float expectedTemp = (receiverEnergy + giverEnergy) / newHc;
    
    EXPECT_NEAR(receiver.temperature, expectedTemp, 1.0f);
}

TEST_F(GasesTest, MergeImmutableReceiver) {
    TileAtmosData receiver = CreateStandardTile(0, 0);
    receiver.flags = TILE_FLAG_IMMUTABLE;
    float initialOxygen = receiver.moles[GAS_OXYGEN];
    
    TileAtmosData giver = CreateStandardTile(1, 0);
    
    atmos_merge(&receiver, &giver, config.gasSpecificHeats);
    
    EXPECT_EQ(receiver.moles[GAS_OXYGEN], initialOxygen);
}

TEST_F(GasesTest, RemoveGasPartial) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    TileAtmosData removed = {};
    
    float initialMoles = GetTotalMoles(&tile);
    float removeAmount = 50.0f;
    
    atmos_remove_gas(&tile, removeAmount, &removed);
    
    EXPECT_NEAR(GetTotalMoles(&tile) + GetTotalMoles(&removed), initialMoles, 0.01f);
    EXPECT_NEAR(GetTotalMoles(&removed), removeAmount, 0.1f);
}

TEST_F(GasesTest, RemoveGasAll) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    TileAtmosData removed = {};
    
    float initialMoles = GetTotalMoles(&tile);
    
    atmos_remove_gas(&tile, initialMoles * 2.0f, &removed);
    
    EXPECT_NEAR(GetTotalMoles(&removed), initialMoles, 0.01f);
}

TEST_F(GasesTest, RemoveRatio) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    TileAtmosData removed = {};
    
    float initialMoles = GetTotalMoles(&tile);
    float ratio = 0.5f;
    
    atmos_remove_ratio(&tile, ratio, &removed);
    
    EXPECT_NEAR(GetTotalMoles(&tile), initialMoles * 0.5f, 0.1f);
    EXPECT_NEAR(GetTotalMoles(&removed), initialMoles * 0.5f, 0.1f);
}

TEST_F(GasesTest, RemoveRatioZero) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    TileAtmosData removed = {};
    
    float initialMoles = GetTotalMoles(&tile);
    
    atmos_remove_ratio(&tile, 0.0f, &removed);
    
    EXPECT_NEAR(GetTotalMoles(&tile), initialMoles, 0.01f);
    EXPECT_NEAR(GetTotalMoles(&removed), 0.0f, 0.01f);
}

TEST_F(GasesTest, RemoveRatioOne) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    TileAtmosData removed = {};
    
    float initialMoles = GetTotalMoles(&tile);
    
    atmos_remove_ratio(&tile, 1.0f, &removed);
    
    EXPECT_NEAR(GetTotalMoles(&removed), initialMoles, 0.01f);
}

TEST_F(GasesTest, RemoveRatioOverOne) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    TileAtmosData removed = {};
    
    float initialMoles = GetTotalMoles(&tile);
    
    atmos_remove_ratio(&tile, 1.5f, &removed);
    
    EXPECT_NEAR(GetTotalMoles(&removed), initialMoles, 0.01f);
}

TEST_F(GasesTest, RemoveRatioPreservesRatios) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    TileAtmosData removed = {};
    
    float oxygenRatio = tile.moles[GAS_OXYGEN] / GetTotalMoles(&tile);
    
    atmos_remove_ratio(&tile, 0.5f, &removed);
    
    float newOxygenRatio = tile.moles[GAS_OXYGEN] / GetTotalMoles(&tile);
    EXPECT_NEAR(oxygenRatio, newOxygenRatio, 0.01f);
}

TEST_F(GasesTest, RemoveRatioTemperaturePreserved) {
    TileAtmosData tile = CreateHotTile(0, 0, 500.0f);
    TileAtmosData removed = {};
    
    atmos_remove_ratio(&tile, 0.5f, &removed);
    
    EXPECT_NEAR(tile.temperature, 500.0f, 0.01f);
    EXPECT_NEAR(removed.temperature, 500.0f, 0.01f);
}

TEST_F(GasesTest, ArchiveTile) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    tile.temperature = 400.0f;
    
    atmos_archive_tile(&tile);
    
    EXPECT_EQ(tile.temperatureArchived, 400.0f);
    for (int i = 0; i < ATMOS_GAS_COUNT; i++) {
        EXPECT_EQ(tile.moles[i], tile.molesArchived[i]);
    }
}

TEST_F(GasesTest, ArchiveAllTiles) {
    SetupLinearGrid(10);
    
    for (int i = 0; i < state->tileCount; i++) {
        state->tiles[i].temperature = 300.0f + i * 10.0f;
    }
    
    atmos_archive_all(state);
    
    for (int i = 0; i < state->tileCount; i++) {
        EXPECT_EQ(state->tiles[i].temperature, state->tiles[i].temperatureArchived);
    }
}

TEST_F(GasesTest, TotalMolesCalculation) {
    TileAtmosData tile = {};
    tile.moles[GAS_OXYGEN] = 10.0f;
    tile.moles[GAS_NITROGEN] = 20.0f;
    tile.moles[GAS_PLASMA] = 5.0f;
    
    EXPECT_NEAR(tile_total_moles(&tile), 35.0f, 0.01f);
}

TEST_F(GasesTest, PressureCalculation) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    float totalMoles = GetTotalMoles(&tile);
    
    float expectedPressure = totalMoles * config.constants.R * tile.temperature / config.constants.cellVolume;
    float actualPressure = GetPressure(&tile);
    
    EXPECT_NEAR(actualPressure, expectedPressure, 0.1f);
}

TEST_F(GasesTest, PressureAtOneAtm) {
    float molesAtOneAtm = (config.constants.oneAtmosphere * config.constants.cellVolume) / 
                          (config.constants.T20C * config.constants.R);
    
    TileAtmosData tile = {};
    tile.temperature = config.constants.T20C;
    tile.moles[GAS_NITROGEN] = molesAtOneAtm;
    
    float pressure = GetPressure(&tile);
    EXPECT_NEAR(pressure, config.constants.oneAtmosphere, 0.1f);
}

TEST_F(GasesTest, GasSpecificHeatsInitialized) {
    EXPECT_GT(config.gasSpecificHeats[GAS_OXYGEN], 0.0f);
    EXPECT_GT(config.gasSpecificHeats[GAS_NITROGEN], 0.0f);
    EXPECT_GT(config.gasSpecificHeats[GAS_CO2], 0.0f);
    EXPECT_GT(config.gasSpecificHeats[GAS_PLASMA], 0.0f);
    EXPECT_GT(config.gasSpecificHeats[GAS_TRITIUM], 0.0f);
}

TEST_F(GasesTest, PlasmaHigherHeatCapacity) {
    EXPECT_GT(config.gasSpecificHeats[GAS_PLASMA], config.gasSpecificHeats[GAS_OXYGEN]);
}

TEST_F(GasesTest, FrezonHighestHeatCapacity) {
    for (int i = 0; i < ATMOS_GAS_COUNT; i++) {
        if (i != GAS_FREZON) {
            EXPECT_GE(config.gasSpecificHeats[GAS_FREZON], config.gasSpecificHeats[i]);
        }
    }
}

TEST_F(GasesTest, CompareExchangeNoExchange) {
    TileAtmosData a = CreateStandardTile(0, 0);
    TileAtmosData b = CreateStandardTile(1, 0);
    
    int result = compare_exchange(&a, &b, &config);
    EXPECT_EQ(result, -2);
}

TEST_F(GasesTest, CompareExchangeMolesDelta) {
    TileAtmosData a = CreateHighPressureTile(0, 0);
    TileAtmosData b = CreateLowPressureTile(1, 0);
    
    int result = compare_exchange(&a, &b, &config);
    EXPECT_GE(result, -1);
}

TEST_F(GasesTest, CompareExchangeTemperatureDelta) {
    TileAtmosData a = CreateHotTile(0, 0, 500.0f);
    TileAtmosData b = CreateStandardTile(1, 0);
    
    int result = compare_exchange(&a, &b, &config);
    EXPECT_EQ(result, -1);
}