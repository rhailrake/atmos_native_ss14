#include "atmos_internal.h"
#include <string.h>

float get_heat_capacity_impl(const float* moles, const float* specificHeats, bool space)
{
    float totalMoles = simd_horizontal_add(moles, ATMOS_GAS_COUNT);

    if (space && totalMoles < 0.0001f)
    {
        return 7000.0f;
    }

    float heatCapacity = simd_dot_product(moles, specificHeats, ATMOS_GAS_ARRAY_SIZE);
    return simd_max(heatCapacity, 0.0003f);
}

float get_heat_capacity_archived_impl(const TileAtmosData* tile, const float* specificHeats)
{
    if (tile->flags & TILE_FLAG_SPACE)
    {
        float totalMoles = simd_horizontal_add(tile->molesArchived, ATMOS_GAS_COUNT);
        if (totalMoles < 0.0001f)
            return 7000.0f;
    }

    float heatCapacity = simd_dot_product(tile->molesArchived, specificHeats, ATMOS_GAS_ARRAY_SIZE);
    return simd_max(heatCapacity, 0.0003f);
}

float get_thermal_energy_impl(const TileAtmosData* tile, const float* specificHeats)
{
    float heatCapacity = get_heat_capacity_impl(tile->moles, specificHeats, (tile->flags & TILE_FLAG_SPACE) != 0);
    return tile->temperature * heatCapacity;
}

void archive_tile(TileAtmosData* tile)
{
    simd_copy(tile->molesArchived, tile->moles, ATMOS_GAS_ARRAY_SIZE);
    tile->temperatureArchived = tile->temperature;
}

int compare_exchange(const TileAtmosData* a, const TileAtmosData* b, const AtmosConfig* config)
{
    float totalMoles = 0.0f;

    for (int i = 0; i < ATMOS_GAS_COUNT; i++)
    {
        float gasMoles = a->moles[i];
        float delta = simd_abs(gasMoles - b->moles[i]);

        if (delta > config->constants.minimumMolesDeltaToMove &&
            delta > gasMoles * config->constants.minimumAirRatioToMove)
        {
            return i;
        }
        totalMoles += gasMoles;
    }

    if (totalMoles > config->constants.minimumMolesDeltaToMove)
    {
        float tempDelta = simd_abs(a->temperature - b->temperature);
        if (tempDelta > config->constants.minimumTemperatureDeltaToSuspend)
            return -1;
    }

    return -2;
}

void merge_impl(TileAtmosData* receiver, const float* giverMoles, float giverTemp,
                const float* specificHeats, float minTempDelta, float minHeatCapacity)
{
    if (receiver->flags & TILE_FLAG_IMMUTABLE)
        return;

    if (simd_abs(receiver->temperature - giverTemp) > minTempDelta)
    {
        float receiverHC = get_heat_capacity_impl(receiver->moles, specificHeats, (receiver->flags & TILE_FLAG_SPACE) != 0);
        float giverHC = get_heat_capacity_impl(giverMoles, specificHeats, false);
        float combinedHC = receiverHC + giverHC;

        if (combinedHC > minHeatCapacity)
        {
            float receiverEnergy = receiver->temperature * receiverHC;
            float giverEnergy = giverTemp * giverHC;
            receiver->temperature = (receiverEnergy + giverEnergy) / combinedHC;
        }
    }

    simd_add_arrays(receiver->moles, giverMoles, ATMOS_GAS_ARRAY_SIZE);
}

void remove_gas_impl(TileAtmosData* tile, float amount, TileAtmosData* removed)
{
    float totalMoles = tile_total_moles(tile);
    if (totalMoles <= 0.0f)
    {
        if (removed)
        {
            simd_zero(removed->moles, ATMOS_GAS_ARRAY_SIZE);
            removed->temperature = tile->temperature;
        }
        return;
    }

    float ratio = amount / totalMoles;
    remove_ratio_impl(tile, ratio, removed, 0.00000005f);
}

void remove_ratio_impl(TileAtmosData* tile, float ratio, TileAtmosData* removed, float gasMinMoles)
{
    if (ratio <= 0.0f)
    {
        if (removed)
        {
            simd_zero(removed->moles, ATMOS_GAS_ARRAY_SIZE);
            removed->temperature = tile->temperature;
        }
        return;
    }

    if (ratio > 1.0f)
        ratio = 1.0f;

    if (removed)
    {
        simd_copy(removed->moles, tile->moles, ATMOS_GAS_ARRAY_SIZE);
        simd_mul_scalar(removed->moles, ratio, ATMOS_GAS_ARRAY_SIZE);
        removed->temperature = tile->temperature;
    }

    if (!(tile->flags & TILE_FLAG_IMMUTABLE))
    {
        for (int i = 0; i < ATMOS_GAS_COUNT; i++)
        {
            tile->moles[i] -= tile->moles[i] * ratio;
            if (tile->moles[i] < gasMinMoles)
                tile->moles[i] = 0.0f;
        }
    }
}

void add_active_tile_impl(GridAtmosState* state, int32_t tileIndex)
{
    if (!state || tileIndex < 0 || tileIndex >= state->tileCount)
        return;

    TileAtmosData* tile = &state->tiles[tileIndex];

    if (tile->flags & TILE_FLAG_EXCITED)
        return;

    tile->flags |= TILE_FLAG_EXCITED;

    ensure_active_capacity(state, state->activeTileCount + 1);
    state->activeTiles[state->activeTileCount++] = tileIndex;
}

void remove_active_tile_impl(GridAtmosState* state, int32_t tileIndex, bool disposeGroup)
{
    if (!state || tileIndex < 0 || tileIndex >= state->tileCount)
        return;

    TileAtmosData* tile = &state->tiles[tileIndex];

    if (!(tile->flags & TILE_FLAG_EXCITED))
        return;

    tile->flags &= ~TILE_FLAG_EXCITED;

    for (int i = 0; i < state->activeTileCount; i++)
    {
        if (state->activeTiles[i] == tileIndex)
        {
            state->activeTiles[i] = state->activeTiles[--state->activeTileCount];
            break;
        }
    }

    if (tile->excitedGroupId >= 0 && disposeGroup)
    {
        dispose_excited_group(state, tile->excitedGroupId);
    }
    else if (tile->excitedGroupId >= 0)
    {
        remove_tile_from_excited_group(state, tile->excitedGroupId, tileIndex);
    }

    tile->excitedGroupId = -1;
}

int32_t create_excited_group(GridAtmosState* state)
{
    ensure_excited_group_capacity(state, state->excitedGroupCount + 1);

    for (int i = 0; i < state->excitedGroupCapacity; i++)
    {
        if (state->excitedGroups[i].disposed || state->excitedGroups[i].tileCount == 0)
        {
            ExcitedGroupData* group = &state->excitedGroups[i];
            group->id = i;
            group->breakdownCooldown = 0;
            group->dismantleCooldown = 0;
            group->tileCount = 0;
            group->disposed = 0;

            if (!group->tileIndices)
            {
                group->tileCapacity = 64;
                group->tileIndices = (int32_t*)malloc(64 * sizeof(int32_t));
            }

            if (i >= state->excitedGroupCount)
                state->excitedGroupCount = i + 1;

            return i;
        }
    }

    return -1;
}

void add_tile_to_excited_group(GridAtmosState* state, int32_t groupId, int32_t tileIndex)
{
    if (groupId < 0 || groupId >= state->excitedGroupCount)
        return;

    ExcitedGroupData* group = &state->excitedGroups[groupId];
    if (group->disposed)
        return;

    TileAtmosData* tile = &state->tiles[tileIndex];
    tile->excitedGroupId = groupId;

    if (group->tileCount >= group->tileCapacity)
    {
        int32_t newCapacity = group->tileCapacity * 2;
        group->tileIndices = (int32_t*)realloc(group->tileIndices, newCapacity * sizeof(int32_t));
        group->tileCapacity = newCapacity;
    }

    group->tileIndices[group->tileCount++] = tileIndex;
}

void remove_tile_from_excited_group(GridAtmosState* state, int32_t groupId, int32_t tileIndex)
{
    if (groupId < 0 || groupId >= state->excitedGroupCount)
        return;

    ExcitedGroupData* group = &state->excitedGroups[groupId];
    if (group->disposed)
        return;

    for (int i = 0; i < group->tileCount; i++)
    {
        if (group->tileIndices[i] == tileIndex)
        {
            group->tileIndices[i] = group->tileIndices[--group->tileCount];
            break;
        }
    }

    state->tiles[tileIndex].excitedGroupId = -1;
}

void merge_excited_groups(GridAtmosState* state, int32_t group1, int32_t group2)
{
    if (group1 == group2)
        return;

    if (group1 < 0 || group1 >= state->excitedGroupCount)
        return;
    if (group2 < 0 || group2 >= state->excitedGroupCount)
        return;

    ExcitedGroupData* g1 = &state->excitedGroups[group1];
    ExcitedGroupData* g2 = &state->excitedGroups[group2];

    if (g1->disposed || g2->disposed)
        return;

    int32_t neededCapacity = g1->tileCount + g2->tileCount;
    if (neededCapacity > g1->tileCapacity)
    {
        int32_t newCapacity = g1->tileCapacity;
        while (newCapacity < neededCapacity)
            newCapacity *= 2;
        g1->tileIndices = (int32_t*)realloc(g1->tileIndices, newCapacity * sizeof(int32_t));
        g1->tileCapacity = newCapacity;
    }

    for (int i = 0; i < g2->tileCount; i++)
    {
        int32_t tileIdx = g2->tileIndices[i];
        state->tiles[tileIdx].excitedGroupId = group1;
        g1->tileIndices[g1->tileCount++] = tileIdx;
    }

    g2->tileCount = 0;
    g2->disposed = 1;
}

void dispose_excited_group(GridAtmosState* state, int32_t groupId)
{
    if (groupId < 0 || groupId >= state->excitedGroupCount)
        return;

    ExcitedGroupData* group = &state->excitedGroups[groupId];
    if (group->disposed)
        return;

    for (int i = 0; i < group->tileCount; i++)
    {
        int32_t tileIdx = group->tileIndices[i];
        if (tileIdx >= 0 && tileIdx < state->tileCount)
            state->tiles[tileIdx].excitedGroupId = -1;
    }

    group->tileCount = 0;
    group->disposed = 1;
}

void reset_excited_group_cooldowns(GridAtmosState* state, int32_t groupId)
{
    if (groupId < 0 || groupId >= state->excitedGroupCount)
        return;

    ExcitedGroupData* group = &state->excitedGroups[groupId];
    if (group->disposed)
        return;

    group->breakdownCooldown = 0;
    group->dismantleCooldown = 0;
}

void excited_group_self_breakdown(GridAtmosState* state, int32_t groupId, const AtmosConfig* config)
{
    if (groupId < 0 || groupId >= state->excitedGroupCount)
        return;

    ExcitedGroupData* group = &state->excitedGroups[groupId];
    if (group->disposed || group->tileCount == 0)
        return;

    float combinedHeatCapacity = 0.0f;
    float combinedTemperature = 0.0f;
    float combinedMoles[ATMOS_GAS_ARRAY_SIZE] = {0};

    for (int i = 0; i < group->tileCount; i++)
    {
        int32_t tileIdx = group->tileIndices[i];
        TileAtmosData* tile = &state->tiles[tileIdx];

        if (tile->flags & TILE_FLAG_IMMUTABLE)
            continue;

        float tileHC = get_heat_capacity_impl(tile->moles, config->gasSpecificHeats, (tile->flags & TILE_FLAG_SPACE) != 0);
        combinedHeatCapacity += tileHC;
        combinedTemperature += tile->temperature * tileHC;

        for (int g = 0; g < ATMOS_GAS_COUNT; g++)
            combinedMoles[g] += tile->moles[g];
    }

    if (combinedHeatCapacity > config->constants.minimumHeatCapacity)
        combinedTemperature /= combinedHeatCapacity;

    int mutableTiles = 0;
    for (int i = 0; i < group->tileCount; i++)
    {
        int32_t tileIdx = group->tileIndices[i];
        TileAtmosData* tile = &state->tiles[tileIdx];
        if (!(tile->flags & TILE_FLAG_IMMUTABLE))
            mutableTiles++;
    }

    if (mutableTiles > 0)
    {
        float divisor = 1.0f / mutableTiles;
        for (int i = 0; i < group->tileCount; i++)
        {
            int32_t tileIdx = group->tileIndices[i];
            TileAtmosData* tile = &state->tiles[tileIdx];

            if (tile->flags & TILE_FLAG_IMMUTABLE)
                continue;

            tile->temperature = combinedTemperature;
            for (int g = 0; g < ATMOS_GAS_COUNT; g++)
                tile->moles[g] = combinedMoles[g] * divisor;
        }
    }

    group->breakdownCooldown = 0;
}

void deactivate_group_tiles(GridAtmosState* state, int32_t groupId)
{
    if (groupId < 0 || groupId >= state->excitedGroupCount)
        return;

    ExcitedGroupData* group = &state->excitedGroups[groupId];
    if (group->disposed)
        return;

    for (int i = 0; i < group->tileCount; i++)
    {
        int32_t tileIdx = group->tileIndices[i];
        remove_active_tile_impl(state, tileIdx, false);
    }

    dispose_excited_group(state, groupId);
}

void consider_pressure_difference(GridAtmosState* state, int32_t tileIndex, int direction, float pressureDiff)
{
    if (!state || tileIndex < 0 || tileIndex >= state->tileCount)
        return;

    TileAtmosData* tile = &state->tiles[tileIndex];

    if (simd_abs(pressureDiff) > tile->pressureDifference)
    {
        tile->pressureDifference = simd_abs(pressureDiff);
        tile->currentTransferDirection = direction;

        bool found = false;
        for (int i = 0; i < state->highPressureTileCount; i++)
        {
            if (state->highPressureTiles[i] == tileIndex)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            state->highPressureTiles[state->highPressureTileCount++] = tileIndex;
        }
    }
}

void high_pressure_movements(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config)
{
    (void)state;
    (void)tileIndex;
    (void)config;
}