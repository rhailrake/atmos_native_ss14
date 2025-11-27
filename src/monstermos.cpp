#include "atmos_internal.h"
#include <stdlib.h>
#include <algorithm>

static void finalize_eq_neighbors(GridAtmosState* state, int32_t tileIndex, const float* transferDirs, const AtmosConfig* config);

static TileAtmosData* s_equalizeTiles[2000];
static TileAtmosData* s_equalizeGiverTiles[200];
static TileAtmosData* s_equalizeTakerTiles[200];
static TileAtmosData* s_equalizeQueue[200];
static TileAtmosData* s_depressurizeTiles[2000];
static TileAtmosData* s_depressurizeSpaceTiles[2000];
static TileAtmosData* s_depressurizeProgressionOrder[4000];

static int tile_mole_delta_compare(const void* a, const void* b)
{
    TileAtmosData* ta = *(TileAtmosData**)a;
    TileAtmosData* tb = *(TileAtmosData**)b;

    if (ta->moleDelta < tb->moleDelta) return -1;
    if (ta->moleDelta > tb->moleDelta) return 1;
    return 0;
}

void equalize_pressure_in_zone(GridAtmosState* state, int32_t startTileIndex, const AtmosConfig* config)
{
    if (!state || !config || startTileIndex < 0 || startTileIndex >= state->tileCount)
        return;

    TileAtmosData* startTile = &state->tiles[startTileIndex];

    if (startTile->lastCycle >= state->updateCounter)
        return;

    startTile->moleDelta = 0;
    startTile->fastDone = 0;
    for (int i = 0; i < ATMOS_DIRECTIONS; i++)
        startTile->transferDirections[i] = 0;

    float startingMoles = tile_total_moles(startTile);
    bool runAtmos = false;

    for (int i = 0; i < ATMOS_DIRECTIONS; i++)
    {
        if (!(startTile->adjacentBits & (1 << i)))
            continue;

        int32_t adjIdx = startTile->adjacentIndices[i];
        if (adjIdx < 0 || adjIdx >= state->tileCount)
            continue;

        TileAtmosData* other = &state->tiles[adjIdx];
        float comparisonMoles = tile_total_moles(other);

        if (simd_abs(comparisonMoles - startingMoles) > config->constants.minimumMolesDeltaToMove)
        {
            runAtmos = true;
            break;
        }
    }

    if (!runAtmos)
    {
        startTile->lastCycle = state->updateCounter;
        return;
    }

    int64_t queueCycle = ++state->equalizationQueueCycle;
    float totalMoles = 0.0f;

    s_equalizeTiles[0] = startTile;
    startTile->lastQueueCycle = queueCycle;
    int tileCount = 1;

    for (int i = 0; i < tileCount; i++)
    {
        if (i > config->constants.monstermosHardTileLimit)
            break;

        TileAtmosData* exploring = s_equalizeTiles[i];

        if (i < config->constants.monstermosTileLimit)
        {
            float tileMoles = tile_total_moles(exploring);
            exploring->moleDelta = tileMoles;
            totalMoles += tileMoles;
        }

        for (int j = 0; j < ATMOS_DIRECTIONS; j++)
        {
            if (!(exploring->adjacentBits & (1 << j)))
                continue;

            int32_t adjIdx = exploring->adjacentIndices[j];
            if (adjIdx < 0 || adjIdx >= state->tileCount)
                continue;

            TileAtmosData* adj = &state->tiles[adjIdx];
            if (adj->lastQueueCycle == queueCycle)
                continue;

            adj->moleDelta = 0;
            adj->fastDone = 0;
            for (int k = 0; k < ATMOS_DIRECTIONS; k++)
                adj->transferDirections[k] = 0;
            adj->lastQueueCycle = queueCycle;

            if (tileCount < config->constants.monstermosHardTileLimit)
                s_equalizeTiles[tileCount++] = adj;

            if ((adj->flags & TILE_FLAG_SPACE) && config->spacingEnabled)
            {
                explosive_depressurize(state, startTileIndex, config);
                return;
            }
        }
    }

    if (tileCount > config->constants.monstermosTileLimit)
    {
        for (int i = config->constants.monstermosTileLimit; i < tileCount; i++)
        {
            TileAtmosData* otherTile = s_equalizeTiles[i];
            if (otherTile)
                otherTile->lastQueueCycle = 0;
        }
        tileCount = config->constants.monstermosTileLimit;
    }

    float averageMoles = totalMoles / tileCount;
    int giverTilesLength = 0;
    int takerTilesLength = 0;

    for (int i = 0; i < tileCount; i++)
    {
        TileAtmosData* otherTile = s_equalizeTiles[i];
        otherTile->lastCycle = state->updateCounter;
        otherTile->moleDelta -= averageMoles;

        if (otherTile->moleDelta > 0)
            s_equalizeGiverTiles[giverTilesLength++] = otherTile;
        else
            s_equalizeTakerTiles[takerTilesLength++] = otherTile;
    }

    float logN = log2f((float)tileCount);

    if (giverTilesLength > logN && takerTilesLength > logN)
    {
        qsort(s_equalizeTiles, tileCount, sizeof(TileAtmosData*), tile_mole_delta_compare);

        for (int i = 0; i < tileCount; i++)
        {
            TileAtmosData* otherTile = s_equalizeTiles[i];
            otherTile->fastDone = 1;

            if (otherTile->moleDelta <= 0)
                continue;

            int eligibleDirectionCount = 0;
            uint8_t eligibleDirections = 0;

            for (int j = 0; j < ATMOS_DIRECTIONS; j++)
            {
                if (!(otherTile->adjacentBits & (1 << j)))
                    continue;

                int32_t adjIdx = otherTile->adjacentIndices[j];
                if (adjIdx < 0 || adjIdx >= state->tileCount)
                    continue;

                TileAtmosData* tile2 = &state->tiles[adjIdx];
                if (tile2->fastDone || tile2->lastQueueCycle != queueCycle)
                    continue;

                eligibleDirections |= (1 << j);
                eligibleDirectionCount++;
            }

            if (eligibleDirectionCount <= 0)
                continue;

            float molesToMove = otherTile->moleDelta / eligibleDirectionCount;

            for (int j = 0; j < ATMOS_DIRECTIONS; j++)
            {
                if (!(eligibleDirections & (1 << j)))
                    continue;

                int32_t adjIdx = otherTile->adjacentIndices[j];
                TileAtmosData* adj = &state->tiles[adjIdx];

                adjust_eq_movement(otherTile, adj, j, molesToMove);
                otherTile->moleDelta -= molesToMove;
                adj->moleDelta += molesToMove;
            }
        }

        giverTilesLength = 0;
        takerTilesLength = 0;

        for (int i = 0; i < tileCount; i++)
        {
            TileAtmosData* otherTile = s_equalizeTiles[i];
            if (otherTile->moleDelta > 0)
                s_equalizeGiverTiles[giverTilesLength++] = otherTile;
            else
                s_equalizeTakerTiles[takerTilesLength++] = otherTile;
        }
    }

    if (giverTilesLength < takerTilesLength)
    {
        for (int j = 0; j < giverTilesLength; j++)
        {
            TileAtmosData* giver = s_equalizeGiverTiles[j];
            giver->currentTransferDirection = -1;
            giver->currentTransferAmount = 0;

            int64_t queueCycleSlow = ++state->equalizationQueueCycle;
            int queueLength = 0;
            s_equalizeQueue[queueLength++] = giver;
            giver->lastSlowQueueCycle = queueCycleSlow;

            for (int i = 0; i < queueLength; i++)
            {
                if (giver->moleDelta <= 0)
                    break;

                TileAtmosData* otherTile = s_equalizeQueue[i];

                for (int k = 0; k < ATMOS_DIRECTIONS; k++)
                {
                    if (giver->moleDelta <= 0)
                        break;

                    if (!(otherTile->adjacentBits & (1 << k)))
                        continue;

                    int32_t adjIdx = otherTile->adjacentIndices[k];
                    if (adjIdx < 0 || adjIdx >= state->tileCount)
                        continue;

                    TileAtmosData* otherTile2 = &state->tiles[adjIdx];
                    if (otherTile2->lastQueueCycle != queueCycle)
                        continue;
                    if (otherTile2->lastSlowQueueCycle == queueCycleSlow)
                        continue;

                    s_equalizeQueue[queueLength++] = otherTile2;
                    otherTile2->lastSlowQueueCycle = queueCycleSlow;
                    otherTile2->currentTransferDirection = opposite_dir(k);
                    otherTile2->currentTransferAmount = 0;

                    if (otherTile2->moleDelta < 0)
                    {
                        if (-otherTile2->moleDelta > giver->moleDelta)
                        {
                            otherTile2->currentTransferAmount -= giver->moleDelta;
                            otherTile2->moleDelta += giver->moleDelta;
                            giver->moleDelta = 0;
                        }
                        else
                        {
                            otherTile2->currentTransferAmount += otherTile2->moleDelta;
                            giver->moleDelta += otherTile2->moleDelta;
                            otherTile2->moleDelta = 0;
                        }
                    }
                }
            }

            for (int i = queueLength - 1; i >= 0; i--)
            {
                TileAtmosData* otherTile = s_equalizeQueue[i];
                if (otherTile->currentTransferAmount == 0 || otherTile->currentTransferDirection < 0)
                    continue;

                int dir = otherTile->currentTransferDirection;
                int32_t adjIdx = otherTile->adjacentIndices[dir];
                if (adjIdx < 0 || adjIdx >= state->tileCount)
                    continue;

                TileAtmosData* adj = &state->tiles[adjIdx];
                adjust_eq_movement(otherTile, adj, dir, otherTile->currentTransferAmount);
                adj->currentTransferAmount += otherTile->currentTransferAmount;
                otherTile->currentTransferAmount = 0;
            }
        }
    }
    else
    {
        for (int j = 0; j < takerTilesLength; j++)
        {
            TileAtmosData* taker = s_equalizeTakerTiles[j];
            taker->currentTransferDirection = -1;
            taker->currentTransferAmount = 0;

            int64_t queueCycleSlow = ++state->equalizationQueueCycle;
            int queueLength = 0;
            s_equalizeQueue[queueLength++] = taker;
            taker->lastSlowQueueCycle = queueCycleSlow;

            for (int i = 0; i < queueLength; i++)
            {
                if (taker->moleDelta >= 0)
                    break;

                TileAtmosData* otherTile = s_equalizeQueue[i];

                for (int k = 0; k < ATMOS_DIRECTIONS; k++)
                {
                    if (taker->moleDelta >= 0)
                        break;

                    if (!(otherTile->adjacentBits & (1 << k)))
                        continue;

                    int32_t adjIdx = otherTile->adjacentIndices[k];
                    if (adjIdx < 0 || adjIdx >= state->tileCount)
                        continue;

                    TileAtmosData* otherTile2 = &state->tiles[adjIdx];
                    if (otherTile2->adjacentBits == 0)
                        continue;
                    if (otherTile2->lastQueueCycle != queueCycle)
                        continue;
                    if (otherTile2->lastSlowQueueCycle == queueCycleSlow)
                        continue;

                    s_equalizeQueue[queueLength++] = otherTile2;
                    otherTile2->lastSlowQueueCycle = queueCycleSlow;
                    otherTile2->currentTransferDirection = opposite_dir(k);
                    otherTile2->currentTransferAmount = 0;

                    if (otherTile2->moleDelta > 0)
                    {
                        if (otherTile2->moleDelta > -taker->moleDelta)
                        {
                            otherTile2->currentTransferAmount -= taker->moleDelta;
                            otherTile2->moleDelta += taker->moleDelta;
                            taker->moleDelta = 0;
                        }
                        else
                        {
                            otherTile2->currentTransferAmount += otherTile2->moleDelta;
                            taker->moleDelta += otherTile2->moleDelta;
                            otherTile2->moleDelta = 0;
                        }
                    }
                }
            }

            for (int i = queueLength - 1; i >= 0; i--)
            {
                TileAtmosData* otherTile = s_equalizeQueue[i];
                if (otherTile->currentTransferAmount == 0 || otherTile->currentTransferDirection < 0)
                    continue;

                int dir = otherTile->currentTransferDirection;
                int32_t adjIdx = otherTile->adjacentIndices[dir];
                if (adjIdx < 0 || adjIdx >= state->tileCount)
                    continue;

                TileAtmosData* adj = &state->tiles[adjIdx];
                adjust_eq_movement(otherTile, adj, dir, otherTile->currentTransferAmount);
                adj->currentTransferAmount += otherTile->currentTransferAmount;
                otherTile->currentTransferAmount = 0;
            }
        }
    }

    for (int i = 0; i < tileCount; i++)
    {
        TileAtmosData* otherTile = s_equalizeTiles[i];
        int32_t otherIdx = (int32_t)(otherTile - state->tiles);
        finalize_eq(state, otherIdx, config);
    }

    for (int i = 0; i < tileCount; i++)
    {
        TileAtmosData* otherTile = s_equalizeTiles[i];
        int32_t otherIdx = (int32_t)(otherTile - state->tiles);

        for (int j = 0; j < ATMOS_DIRECTIONS; j++)
        {
            if (!(otherTile->adjacentBits & (1 << j)))
                continue;

            int32_t adjIdx = otherTile->adjacentIndices[j];
            if (adjIdx < 0 || adjIdx >= state->tileCount)
                continue;

            TileAtmosData* otherTile2 = &state->tiles[adjIdx];
            if (otherTile2->adjacentBits == 0)
                continue;

            int cmp = compare_exchange(otherTile2, startTile, config);
            if (cmp == -2)
                continue;

            add_active_tile_impl(state, adjIdx);
            break;
        }
    }

    for (int i = 0; i < 2000; i++)
        s_equalizeTiles[i] = nullptr;
    for (int i = 0; i < 200; i++)
    {
        s_equalizeGiverTiles[i] = nullptr;
        s_equalizeTakerTiles[i] = nullptr;
        s_equalizeQueue[i] = nullptr;
    }
}

void explosive_depressurize(GridAtmosState* state, int32_t startTileIndex, const AtmosConfig* config)
{
    if (!state || !config || startTileIndex < 0 || startTileIndex >= state->tileCount)
        return;

    if (!config->spacingEnabled)
        return;

    TileAtmosData* startTile = &state->tiles[startTileIndex];

    int64_t queueCycle = ++state->equalizationQueueCycle;
    float totalMolesRemoved = 0.0f;

    int tileCount = 0;
    int spaceTileCount = 0;

    s_depressurizeTiles[tileCount++] = startTile;
    startTile->lastQueueCycle = queueCycle;
    startTile->currentTransferDirection = -1;

    for (int i = 0; i < tileCount; i++)
    {
        TileAtmosData* otherTile = s_depressurizeTiles[i];
        otherTile->lastCycle = state->updateCounter;
        otherTile->currentTransferDirection = -1;

        if (!(otherTile->flags & TILE_FLAG_SPACE))
        {
            for (int j = 0; j < ATMOS_DIRECTIONS; j++)
            {
                int32_t adjIdx = otherTile->adjacentIndices[j];
                if (adjIdx < 0 || adjIdx >= state->tileCount)
                    continue;

                TileAtmosData* otherTile2 = &state->tiles[adjIdx];
                if (otherTile2->lastQueueCycle == queueCycle)
                    continue;

                if (!(otherTile->adjacentBits & (1 << j)))
                    continue;

                otherTile2->lastQueueCycle = queueCycle;
                otherTile2->currentTransferDirection = -1;
                otherTile2->currentTransferAmount = 0;

                if (tileCount < config->constants.monstermosHardTileLimit)
                    s_depressurizeTiles[tileCount++] = otherTile2;
            }
        }
        else
        {
            s_depressurizeSpaceTiles[spaceTileCount++] = otherTile;
        }

        if (tileCount >= config->constants.monstermosHardTileLimit ||
            spaceTileCount >= config->constants.monstermosHardTileLimit)
            break;
    }

    int64_t queueCycleSlow = ++state->equalizationQueueCycle;
    int progressionCount = 0;

    for (int i = 0; i < spaceTileCount; i++)
    {
        TileAtmosData* otherTile = s_depressurizeSpaceTiles[i];
        s_depressurizeProgressionOrder[progressionCount++] = otherTile;
        otherTile->lastSlowQueueCycle = queueCycleSlow;
        otherTile->currentTransferDirection = -1;
    }

    for (int i = 0; i < progressionCount; i++)
    {
        TileAtmosData* otherTile = s_depressurizeProgressionOrder[i];

        for (int j = 0; j < ATMOS_DIRECTIONS; j++)
        {
            if (!(otherTile->adjacentBits & (1 << j)) && !(otherTile->flags & TILE_FLAG_SPACE))
                continue;

            int32_t adjIdx = otherTile->adjacentIndices[j];
            if (adjIdx < 0 || adjIdx >= state->tileCount)
                continue;

            TileAtmosData* tile2 = &state->tiles[adjIdx];
            if (tile2->lastQueueCycle != queueCycle)
                continue;
            if (tile2->lastSlowQueueCycle == queueCycleSlow)
                continue;
            if (tile2->flags & TILE_FLAG_SPACE)
                continue;

            tile2->currentTransferDirection = opposite_dir(j);
            tile2->currentTransferAmount = 0.0f;
            tile2->lastSlowQueueCycle = queueCycleSlow;
            s_depressurizeProgressionOrder[progressionCount++] = tile2;
        }
    }

    for (int i = progressionCount - 1; i >= 0; i--)
    {
        TileAtmosData* otherTile = s_depressurizeProgressionOrder[i];
        if (otherTile->currentTransferDirection < 0)
            continue;

        int32_t otherIdx = (int32_t)(otherTile - state->tiles);
        int dir = otherTile->currentTransferDirection;
        int32_t adjIdx = otherTile->adjacentIndices[dir];

        if (adjIdx < 0 || adjIdx >= state->tileCount)
        {
            if (!(otherTile->flags & TILE_FLAG_IMMUTABLE))
            {
                simd_zero(otherTile->moles, ATMOS_GAS_ARRAY_SIZE);
                otherTile->temperature = config->constants.TCMB;
            }
            continue;
        }

        TileAtmosData* otherTile2 = &state->tiles[adjIdx];

        state->highPressureTiles[state->highPressureTileCount++] = otherIdx;
        add_active_tile_impl(state, otherIdx);

        float sum = tile_total_moles(otherTile);

        if (config->spacingEscapeRatio < 1.0f)
        {
            sum *= config->spacingEscapeRatio;
            if (sum < config->spacingMinGas)
            {
                sum = simd_min(config->spacingMinGas, tile_total_moles(otherTile));
            }
            if (sum + otherTile->currentTransferAmount > config->spacingMaxWind)
            {
                sum = simd_max(config->spacingMinGas, config->spacingMaxWind - otherTile->currentTransferAmount);
            }
        }

        totalMolesRemoved += sum;
        otherTile->currentTransferAmount += sum;
        otherTile2->currentTransferAmount += otherTile->currentTransferAmount;
        otherTile->pressureDifference = otherTile->currentTransferAmount;
        otherTile->currentTransferDirection = dir;

        if (otherTile2->currentTransferDirection < 0)
        {
            otherTile2->pressureDifference = otherTile2->currentTransferAmount;
            otherTile2->currentTransferDirection = dir;
        }

        float currentMoles = tile_total_moles(otherTile);
        float pressure = tile_pressure(otherTile, config->constants.R, config->constants.cellVolume);

        if (pressure - sum > config->spacingMinGas * 0.1f && !(otherTile->flags & TILE_FLAG_IMMUTABLE))
        {
            float ratio = sum / currentMoles;
            if (ratio > 1.0f) ratio = 1.0f;

            for (int g = 0; g < ATMOS_GAS_COUNT; g++)
            {
                float transfer = otherTile->moles[g] * ratio;
                otherTile->moles[g] -= transfer;
                if (!(otherTile2->flags & TILE_FLAG_IMMUTABLE))
                    otherTile2->moles[g] += transfer * 0.7f;
            }

            if (otherTile->temperature > 280.0f)
            {
                float remaining = currentMoles - sum;
                float temploss = (remaining > 0) ? (remaining / currentMoles) : 0.0f;
                otherTile->temperature *= 0.9f + 0.1f * temploss;
            }
        }
        else if (!(otherTile->flags & TILE_FLAG_IMMUTABLE))
        {
            simd_zero(otherTile->moles, ATMOS_GAS_ARRAY_SIZE);
            otherTile->temperature = config->constants.TCMB;
        }
    }

    for (int i = 0; i < 2000; i++)
    {
        s_depressurizeTiles[i] = nullptr;
        s_depressurizeSpaceTiles[i] = nullptr;
    }
    for (int i = 0; i < 4000; i++)
        s_depressurizeProgressionOrder[i] = nullptr;
}

void adjust_eq_movement(TileAtmosData* tile, TileAtmosData* adj, int direction, float amount)
{
    if (!tile || !adj || direction < 0 || direction >= ATMOS_DIRECTIONS)
        return;

    tile->transferDirections[direction] += amount;
    adj->transferDirections[opposite_dir(direction)] -= amount;
}

void finalize_eq(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config)
{
    if (!state || !config || tileIndex < 0 || tileIndex >= state->tileCount)
        return;

    TileAtmosData* tile = &state->tiles[tileIndex];

    float transferDirections[ATMOS_DIRECTIONS];
    bool hasTransferDirs = false;

    for (int i = 0; i < ATMOS_DIRECTIONS; i++)
    {
        float amount = tile->transferDirections[i];
        if (amount == 0)
        {
            transferDirections[i] = 0;
            continue;
        }

        transferDirections[i] = amount;
        tile->transferDirections[i] = 0;
        hasTransferDirs = true;
    }

    if (!hasTransferDirs)
        return;

    for (int i = 0; i < ATMOS_DIRECTIONS; i++)
    {
        if (!(tile->adjacentBits & (1 << i)))
            continue;

        float amount = transferDirections[i];
        if (amount <= 0)
            continue;

        int32_t adjIdx = tile->adjacentIndices[i];
        if (adjIdx < 0 || adjIdx >= state->tileCount)
            continue;

        TileAtmosData* otherTile = &state->tiles[adjIdx];

        float totalMoles = tile_total_moles(tile);
        if (totalMoles < amount)
        {
            finalize_eq_neighbors(state, tileIndex, transferDirections, config);
            totalMoles = tile_total_moles(tile);
        }

        if (totalMoles <= 0)
            continue;

        float ratio = amount / totalMoles;
        if (ratio > 1.0f) ratio = 1.0f;

        otherTile->transferDirections[opposite_dir(i)] = 0;

        if (!(tile->flags & TILE_FLAG_IMMUTABLE) && !(otherTile->flags & TILE_FLAG_IMMUTABLE))
        {
            for (int g = 0; g < ATMOS_GAS_COUNT; g++)
            {
                float transfer = tile->moles[g] * ratio;
                tile->moles[g] -= transfer;
                otherTile->moles[g] += transfer;
            }

            merge_impl(otherTile, tile->moles, tile->temperature,
                      config->gasSpecificHeats,
                      config->constants.minimumTemperatureDeltaToConsider,
                      config->constants.minimumHeatCapacity);
        }

        consider_pressure_difference(state, tileIndex, i, amount);
    }
}

static void finalize_eq_neighbors(GridAtmosState* state, int32_t tileIndex, const float* transferDirs, const AtmosConfig* config)
{
    TileAtmosData* tile = &state->tiles[tileIndex];

    for (int i = 0; i < ATMOS_DIRECTIONS; i++)
    {
        float amount = transferDirs[i];
        if (amount >= 0)
            continue;

        if (!(tile->adjacentBits & (1 << i)))
            continue;

        int32_t adjIdx = tile->adjacentIndices[i];
        if (adjIdx >= 0 && adjIdx < state->tileCount)
        {
            finalize_eq(state, adjIdx, config);
        }
    }
}