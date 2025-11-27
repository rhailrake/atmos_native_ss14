#include "atmos_internal.h"

void share_impl(TileAtmosData* receiver, TileAtmosData* sharer, int adjacentCount, const AtmosConfig* config)
{
    if (!receiver || !sharer || !config)
        return;

    bool receiverImmutable = (receiver->flags & TILE_FLAG_IMMUTABLE) != 0;
    bool sharerImmutable = (sharer->flags & TILE_FLAG_IMMUTABLE) != 0;

    float temperatureDelta = receiver->temperatureArchived - sharer->temperatureArchived;
    float absTemperatureDelta = simd_abs(temperatureDelta);

    float oldHeatCapacity = 0.0f;
    float oldSharerHeatCapacity = 0.0f;

    if (absTemperatureDelta > config->constants.minimumTemperatureDeltaToConsider)
    {
        oldHeatCapacity = get_heat_capacity_impl(receiver->moles, config->gasSpecificHeats, (receiver->flags & TILE_FLAG_SPACE) != 0);
        oldSharerHeatCapacity = get_heat_capacity_impl(sharer->moles, config->gasSpecificHeats, (sharer->flags & TILE_FLAG_SPACE) != 0);
    }

    float heatCapacityToSharer = 0.0f;
    float heatCapacitySharerToThis = 0.0f;
    float movedMoles = 0.0f;
    float absMovedMoles = 0.0f;

    float divisor = 1.0f / (adjacentCount + 1);

    for (int i = 0; i < ATMOS_GAS_COUNT; i++)
    {
        float thisValue = receiver->moles[i];
        float sharerValue = sharer->moles[i];
        float delta = (thisValue - sharerValue) * divisor;

        if (simd_abs(delta) < config->constants.gasMinMoles)
            continue;

        if (absTemperatureDelta > config->constants.minimumTemperatureDeltaToConsider)
        {
            float gasHeatCapacity = delta * config->gasSpecificHeats[i];
            if (delta > 0)
                heatCapacityToSharer += gasHeatCapacity;
            else
                heatCapacitySharerToThis -= gasHeatCapacity;
        }

        if (!receiverImmutable)
            receiver->moles[i] -= delta;
        if (!sharerImmutable)
            sharer->moles[i] += delta;

        movedMoles += delta;
        absMovedMoles += simd_abs(delta);
    }

    receiver->lastShare = absMovedMoles;

    if (absTemperatureDelta > config->constants.minimumTemperatureDeltaToConsider)
    {
        float newHeatCapacity = oldHeatCapacity + heatCapacitySharerToThis - heatCapacityToSharer;
        float newSharerHeatCapacity = oldSharerHeatCapacity + heatCapacityToSharer - heatCapacitySharerToThis;

        if (!receiverImmutable && newHeatCapacity > config->constants.minimumHeatCapacity)
        {
            receiver->temperature = ((oldHeatCapacity * receiver->temperature) -
                                     (heatCapacityToSharer * receiver->temperatureArchived) +
                                     (heatCapacitySharerToThis * sharer->temperatureArchived)) / newHeatCapacity;
        }

        if (!sharerImmutable && newSharerHeatCapacity > config->constants.minimumHeatCapacity)
        {
            sharer->temperature = ((oldSharerHeatCapacity * sharer->temperature) -
                                   (heatCapacitySharerToThis * sharer->temperatureArchived) +
                                   (heatCapacityToSharer * receiver->temperatureArchived)) / newSharerHeatCapacity;
        }

        if (simd_abs(oldSharerHeatCapacity) > config->constants.minimumHeatCapacity)
        {
            if (simd_abs(newSharerHeatCapacity / oldSharerHeatCapacity - 1.0f) < 0.1f)
            {
                temperature_share_impl(receiver, sharer, config->constants.openHeatTransferCoefficient, config);
            }
        }
    }
}

float temperature_share_impl(TileAtmosData* receiver, TileAtmosData* sharer, float conductionCoefficient, const AtmosConfig* config)
{
    if (!receiver || !sharer || !config)
        return 0.0f;

    float temperatureDelta = receiver->temperatureArchived - sharer->temperatureArchived;

    if (simd_abs(temperatureDelta) > config->constants.minimumTemperatureDeltaToConsider)
    {
        float heatCapacity = get_heat_capacity_archived_impl(receiver, config->gasSpecificHeats);
        float sharerHeatCapacity = get_heat_capacity_archived_impl(sharer, config->gasSpecificHeats);

        if (sharerHeatCapacity > config->constants.minimumHeatCapacity &&
            heatCapacity > config->constants.minimumHeatCapacity)
        {
            float heat = conductionCoefficient * temperatureDelta *
                        (heatCapacity * sharerHeatCapacity / (heatCapacity + sharerHeatCapacity));

            if (!(receiver->flags & TILE_FLAG_IMMUTABLE))
            {
                receiver->temperature = simd_max(receiver->temperature - heat / heatCapacity, config->constants.TCMB);
            }

            if (!(sharer->flags & TILE_FLAG_IMMUTABLE))
            {
                sharer->temperature = simd_max(sharer->temperature + heat / sharerHeatCapacity, config->constants.TCMB);
            }
        }
    }

    return sharer->temperature;
}

float temperature_share_solid(TileAtmosData* receiver, float conductionCoefficient, float sharerTemp, float sharerHeatCapacity, const AtmosConfig* config)
{
    if (!receiver || !config)
        return sharerTemp;

    float temperatureDelta = receiver->temperatureArchived - sharerTemp;

    if (simd_abs(temperatureDelta) > config->constants.minimumTemperatureDeltaToConsider)
    {
        float heatCapacity = get_heat_capacity_archived_impl(receiver, config->gasSpecificHeats);

        if (sharerHeatCapacity > config->constants.minimumHeatCapacity &&
            heatCapacity > config->constants.minimumHeatCapacity)
        {
            float heat = conductionCoefficient * temperatureDelta *
                        (heatCapacity * sharerHeatCapacity / (heatCapacity + sharerHeatCapacity));

            if (!(receiver->flags & TILE_FLAG_IMMUTABLE))
            {
                receiver->temperature = simd_max(receiver->temperature - heat / heatCapacity, config->constants.TCMB);
            }

            sharerTemp = simd_max(sharerTemp + heat / sharerHeatCapacity, config->constants.TCMB);
        }
    }

    return sharerTemp;
}

void process_cell(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config)
{
    if (!state || !config || tileIndex < 0 || tileIndex >= state->tileCount)
        return;

    TileAtmosData* tile = &state->tiles[tileIndex];

    if (tile->flags & TILE_FLAG_IMMUTABLE)
    {
        remove_active_tile_impl(state, tileIndex, true);
        return;
    }

    tile->lastCycle = state->updateCounter;

    int adjacentTileLength = 0;
    for (int i = 0; i < ATMOS_DIRECTIONS; i++)
    {
        if (tile->adjacentBits & (1 << i))
            adjacentTileLength++;
    }

    for (int i = 0; i < ATMOS_DIRECTIONS; i++)
    {
        uint8_t dirBit = (1 << i);
        if (!(tile->adjacentBits & dirBit))
            continue;

        int32_t adjIndex = tile->adjacentIndices[i];
        if (adjIndex < 0 || adjIndex >= state->tileCount)
            continue;

        TileAtmosData* enemyTile = &state->tiles[adjIndex];
        if (enemyTile->flags & TILE_FLAG_IMMUTABLE)
            continue;

        if (state->updateCounter <= enemyTile->lastCycle)
            continue;

        bool shouldShareAir = false;

        if (config->excitedGroupsEnabled &&
            tile->excitedGroupId >= 0 &&
            enemyTile->excitedGroupId >= 0)
        {
            if (tile->excitedGroupId != enemyTile->excitedGroupId)
            {
                merge_excited_groups(state, tile->excitedGroupId, enemyTile->excitedGroupId);
            }
            shouldShareAir = true;
        }
        else
        {
            int exchangeResult = compare_exchange(tile, enemyTile, config);
            if (exchangeResult != -2)
            {
                add_active_tile_impl(state, adjIndex);

                if (config->excitedGroupsEnabled)
                {
                    int32_t groupId = tile->excitedGroupId;
                    if (groupId < 0)
                        groupId = enemyTile->excitedGroupId;
                    if (groupId < 0)
                    {
                        groupId = create_excited_group(state);
                    }

                    if (groupId >= 0)
                    {
                        if (tile->excitedGroupId < 0)
                            add_tile_to_excited_group(state, groupId, tileIndex);
                        if (enemyTile->excitedGroupId < 0)
                            add_tile_to_excited_group(state, groupId, adjIndex);
                    }
                }

                shouldShareAir = true;
            }
        }

        if (shouldShareAir)
        {
            share_impl(tile, enemyTile, adjacentTileLength, config);

            if (!config->monstermosEnabled)
            {
                float pressure1 = tile_pressure(tile, config->constants.R, config->constants.cellVolume);
                float pressure2 = tile_pressure(enemyTile, config->constants.R, config->constants.cellVolume);
                float difference = pressure1 - pressure2;

                if (difference >= 0)
                {
                    consider_pressure_difference(state, tileIndex, i, difference);
                }
                else
                {
                    consider_pressure_difference(state, adjIndex, opposite_dir(i), -difference);
                }
            }

            last_share_check(state, tile, config);
        }
    }

    react_impl(tile, config);

    if (tile->temperature > config->constants.minimumTemperatureStartSuperConduction)
    {
        if (consider_superconductivity(state, tileIndex, true, config))
        {
            return;
        }
    }

    if (config->excitedGroupsEnabled && tile->excitedGroupId < 0)
    {
        remove_active_tile_impl(state, tileIndex, true);
    }
}

void last_share_check(GridAtmosState* state, TileAtmosData* tile, const AtmosConfig* config)
{
    if (!state || !tile || !config)
        return;

    if (tile->excitedGroupId < 0)
        return;

    if (tile->lastShare > config->constants.minimumAirToSuspend)
    {
        reset_excited_group_cooldowns(state, tile->excitedGroupId);
    }
    else if (tile->lastShare > config->constants.minimumMolesDeltaToMove)
    {
        if (tile->excitedGroupId >= 0 && tile->excitedGroupId < state->excitedGroupCount)
        {
            state->excitedGroups[tile->excitedGroupId].dismantleCooldown = 0;
        }
    }
}