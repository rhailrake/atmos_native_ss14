#include "test_common.h"

class ReactionTest : public AtmosTestFixture {};

TEST_F(ReactionTest, PlasmaFireBasic) {
    TileAtmosData tile = {};
    tile.moles[GAS_PLASMA] = 10.0f;
    tile.moles[GAS_OXYGEN] = 30.0f;
    tile.temperature = config.constants.plasmaUpperTemperature + 100.0f;

    float initialPlasma = tile.moles[GAS_PLASMA];
    float initialOxygen = tile.moles[GAS_OXYGEN];

    int result = plasma_fire_reaction(&tile, &config);

    EXPECT_EQ(result, 1);
    EXPECT_LT(tile.moles[GAS_PLASMA], initialPlasma);
    EXPECT_LT(tile.moles[GAS_OXYGEN], initialOxygen);
    EXPECT_GT(tile.moles[GAS_CO2], 0.0f);
    EXPECT_GT(tile.moles[GAS_WATER_VAPOR], 0.0f);
}

TEST_F(ReactionTest, PlasmaFireNoPlasma) {
    TileAtmosData tile = {};
    tile.moles[GAS_OXYGEN] = 30.0f;
    tile.temperature = config.constants.plasmaUpperTemperature + 100.0f;

    int result = plasma_fire_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, PlasmaFireNoOxygen) {
    TileAtmosData tile = {};
    tile.moles[GAS_PLASMA] = 10.0f;
    tile.temperature = config.constants.plasmaUpperTemperature + 100.0f;

    int result = plasma_fire_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, PlasmaFireLowTemperature) {
    TileAtmosData tile = {};
    tile.moles[GAS_PLASMA] = 10.0f;
    tile.moles[GAS_OXYGEN] = 30.0f;
    tile.temperature = config.constants.plasmaMinimumBurnTemperature - 10.0f;

    int result = plasma_fire_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, PlasmaFireTemperatureIncrease) {
    TileAtmosData tile = {};
    tile.moles[GAS_PLASMA] = 10.0f;
    tile.moles[GAS_OXYGEN] = 30.0f;
    tile.temperature = config.constants.plasmaUpperTemperature + 100.0f;

    float initialTemp = tile.temperature;

    plasma_fire_reaction(&tile, &config);

    EXPECT_GT(tile.temperature, initialTemp);
}

TEST_F(ReactionTest, TritiumFireBasic) {
    TileAtmosData tile = {};
    tile.moles[GAS_TRITIUM] = 5.0f;
    tile.moles[GAS_OXYGEN] = 600.0f;
    tile.temperature = config.constants.plasmaMinimumBurnTemperature + 100.0f;

    float initialTritium = tile.moles[GAS_TRITIUM];
    float initialOxygen = tile.moles[GAS_OXYGEN];

    int result = tritium_fire_reaction(&tile, &config);

    EXPECT_EQ(result, 1);
    EXPECT_LT(tile.moles[GAS_TRITIUM], initialTritium);
    EXPECT_LT(tile.moles[GAS_OXYGEN], initialOxygen);
    EXPECT_GT(tile.moles[GAS_WATER_VAPOR], 0.0f);
}

TEST_F(ReactionTest, TritiumFireNoTritium) {
    TileAtmosData tile = {};
    tile.moles[GAS_OXYGEN] = 600.0f;
    tile.temperature = config.constants.plasmaMinimumBurnTemperature + 100.0f;

    int result = tritium_fire_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, TritiumFireNoOxygen) {
    TileAtmosData tile = {};
    tile.moles[GAS_TRITIUM] = 5.0f;
    tile.temperature = config.constants.plasmaMinimumBurnTemperature + 100.0f;

    int result = tritium_fire_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, TritiumFireLowTemperature) {
    TileAtmosData tile = {};
    tile.moles[GAS_TRITIUM] = 5.0f;
    tile.moles[GAS_OXYGEN] = 600.0f;
    tile.temperature = config.constants.plasmaMinimumBurnTemperature - 10.0f;

    int result = tritium_fire_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, TritiumFireTemperatureIncrease) {
    TileAtmosData tile = {};
    tile.moles[GAS_TRITIUM] = 5.0f;
    tile.moles[GAS_OXYGEN] = 600.0f;
    tile.temperature = config.constants.plasmaMinimumBurnTemperature + 100.0f;

    float initialTemp = tile.temperature;

    tritium_fire_reaction(&tile, &config);

    EXPECT_GT(tile.temperature, initialTemp);
}

TEST_F(ReactionTest, WaterVaporCondensationBasic) {
    TileAtmosData tile = {};
    tile.moles[GAS_WATER_VAPOR] = 10.0f;
    tile.moles[GAS_NITROGEN] = 79.0f;
    tile.temperature = config.constants.T0C + 50.0f;

    float initialWater = tile.moles[GAS_WATER_VAPOR];

    int result = water_vapor_reaction(&tile, &config);

    EXPECT_EQ(result, 1);
    EXPECT_LT(tile.moles[GAS_WATER_VAPOR], initialWater);
}

TEST_F(ReactionTest, WaterVaporNoCondensationLowAmount) {
    TileAtmosData tile = {};
    tile.moles[GAS_WATER_VAPOR] = 0.1f;
    tile.moles[GAS_NITROGEN] = 79.0f;
    tile.temperature = config.constants.T0C + 50.0f;

    int result = water_vapor_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, WaterVaporNoCondensationHighTemp) {
    TileAtmosData tile = {};
    tile.moles[GAS_WATER_VAPOR] = 10.0f;
    tile.moles[GAS_NITROGEN] = 79.0f;
    tile.temperature = config.constants.T0C + 150.0f;

    float initialWater = tile.moles[GAS_WATER_VAPOR];

    int result = water_vapor_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
    EXPECT_FLOAT_EQ(tile.moles[GAS_WATER_VAPOR], initialWater);
}

TEST_F(ReactionTest, WaterVaporPartialCondensation) {
    TileAtmosData tile = {};
    tile.moles[GAS_WATER_VAPOR] = 100.0f;
    tile.moles[GAS_NITROGEN] = 79.0f;
    tile.temperature = config.constants.T0C;

    float initialWater = tile.moles[GAS_WATER_VAPOR];

    water_vapor_reaction(&tile, &config);

    EXPECT_NEAR(tile.moles[GAS_WATER_VAPOR], initialWater * 0.95f, 0.1f);
}

TEST_F(ReactionTest, N2ODecompositionBasic) {
    TileAtmosData tile = {};
    tile.moles[GAS_N2O] = 20.0f;
    tile.moles[GAS_NITROGEN] = 10.0f;
    tile.temperature = config.constants.T0C + 300.0f;

    float initialN2O = tile.moles[GAS_N2O];
    float initialN2 = tile.moles[GAS_NITROGEN];
    float initialO2 = tile.moles[GAS_OXYGEN];

    int result = n2o_decomposition_reaction(&tile, &config);

    EXPECT_EQ(result, 1);
    EXPECT_LT(tile.moles[GAS_N2O], initialN2O);
    EXPECT_GT(tile.moles[GAS_NITROGEN], initialN2);
    EXPECT_GT(tile.moles[GAS_OXYGEN], initialO2);
}

TEST_F(ReactionTest, N2ODecompositionNoN2O) {
    TileAtmosData tile = {};
    tile.moles[GAS_NITROGEN] = 10.0f;
    tile.temperature = config.constants.T0C + 300.0f;

    int result = n2o_decomposition_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, N2ODecompositionLowTemperature) {
    TileAtmosData tile = {};
    tile.moles[GAS_N2O] = 20.0f;
    tile.temperature = config.constants.T0C + 200.0f;

    int result = n2o_decomposition_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, N2ODecompositionTemperatureIncrease) {
    TileAtmosData tile = {};
    tile.moles[GAS_N2O] = 20.0f;
    tile.moles[GAS_NITROGEN] = 10.0f;
    tile.temperature = config.constants.T0C + 300.0f;

    float initialTemp = tile.temperature;

    n2o_decomposition_reaction(&tile, &config);

    EXPECT_GT(tile.temperature, initialTemp);
}

TEST_F(ReactionTest, N2ODecompositionProductRatio) {
    TileAtmosData tile = {};
    tile.moles[GAS_N2O] = 20.0f;
    tile.temperature = config.constants.T0C + 300.0f;

    n2o_decomposition_reaction(&tile, &config);

    float n2Produced = tile.moles[GAS_NITROGEN];
    float o2Produced = tile.moles[GAS_OXYGEN];

    EXPECT_NEAR(n2Produced / o2Produced, 2.0f, 0.1f);
}

TEST_F(ReactionTest, AmmoniaOxygenBasic) {
    TileAtmosData tile = {};
    tile.moles[GAS_AMMONIA] = 20.0f;
    tile.moles[GAS_OXYGEN] = 30.0f;
    tile.temperature = config.constants.T0C + 150.0f;

    float initialAmmonia = tile.moles[GAS_AMMONIA];
    float initialOxygen = tile.moles[GAS_OXYGEN];

    int result = ammonia_oxygen_reaction(&tile, &config);

    EXPECT_EQ(result, 1);
    EXPECT_LT(tile.moles[GAS_AMMONIA], initialAmmonia);
    EXPECT_LT(tile.moles[GAS_OXYGEN], initialOxygen);
    EXPECT_GT(tile.moles[GAS_NITROGEN], 0.0f);
    EXPECT_GT(tile.moles[GAS_WATER_VAPOR], 0.0f);
}

TEST_F(ReactionTest, AmmoniaOxygenNoAmmonia) {
    TileAtmosData tile = {};
    tile.moles[GAS_OXYGEN] = 30.0f;
    tile.temperature = config.constants.T0C + 150.0f;

    int result = ammonia_oxygen_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, AmmoniaOxygenNoOxygen) {
    TileAtmosData tile = {};
    tile.moles[GAS_AMMONIA] = 20.0f;
    tile.temperature = config.constants.T0C + 150.0f;

    int result = ammonia_oxygen_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, AmmoniaOxygenLowTemperature) {
    TileAtmosData tile = {};
    tile.moles[GAS_AMMONIA] = 20.0f;
    tile.moles[GAS_OXYGEN] = 30.0f;
    tile.temperature = config.constants.T0C + 50.0f;

    int result = ammonia_oxygen_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, AmmoniaOxygenProductRatio) {
    TileAtmosData tile = {};
    tile.moles[GAS_AMMONIA] = 100.0f;
    tile.moles[GAS_OXYGEN] = 100.0f;
    tile.temperature = config.constants.T0C + 150.0f;

    ammonia_oxygen_reaction(&tile, &config);

    float n2Produced = tile.moles[GAS_NITROGEN];
    float waterProduced = tile.moles[GAS_WATER_VAPOR];

    EXPECT_NEAR(waterProduced / n2Produced, 3.0f, 0.1f);
}

TEST_F(ReactionTest, FrezonProductionBasic) {
    TileAtmosData tile = {};
    tile.moles[GAS_TRITIUM] = 10.0f;
    tile.moles[GAS_OXYGEN] = 600.0f;
    tile.moles[GAS_NITROGEN] = 10.0f;
    tile.temperature = 73.15f;

    float initialTritium = tile.moles[GAS_TRITIUM];
    float initialOxygen = tile.moles[GAS_OXYGEN];
    float initialNitrogen = tile.moles[GAS_NITROGEN];

    int result = frezon_production_reaction(&tile, &config);

    EXPECT_EQ(result, 1);
    EXPECT_LT(tile.moles[GAS_TRITIUM], initialTritium);
    EXPECT_LT(tile.moles[GAS_OXYGEN], initialOxygen);
    EXPECT_LT(tile.moles[GAS_NITROGEN], initialNitrogen);
    EXPECT_GT(tile.moles[GAS_FREZON], 0.0f);
}

TEST_F(ReactionTest, FrezonProductionNoTritium) {
    TileAtmosData tile = {};
    tile.moles[GAS_OXYGEN] = 600.0f;
    tile.moles[GAS_NITROGEN] = 10.0f;
    tile.temperature = 73.15f;

    int result = frezon_production_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, FrezonProductionNoOxygen) {
    TileAtmosData tile = {};
    tile.moles[GAS_TRITIUM] = 10.0f;
    tile.moles[GAS_NITROGEN] = 10.0f;
    tile.temperature = 73.15f;

    int result = frezon_production_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, FrezonProductionNoNitrogen) {
    TileAtmosData tile = {};
    tile.moles[GAS_TRITIUM] = 10.0f;
    tile.moles[GAS_OXYGEN] = 600.0f;
    tile.temperature = 73.15f;

    int result = frezon_production_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, FrezonProductionHighTemperature) {
    TileAtmosData tile = {};
    tile.moles[GAS_TRITIUM] = 10.0f;
    tile.moles[GAS_OXYGEN] = 600.0f;
    tile.moles[GAS_NITROGEN] = 10.0f;
    tile.temperature = config.constants.frezonCoolMidTemperature + 10.0f;

    int result = frezon_production_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, FrezonProductionEfficiencyAtLowTemp) {
    TileAtmosData tile = {};
    tile.moles[GAS_TRITIUM] = 10.0f;
    tile.moles[GAS_OXYGEN] = 600.0f;
    tile.moles[GAS_NITROGEN] = 10.0f;
    tile.temperature = 50.0f;

    frezon_production_reaction(&tile, &config);

    float frezonProduced = tile.moles[GAS_FREZON];

    TileAtmosData tile2 = {};
    tile2.moles[GAS_TRITIUM] = 10.0f;
    tile2.moles[GAS_OXYGEN] = 600.0f;
    tile2.moles[GAS_NITROGEN] = 10.0f;
    tile2.temperature = 200.0f;

    frezon_production_reaction(&tile2, &config);

    EXPECT_GT(frezonProduced, tile2.moles[GAS_FREZON]);
}

TEST_F(ReactionTest, FrezonCoolantBasic) {
    TileAtmosData tile = {};
    tile.moles[GAS_FREZON] = 10.0f;
    tile.moles[GAS_NITROGEN] = 100.0f;
    tile.temperature = 300.0f;

    float initialTemp = tile.temperature;
    float initialNitrogen = tile.moles[GAS_NITROGEN];

    int result = frezon_coolant_reaction(&tile, &config);

    EXPECT_EQ(result, 1);
    EXPECT_LT(tile.temperature, initialTemp);
    EXPECT_LT(tile.moles[GAS_NITROGEN], initialNitrogen);
}

TEST_F(ReactionTest, FrezonCoolantNoFrezon) {
    TileAtmosData tile = {};
    tile.moles[GAS_NITROGEN] = 100.0f;
    tile.temperature = 300.0f;

    int result = frezon_coolant_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, FrezonCoolantNoNitrogen) {
    TileAtmosData tile = {};
    tile.moles[GAS_FREZON] = 10.0f;
    tile.temperature = 300.0f;

    int result = frezon_coolant_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, FrezonCoolantLowTemperature) {
    TileAtmosData tile = {};
    tile.moles[GAS_FREZON] = 10.0f;
    tile.moles[GAS_NITROGEN] = 100.0f;
    tile.temperature = 10.0f;

    int result = frezon_coolant_reaction(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, FrezonCoolantEffectivenessScalesWithTemp) {
    TileAtmosData tile1 = {};
    tile1.moles[GAS_FREZON] = 10.0f;
    tile1.moles[GAS_NITROGEN] = 100.0f;
    tile1.temperature = 200.0f;
    float temp1Before = tile1.temperature;

    frezon_coolant_reaction(&tile1, &config);
    float cooling1 = temp1Before - tile1.temperature;

    TileAtmosData tile2 = {};
    tile2.moles[GAS_FREZON] = 10.0f;
    tile2.moles[GAS_NITROGEN] = 100.0f;
    tile2.temperature = 500.0f;
    float temp2Before = tile2.temperature;

    frezon_coolant_reaction(&tile2, &config);
    float cooling2 = temp2Before - tile2.temperature;

    EXPECT_GT(cooling2, cooling1);
}

TEST_F(ReactionTest, ReactImplCallsAllReactions) {
    TileAtmosData tile = {};
    tile.moles[GAS_PLASMA] = 10.0f;
    tile.moles[GAS_OXYGEN] = 30.0f;
    tile.temperature = config.constants.plasmaUpperTemperature + 100.0f;

    int result = react_impl(&tile, &config);

    EXPECT_EQ(result, 1);
}

TEST_F(ReactionTest, ReactImplNoReactionLowEnergy) {
    TileAtmosData tile = {};
    tile.moles[GAS_PLASMA] = 0.001f;
    tile.moles[GAS_OXYGEN] = 0.001f;
    tile.temperature = 10.0f;

    int result = react_impl(&tile, &config);

    EXPECT_EQ(result, 0);
}

TEST_F(ReactionTest, ReactImplImmutableTile) {
    TileAtmosData tile = {};
    tile.moles[GAS_PLASMA] = 10.0f;
    tile.moles[GAS_OXYGEN] = 30.0f;
    tile.temperature = config.constants.plasmaUpperTemperature + 100.0f;
    tile.flags = TILE_FLAG_IMMUTABLE;

    float initialPlasma = tile.moles[GAS_PLASMA];

    int result = react_impl(&tile, &config);

    EXPECT_EQ(result, 0);
    EXPECT_FLOAT_EQ(tile.moles[GAS_PLASMA], initialPlasma);
}

TEST_F(ReactionTest, PlasmaFireImmutableTile) {
    TileAtmosData tile = {};
    tile.moles[GAS_PLASMA] = 10.0f;
    tile.moles[GAS_OXYGEN] = 30.0f;
    tile.temperature = config.constants.plasmaUpperTemperature + 100.0f;
    tile.flags = TILE_FLAG_IMMUTABLE;

    float initialPlasma = tile.moles[GAS_PLASMA];

    int result = plasma_fire_reaction(&tile, &config);

    EXPECT_EQ(result, 1);
    EXPECT_FLOAT_EQ(tile.moles[GAS_PLASMA], initialPlasma);
}

TEST_F(ReactionTest, ReactionChainInProcessCell) {
    TileAtmosData tile = {};
    tile.moles[GAS_PLASMA] = 50.0f;
    tile.moles[GAS_OXYGEN] = 100.0f;
    tile.temperature = config.constants.plasmaUpperTemperature + 100.0f;
    tile.thermalConductivity = 0.5f;
    tile.heatCapacity = 10000.0f;
    for (int i = 0; i < ATMOS_DIRECTIONS; i++)
        tile.adjacentIndices[i] = -1;
    tile.excitedGroupId = -1;

    int32_t idx = atmos_add_tile(state, &tile);
    atmos_add_active_tile(state, idx);

    float initialPlasma = state->tiles[idx].moles[GAS_PLASMA];

    for (int cycle = 0; cycle < 10; cycle++) {
        atmos_process(state, &config);
    }

    EXPECT_LT(state->tiles[idx].moles[GAS_PLASMA], initialPlasma);
    EXPECT_GT(state->tiles[idx].moles[GAS_CO2], 0.0f);
}

TEST_F(ReactionTest, MultipleReactionsSameTime) {
    TileAtmosData tile = {};
    tile.moles[GAS_PLASMA] = 10.0f;
    tile.moles[GAS_TRITIUM] = 5.0f;
    tile.moles[GAS_OXYGEN] = 700.0f;
    tile.temperature = config.constants.plasmaUpperTemperature + 100.0f;

    float initialPlasma = tile.moles[GAS_PLASMA];
    float initialTritium = tile.moles[GAS_TRITIUM];

    react_impl(&tile, &config);

    EXPECT_LT(tile.moles[GAS_PLASMA], initialPlasma);
    EXPECT_LT(tile.moles[GAS_TRITIUM], initialTritium);
}

TEST_F(ReactionTest, PlasmaFireProductsCorrect) {
    TileAtmosData tile = {};
    tile.moles[GAS_PLASMA] = 100.0f;
    tile.moles[GAS_OXYGEN] = 1000.0f;
    tile.temperature = config.constants.plasmaUpperTemperature + 100.0f;

    plasma_fire_reaction(&tile, &config);

    float co2Produced = tile.moles[GAS_CO2];
    float waterProduced = tile.moles[GAS_WATER_VAPOR];

    EXPECT_NEAR(co2Produced / waterProduced, 3.0f, 0.1f);
}

TEST_F(ReactionTest, TritiumFireConsumesCorrectRatio) {
    TileAtmosData tile = {};
    tile.moles[GAS_TRITIUM] = 1.0f;
    tile.moles[GAS_OXYGEN] = 200.0f;
    tile.temperature = config.constants.plasmaMinimumBurnTemperature + 100.0f;

    float initialTritium = tile.moles[GAS_TRITIUM];
    float initialOxygen = tile.moles[GAS_OXYGEN];

    tritium_fire_reaction(&tile, &config);

    float tritiumBurned = initialTritium - tile.moles[GAS_TRITIUM];
    float oxygenBurned = initialOxygen - tile.moles[GAS_OXYGEN];

    if (tritiumBurned > 0) {
        EXPECT_NEAR(oxygenBurned / tritiumBurned, config.constants.tritiumBurnOxyFactor, 1.0f);
    }
}