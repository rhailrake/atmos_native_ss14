#pragma once

#include <stdint.h>
#include <string.h>

#define ATMOS_GAS_COUNT 9
#define ATMOS_GAS_ARRAY_SIZE 12
#define ATMOS_DIRECTIONS 4

#define ATMOS_DIR_NORTH 0
#define ATMOS_DIR_SOUTH 1
#define ATMOS_DIR_EAST  2
#define ATMOS_DIR_WEST  3

#define ATMOS_DIR_BIT_NORTH (1 << 0)
#define ATMOS_DIR_BIT_SOUTH (1 << 1)
#define ATMOS_DIR_BIT_EAST  (1 << 2)
#define ATMOS_DIR_BIT_WEST  (1 << 3)
#define ATMOS_DIR_BIT_ALL   0x0F

#define TILE_FLAG_SPACE       (1 << 0)
#define TILE_FLAG_EXCITED     (1 << 1)
#define TILE_FLAG_HOTSPOT     (1 << 2)
#define TILE_FLAG_IMMUTABLE   (1 << 3)
#define TILE_FLAG_MAP_ATMOS   (1 << 4)
#define TILE_FLAG_SUPERCONDUCT (1 << 5)
#define TILE_FLAG_PROCESSED   (1 << 6)

#define GAS_OXYGEN        0
#define GAS_NITROGEN      1
#define GAS_CO2           2
#define GAS_PLASMA        3
#define GAS_TRITIUM       4
#define GAS_WATER_VAPOR   5
#define GAS_AMMONIA       6
#define GAS_N2O           7
#define GAS_FREZON        8

struct TileAtmosData
{
    float moles[ATMOS_GAS_ARRAY_SIZE];
    float molesArchived[ATMOS_GAS_ARRAY_SIZE];
    float temperature;
    float temperatureArchived;
    float heatCapacity;
    float thermalConductivity;
    float pressureDifference;
    float lastShare;

    int32_t gridX;
    int32_t gridY;
    int32_t adjacentIndices[ATMOS_DIRECTIONS];

    uint8_t adjacentBits;
    uint8_t blockedBits;
    uint8_t flags;
    uint8_t hotspotState;

    float hotspotTemperature;
    float hotspotVolume;

    float moleDelta;
    float transferDirections[ATMOS_DIRECTIONS];
    float currentTransferAmount;
    int32_t currentTransferDirection;

    int32_t lastCycle;
    int32_t lastQueueCycle;
    int32_t lastSlowQueueCycle;
    int32_t excitedGroupId;

    uint8_t fastDone;
    uint8_t padding[3];
};

struct ExcitedGroupData
{
    int32_t id;
    int32_t breakdownCooldown;
    int32_t dismantleCooldown;
    int32_t tileCount;
    int32_t* tileIndices;
    uint8_t disposed;
    uint8_t padding[3];
};

struct AtmosConstants
{
    float R;
    float oneAtmosphere;
    float TCMB;
    float T0C;
    float T20C;
    float Tmax;
    float cellVolume;
    float gasMinMoles;
    float openHeatTransferCoefficient;
    float heatCapacityVacuum;
    float minimumAirRatioToSuspend;
    float minimumAirRatioToMove;
    float minimumAirToSuspend;
    float minimumTemperatureToMove;
    float minimumMolesDeltaToMove;
    float minimumTemperatureDeltaToSuspend;
    float minimumTemperatureDeltaToConsider;
    float minimumTemperatureStartSuperConduction;
    float minimumTemperatureForSuperconduction;
    float minimumHeatCapacity;
    float spaceHeatCapacity;
    float fireMinimumTemperatureToExist;
    float fireMinimumTemperatureToSpread;
    float fireSpreadRadiosityScale;
    float firePlasmaEnergyReleased;
    float fireHydrogenEnergyReleased;
    float fireGrowthRate;
    float plasmaMinimumBurnTemperature;
    float plasmaUpperTemperature;
    float plasmaOxygenFullburn;
    float plasmaBurnRateDelta;
    float oxygenBurnRateBase;
    float superSaturationThreshold;
    float tritiumBurnOxyFactor;
    float tritiumBurnTritFactor;
    float frezonCoolLowerTemperature;
    float frezonCoolMidTemperature;
    float frezonCoolMaximumEnergyModifier;
    float frezonNitrogenCoolRatio;
    float frezonCoolEnergyReleased;
    float frezonCoolRateModifier;
    float windowHeatTransferCoefficient;
    float mcellWithRatio;

    int32_t excitedGroupBreakdownCycles;
    int32_t excitedGroupsDismantleCycles;
    int32_t monstermosHardTileLimit;
    int32_t monstermosTileLimit;
};

struct AtmosConfig
{
    float gasSpecificHeats[ATMOS_GAS_ARRAY_SIZE];
    AtmosConstants constants;
    int32_t maxProcessTimeMicroseconds;
    float speedup;
    float heatScale;
    uint8_t monstermosEnabled;
    uint8_t excitedGroupsEnabled;
    uint8_t superconductionEnabled;
    uint8_t spacingEnabled;
    float spacingEscapeRatio;
    float spacingMinGas;
    float spacingMaxWind;
};

struct AtmosResult
{
    int32_t tilesProcessed;
    int32_t activeTilesCount;
    int32_t hotspotTilesCount;
    int32_t superconductTilesCount;
    int32_t excitedGroupsCount;
    int32_t reactionsTriggered;
    float maxPressureDelta;
    uint8_t processingComplete;
    uint8_t padding[3];
};

struct GridAtmosState
{
    TileAtmosData* tiles;
    int32_t tileCount;
    int32_t tileCapacity;

    int32_t* activeTiles;
    int32_t activeTileCount;
    int32_t activeTileCapacity;

    int32_t* hotspotTiles;
    int32_t hotspotTileCount;

    int32_t* superconductTiles;
    int32_t superconductTileCount;

    int32_t* highPressureTiles;
    int32_t highPressureTileCount;

    ExcitedGroupData* excitedGroups;
    int32_t excitedGroupCount;
    int32_t excitedGroupCapacity;

    int32_t updateCounter;
    int64_t equalizationQueueCycle;
};

inline void atmos_constants_init_default(AtmosConstants* c)
{
    c->R = 8.314462618f;
    c->oneAtmosphere = 101.325f;
    c->TCMB = 2.7f;
    c->T0C = 273.15f;
    c->T20C = 293.15f;
    c->Tmax = 262144.0f;
    c->cellVolume = 2500.0f;
    c->gasMinMoles = 0.00000005f;
    c->openHeatTransferCoefficient = 0.4f;
    c->heatCapacityVacuum = 7000.0f;
    c->minimumAirRatioToSuspend = 0.1f;
    c->minimumAirRatioToMove = 0.001f;

    float molesCellStandard = (c->oneAtmosphere * c->cellVolume) / (c->T20C * c->R);
    c->minimumAirToSuspend = molesCellStandard * c->minimumAirRatioToSuspend;
    c->minimumTemperatureToMove = c->T20C + 100.0f;
    c->minimumMolesDeltaToMove = molesCellStandard * c->minimumAirRatioToMove;
    c->minimumTemperatureDeltaToSuspend = 4.0f;
    c->minimumTemperatureDeltaToConsider = 0.01f;
    c->minimumTemperatureStartSuperConduction = c->T20C + 400.0f;
    c->minimumTemperatureForSuperconduction = c->T20C + 80.0f;
    c->minimumHeatCapacity = 0.0003f;
    c->spaceHeatCapacity = 7000.0f;
    c->fireMinimumTemperatureToExist = c->T0C + 100.0f;
    c->fireMinimumTemperatureToSpread = c->T0C + 150.0f;
    c->fireSpreadRadiosityScale = 0.85f;
    c->firePlasmaEnergyReleased = 160000.0f;
    c->fireHydrogenEnergyReleased = 284000.0f;
    c->fireGrowthRate = 40000.0f;
    c->plasmaMinimumBurnTemperature = c->T0C + 100.0f;
    c->plasmaUpperTemperature = c->T0C + 1370.0f;
    c->plasmaOxygenFullburn = 10.0f;
    c->plasmaBurnRateDelta = 9.0f;
    c->oxygenBurnRateBase = 1.4f;
    c->superSaturationThreshold = 96.0f;
    c->tritiumBurnOxyFactor = 100.0f;
    c->tritiumBurnTritFactor = 10.0f;
    c->frezonCoolLowerTemperature = 23.15f;
    c->frezonCoolMidTemperature = 373.15f;
    c->frezonCoolMaximumEnergyModifier = 10.0f;
    c->frezonNitrogenCoolRatio = 5.0f;
    c->frezonCoolEnergyReleased = -600000.0f;
    c->frezonCoolRateModifier = 20.0f;
    c->windowHeatTransferCoefficient = 0.1f;
    c->mcellWithRatio = molesCellStandard * 0.005f;

    c->excitedGroupBreakdownCycles = 4;
    c->excitedGroupsDismantleCycles = 16;
    c->monstermosHardTileLimit = 2000;
    c->monstermosTileLimit = 200;
}

inline float tile_total_moles(const TileAtmosData* tile)
{
    float total = 0.0f;
    for (int i = 0; i < ATMOS_GAS_COUNT; i++)
        total += tile->moles[i];
    return total;
}

inline float tile_pressure(const TileAtmosData* tile, float R, float volume)
{
    if (volume <= 0.0f) return 0.0f;
    return tile_total_moles(tile) * R * tile->temperature / volume;
}

inline int opposite_dir(int dir)
{
    switch (dir)
    {
        case ATMOS_DIR_NORTH: return ATMOS_DIR_SOUTH;
        case ATMOS_DIR_SOUTH: return ATMOS_DIR_NORTH;
        case ATMOS_DIR_EAST:  return ATMOS_DIR_WEST;
        case ATMOS_DIR_WEST:  return ATMOS_DIR_EAST;
    }
    return -1;
}

inline uint8_t opposite_dir_bit(uint8_t bit)
{
    switch (bit)
    {
        case ATMOS_DIR_BIT_NORTH: return ATMOS_DIR_BIT_SOUTH;
        case ATMOS_DIR_BIT_SOUTH: return ATMOS_DIR_BIT_NORTH;
        case ATMOS_DIR_BIT_EAST:  return ATMOS_DIR_BIT_WEST;
        case ATMOS_DIR_BIT_WEST:  return ATMOS_DIR_BIT_EAST;
    }
    return 0;
}