#include "atmos_internal.h"

static uint8_t conductivity_directions(GridAtmosState* state, TileAtmosData* tile, const AtmosConfig* config)
{
    (void)state;
    (void)config;

    if (tile->flags & TILE_FLAG_IMMUTABLE)
    {
        archive_tile(tile);
        return ATMOS_DIR_BIT_ALL;
    }

    return ATMOS_DIR_BIT_ALL;
}

void superconduct(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config)
{
    if (!state || !config || tileIndex < 0 || tileIndex >= state->tileCount)
        return;

    TileAtmosData* tile = &state->tiles[tileIndex];
    uint8_t directions = conductivity_directions(state, tile, config);

    for (int i = 0; i < ATMOS_DIRECTIONS; i++)
    {
        if (!(directions & (1 << i)))
            continue;

        int32_t adjIdx = tile->adjacentIndices[i];
        if (adjIdx < 0 || adjIdx >= state->tileCount)
            continue;

        TileAtmosData* adjacent = &state->tiles[adjIdx];
        if (adjacent->thermalConductivity == 0.0f)
            continue;

        if (adjacent->lastCycle < state->updateCounter)
            archive_tile(adjacent);

        neighbor_conduct_with_source(state, adjIdx, tileIndex, config);
        consider_superconductivity(state, adjIdx, false, config);
    }

    radiate_to_space(tile, config);
    finish_superconductivity(state, tileIndex, (tile->flags & TILE_FLAG_IMMUTABLE) ? tile->temperature : tile->temperature, config);
}

bool consider_superconductivity(GridAtmosState* state, int32_t tileIndex, bool starting, const AtmosConfig* config)
{
    if (!state || !config || tileIndex < 0 || tileIndex >= state->tileCount)
        return false;

    if (!config->superconductionEnabled)
        return false;

    TileAtmosData* tile = &state->tiles[tileIndex];

    if (tile->thermalConductivity == 0.0f)
        return false;

    float minTemp = starting
        ? config->constants.minimumTemperatureStartSuperConduction
        : config->constants.minimumTemperatureForSuperconduction;

    if (tile->temperature < minTemp)
        return false;

    float heatCapacity = get_heat_capacity_impl(tile->moles, config->gasSpecificHeats, (tile->flags & TILE_FLAG_SPACE) != 0);
    if (heatCapacity < config->constants.mcellWithRatio)
        return false;

    if (tile->flags & TILE_FLAG_SUPERCONDUCT)
        return true;

    tile->flags |= TILE_FLAG_SUPERCONDUCT;

    bool found = false;
    for (int i = 0; i < state->superconductTileCount; i++)
    {
        if (state->superconductTiles[i] == tileIndex)
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        state->superconductTiles[state->superconductTileCount++] = tileIndex;
    }

    return true;
}

void neighbor_conduct_with_source(GridAtmosState* state, int32_t tileIndex, int32_t sourceIndex, const AtmosConfig* config)
{
    if (!state || !config)
        return;
    if (tileIndex < 0 || tileIndex >= state->tileCount)
        return;
    if (sourceIndex < 0 || sourceIndex >= state->tileCount)
        return;

    TileAtmosData* tile = &state->tiles[tileIndex];
    TileAtmosData* other = &state->tiles[sourceIndex];

    bool tileHasAir = !(tile->flags & TILE_FLAG_IMMUTABLE) && tile_total_moles(tile) > 0;
    bool otherHasAir = !(other->flags & TILE_FLAG_IMMUTABLE) && tile_total_moles(other) > 0;

    if (!tileHasAir)
    {
        if (otherHasAir)
        {
            tile->temperature = temperature_share_solid(other, tile->thermalConductivity, tile->temperature, tile->heatCapacity, config);
        }
        else
        {
            float deltaTemperature = tile->temperatureArchived - other->temperatureArchived;
            if (simd_abs(deltaTemperature) > config->constants.minimumTemperatureDeltaToConsider &&
                tile->heatCapacity != 0.0f && other->heatCapacity != 0.0f)
            {
                float heat = tile->thermalConductivity * deltaTemperature *
                            (tile->heatCapacity * other->heatCapacity / (tile->heatCapacity + other->heatCapacity));

                tile->temperature -= heat / tile->heatCapacity;
                other->temperature += heat / other->heatCapacity;
            }
        }
        return;
    }

    if (otherHasAir)
    {
        temperature_share_impl(other, tile, config->constants.windowHeatTransferCoefficient, config);
    }
    else
    {
        other->temperature = temperature_share_solid(tile, other->thermalConductivity, other->temperature, other->heatCapacity, config);
    }

    add_active_tile_impl(state, tileIndex);
}

void radiate_to_space(TileAtmosData* tile, const AtmosConfig* config)
{
    if (!tile || !config)
        return;

    if (tile->temperature <= config->constants.T0C)
        return;

    float deltaTemperature = tile->temperatureArchived - config->constants.TCMB;
    if (tile->heatCapacity > 0 && simd_abs(deltaTemperature) > config->constants.minimumTemperatureDeltaToConsider)
    {
        float heat = tile->thermalConductivity * deltaTemperature *
                    (tile->heatCapacity * config->constants.heatCapacityVacuum /
                     (tile->heatCapacity + config->constants.heatCapacityVacuum));

        if (!(tile->flags & TILE_FLAG_IMMUTABLE))
        {
            tile->temperature -= heat;
            tile->temperature = simd_max(tile->temperature, config->constants.TCMB);
        }
    }
}

void finish_superconductivity(GridAtmosState* state, int32_t tileIndex, float temperature, const AtmosConfig* config)
{
    if (!state || !config || tileIndex < 0 || tileIndex >= state->tileCount)
        return;

    TileAtmosData* tile = &state->tiles[tileIndex];

    if (tile_total_moles(tile) > 0 && !(tile->flags & TILE_FLAG_IMMUTABLE))
    {
        tile->temperature = temperature_share_solid(tile, tile->thermalConductivity, tile->temperature, tile->heatCapacity, config);
    }

    if (temperature < config->constants.minimumTemperatureForSuperconduction)
    {
        tile->flags &= ~TILE_FLAG_SUPERCONDUCT;

        for (int i = 0; i < state->superconductTileCount; i++)
        {
            if (state->superconductTiles[i] == tileIndex)
            {
                state->superconductTiles[i] = state->superconductTiles[--state->superconductTileCount];
                break;
            }
        }
    }
}