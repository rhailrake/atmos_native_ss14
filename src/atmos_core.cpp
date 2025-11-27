#define ATMOS_EXPORT
#include "atmos_api.h"
#include "atmos_internal.h"
#include <stdlib.h>
#include <string.h>
#include <chrono>

#define ATMOS_VERSION 1

static int g_simdLevel = -1;

static int detect_simd_level()
{
#if defined(__AVX512F__)
    return 3;
#elif defined(__AVX2__)
    return 2;
#elif defined(__SSE4_1__)
    return 1;
#else
    return 0;
#endif
}

extern "C"
{

ATMOS_API uint32_t atmos_get_version()
{
    return ATMOS_VERSION;
}

ATMOS_API uint32_t atmos_get_simd_level()
{
    if (g_simdLevel < 0)
        g_simdLevel = detect_simd_level();
    return g_simdLevel;
}

ATMOS_API GridAtmosState* atmos_create_grid(int32_t initialCapacity)
{
    if (initialCapacity < 64)
        initialCapacity = 64;

    GridAtmosState* state = (GridAtmosState*)calloc(1, sizeof(GridAtmosState));
    if (!state) return nullptr;

    state->tileCapacity = initialCapacity;
    state->tiles = (TileAtmosData*)calloc(initialCapacity, sizeof(TileAtmosData));
    if (!state->tiles)
    {
        free(state);
        return nullptr;
    }

    state->activeTileCapacity = initialCapacity;
    state->activeTiles = (int32_t*)malloc(initialCapacity * sizeof(int32_t));

    state->hotspotTiles = (int32_t*)malloc(initialCapacity * sizeof(int32_t));
    state->superconductTiles = (int32_t*)malloc(initialCapacity * sizeof(int32_t));
    state->highPressureTiles = (int32_t*)malloc(initialCapacity * sizeof(int32_t));

    state->excitedGroupCapacity = 256;
    state->excitedGroups = (ExcitedGroupData*)calloc(256, sizeof(ExcitedGroupData));

    state->updateCounter = 1;
    state->equalizationQueueCycle = 0;

    return state;
}

ATMOS_API void atmos_destroy_grid(GridAtmosState* state)
{
    if (!state) return;

    if (state->tiles)
        free(state->tiles);

    if (state->activeTiles)
        free(state->activeTiles);

    if (state->hotspotTiles)
        free(state->hotspotTiles);

    if (state->superconductTiles)
        free(state->superconductTiles);

    if (state->highPressureTiles)
        free(state->highPressureTiles);

    if (state->excitedGroups)
    {
        for (int i = 0; i < state->excitedGroupCount; i++)
        {
            if (state->excitedGroups[i].tileIndices)
                free(state->excitedGroups[i].tileIndices);
        }
        free(state->excitedGroups);
    }

    free(state);
}

ATMOS_API void atmos_reset_grid(GridAtmosState* state)
{
    if (!state) return;

    state->tileCount = 0;
    state->activeTileCount = 0;
    state->hotspotTileCount = 0;
    state->superconductTileCount = 0;
    state->highPressureTileCount = 0;

    for (int i = 0; i < state->excitedGroupCount; i++)
    {
        if (state->excitedGroups[i].tileIndices)
        {
            free(state->excitedGroups[i].tileIndices);
            state->excitedGroups[i].tileIndices = nullptr;
        }
    }
    state->excitedGroupCount = 0;

    state->updateCounter = 1;
    state->equalizationQueueCycle = 0;
}

ATMOS_API int32_t atmos_add_tile(GridAtmosState* state, const TileAtmosData* tile)
{
    if (!state || !tile) return -1;

    ensure_tile_capacity(state, state->tileCount + 1);

    int32_t index = state->tileCount++;
    memcpy(&state->tiles[index], tile, sizeof(TileAtmosData));
    return index;
}

ATMOS_API void atmos_update_tile(GridAtmosState* state, int32_t index, const TileAtmosData* tile)
{
    if (!state || !tile || index < 0 || index >= state->tileCount) return;
    memcpy(&state->tiles[index], tile, sizeof(TileAtmosData));
}

ATMOS_API TileAtmosData* atmos_get_tile(GridAtmosState* state, int32_t index)
{
    if (!state || index < 0 || index >= state->tileCount) return nullptr;
    return &state->tiles[index];
}

ATMOS_API void atmos_set_adjacency(GridAtmosState* state, int32_t tileIndex, int32_t direction, int32_t adjacentIndex)
{
    if (!state || tileIndex < 0 || tileIndex >= state->tileCount) return;
    if (direction < 0 || direction >= ATMOS_DIRECTIONS) return;

    TileAtmosData* tile = &state->tiles[tileIndex];
    tile->adjacentIndices[direction] = adjacentIndex;

    if (adjacentIndex >= 0)
        tile->adjacentBits |= (1 << direction);
    else
        tile->adjacentBits &= ~(1 << direction);
}

ATMOS_API void atmos_add_active_tile(GridAtmosState* state, int32_t tileIndex)
{
    add_active_tile_impl(state, tileIndex);
}

ATMOS_API void atmos_remove_active_tile(GridAtmosState* state, int32_t tileIndex)
{
    remove_active_tile_impl(state, tileIndex, true);
}

ATMOS_API AtmosResult atmos_process(GridAtmosState* state, const AtmosConfig* config)
{
    AtmosResult result = {};
    if (!state || !config) return result;

    auto startTime = std::chrono::high_resolution_clock::now();

    state->updateCounter++;

    AtmosResult activeResult = atmos_process_active_tiles(state, config);
    result.tilesProcessed += activeResult.tilesProcessed;

    if (config->excitedGroupsEnabled)
    {
        AtmosResult groupResult = atmos_process_excited_groups(state, config);
        result.excitedGroupsCount = groupResult.excitedGroupsCount;
    }

    AtmosResult hotspotResult = atmos_process_hotspots(state, config);
    result.hotspotTilesCount = hotspotResult.hotspotTilesCount;

    if (config->superconductionEnabled)
    {
        AtmosResult superResult = atmos_process_superconductivity(state, config);
        result.superconductTilesCount = superResult.superconductTilesCount;
    }

    AtmosResult hpResult = atmos_process_high_pressure(state, config);
    result.maxPressureDelta = hpResult.maxPressureDelta;

    result.activeTilesCount = state->activeTileCount;
    result.processingComplete = 1;

    return result;
}

ATMOS_API AtmosResult atmos_process_revalidate(GridAtmosState* state, const AtmosConfig* config)
{
    AtmosResult result = {};
    if (!state || !config) return result;

    result.processingComplete = 1;
    return result;
}

ATMOS_API AtmosResult atmos_process_active_tiles(GridAtmosState* state, const AtmosConfig* config)
{
    AtmosResult result = {};
    if (!state || !config) return result;

    auto startTime = std::chrono::high_resolution_clock::now();
    int checkInterval = 30;

    for (int i = 0; i < state->tileCount; i++)
    {
        TileAtmosData* tile = &state->tiles[i];
        if (tile->flags & TILE_FLAG_IMMUTABLE)
            continue;

        archive_tile(tile);
    }

    int processed = 0;
    for (int i = 0; i < state->activeTileCount; i++)
    {
        int32_t tileIndex = state->activeTiles[i];
        if (tileIndex < 0 || tileIndex >= state->tileCount)
            continue;

        if (config->monstermosEnabled)
        {
            equalize_pressure_in_zone(state, tileIndex, config);
        }

        process_cell(state, tileIndex, config);
        processed++;

        if (processed % checkInterval == 0)
        {
            auto elapsed = std::chrono::high_resolution_clock::now() - startTime;
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
            if (us > config->maxProcessTimeMicroseconds)
            {
                result.processingComplete = 0;
                break;
            }
        }
    }

    result.tilesProcessed = processed;
    result.activeTilesCount = state->activeTileCount;
    result.processingComplete = 1;

    return result;
}

ATMOS_API AtmosResult atmos_process_excited_groups(GridAtmosState* state, const AtmosConfig* config)
{
    AtmosResult result = {};
    if (!state || !config) return result;

    for (int i = 0; i < state->excitedGroupCount; i++)
    {
        ExcitedGroupData* group = &state->excitedGroups[i];
        if (group->disposed) continue;

        group->breakdownCooldown++;
        group->dismantleCooldown++;

        if (group->breakdownCooldown > config->constants.excitedGroupBreakdownCycles)
        {
            excited_group_self_breakdown(state, i, config);
        }
        else if (group->dismantleCooldown > config->constants.excitedGroupsDismantleCycles)
        {
            deactivate_group_tiles(state, i);
        }
    }

    result.excitedGroupsCount = state->excitedGroupCount;
    result.processingComplete = 1;
    return result;
}

ATMOS_API AtmosResult atmos_process_hotspots(GridAtmosState* state, const AtmosConfig* config)
{
    AtmosResult result = {};
    if (!state || !config) return result;

    for (int i = 0; i < state->hotspotTileCount; i++)
    {
        int32_t tileIndex = state->hotspotTiles[i];
        process_hotspot(state, tileIndex, config);
    }

    result.hotspotTilesCount = state->hotspotTileCount;
    result.processingComplete = 1;
    return result;
}

ATMOS_API AtmosResult atmos_process_superconductivity(GridAtmosState* state, const AtmosConfig* config)
{
    AtmosResult result = {};
    if (!state || !config) return result;

    for (int i = 0; i < state->superconductTileCount; i++)
    {
        int32_t tileIndex = state->superconductTiles[i];
        superconduct(state, tileIndex, config);
    }

    result.superconductTilesCount = state->superconductTileCount;
    result.processingComplete = 1;
    return result;
}

ATMOS_API AtmosResult atmos_process_high_pressure(GridAtmosState* state, const AtmosConfig* config)
{
    AtmosResult result = {};
    if (!state || !config) return result;

    float maxDelta = 0.0f;
    for (int i = 0; i < state->highPressureTileCount; i++)
    {
        int32_t tileIndex = state->highPressureTiles[i];
        TileAtmosData* tile = &state->tiles[tileIndex];

        high_pressure_movements(state, tileIndex, config);

        if (tile->pressureDifference > maxDelta)
            maxDelta = tile->pressureDifference;

        tile->pressureDifference = 0.0f;
        tile->currentTransferDirection = -1;
    }

    state->highPressureTileCount = 0;
    result.maxPressureDelta = maxDelta;
    result.processingComplete = 1;
    return result;
}

ATMOS_API AtmosResult atmos_equalize_pressure_zone(GridAtmosState* state, int32_t startTile, const AtmosConfig* config)
{
    AtmosResult result = {};
    if (!state || !config) return result;

    equalize_pressure_in_zone(state, startTile, config);

    result.processingComplete = 1;
    return result;
}

ATMOS_API AtmosResult atmos_explosive_depressurize(GridAtmosState* state, int32_t startTile, const AtmosConfig* config)
{
    AtmosResult result = {};
    if (!state || !config) return result;

    explosive_depressurize(state, startTile, config);

    result.processingComplete = 1;
    return result;
}

ATMOS_API void atmos_ignite_hotspot(GridAtmosState* state, int32_t tileIndex, float temperature, float volume)
{
    if (!state || tileIndex < 0 || tileIndex >= state->tileCount) return;

    TileAtmosData* tile = &state->tiles[tileIndex];
    tile->hotspotTemperature = temperature;
    tile->hotspotVolume = volume;
    tile->flags |= TILE_FLAG_HOTSPOT;

    bool found = false;
    for (int i = 0; i < state->hotspotTileCount; i++)
    {
        if (state->hotspotTiles[i] == tileIndex)
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        state->hotspotTiles[state->hotspotTileCount++] = tileIndex;
    }
}

ATMOS_API void atmos_extinguish_hotspot(GridAtmosState* state, int32_t tileIndex)
{
    if (!state || tileIndex < 0 || tileIndex >= state->tileCount) return;

    TileAtmosData* tile = &state->tiles[tileIndex];
    tile->hotspotTemperature = 0.0f;
    tile->hotspotVolume = 0.0f;
    tile->hotspotState = 0;
    tile->flags &= ~TILE_FLAG_HOTSPOT;

    for (int i = 0; i < state->hotspotTileCount; i++)
    {
        if (state->hotspotTiles[i] == tileIndex)
        {
            state->hotspotTiles[i] = state->hotspotTiles[--state->hotspotTileCount];
            break;
        }
    }
}

ATMOS_API float atmos_get_heat_capacity(const TileAtmosData* tile, const float* specificHeats)
{
    if (!tile || !specificHeats) return 0.0f;
    return get_heat_capacity_impl(tile->moles, specificHeats, (tile->flags & TILE_FLAG_SPACE) != 0);
}

ATMOS_API float atmos_get_heat_capacity_archived(const TileAtmosData* tile, const float* specificHeats)
{
    if (!tile || !specificHeats) return 0.0f;
    return get_heat_capacity_archived_impl(tile, specificHeats);
}

ATMOS_API float atmos_get_thermal_energy(const TileAtmosData* tile, const float* specificHeats)
{
    if (!tile || !specificHeats) return 0.0f;
    return get_thermal_energy_impl(tile, specificHeats);
}

ATMOS_API void atmos_merge(TileAtmosData* receiver, const TileAtmosData* giver, const float* specificHeats)
{
    if (!receiver || !giver || !specificHeats) return;
    if (receiver->flags & TILE_FLAG_IMMUTABLE) return;

    merge_impl(receiver, giver->moles, giver->temperature, specificHeats, 0.01f, 0.0003f);
}

ATMOS_API void atmos_remove_gas(TileAtmosData* tile, float amount, TileAtmosData* removed)
{
    if (!tile) return;
    remove_gas_impl(tile, amount, removed);
}

ATMOS_API void atmos_remove_ratio(TileAtmosData* tile, float ratio, TileAtmosData* removed)
{
    if (!tile) return;
    remove_ratio_impl(tile, ratio, removed, 0.00000005f);
}

ATMOS_API int32_t atmos_react(TileAtmosData* tile, const AtmosConfig* config)
{
    if (!tile || !config) return 0;
    return react_impl(tile, config);
}

ATMOS_API void atmos_share(TileAtmosData* receiver, TileAtmosData* sharer, int32_t adjacentCount, const AtmosConfig* config)
{
    if (!receiver || !sharer || !config) return;
    share_impl(receiver, sharer, adjacentCount, config);
}

ATMOS_API float atmos_temperature_share(TileAtmosData* receiver, TileAtmosData* sharer, float conductionCoefficient, const AtmosConfig* config)
{
    if (!receiver || !sharer || !config) return 0.0f;
    return temperature_share_impl(receiver, sharer, conductionCoefficient, config);
}

ATMOS_API void atmos_config_init_default(AtmosConfig* config)
{
    if (!config) return;

    memset(config, 0, sizeof(AtmosConfig));

    config->gasSpecificHeats[GAS_OXYGEN] = 20.0f;
    config->gasSpecificHeats[GAS_NITROGEN] = 20.0f;
    config->gasSpecificHeats[GAS_CO2] = 30.0f;
    config->gasSpecificHeats[GAS_PLASMA] = 200.0f;
    config->gasSpecificHeats[GAS_TRITIUM] = 10.0f;
    config->gasSpecificHeats[GAS_WATER_VAPOR] = 40.0f;
    config->gasSpecificHeats[GAS_AMMONIA] = 20.0f;
    config->gasSpecificHeats[GAS_N2O] = 40.0f;
    config->gasSpecificHeats[GAS_FREZON] = 600.0f;

    atmos_constants_init_default(&config->constants);

    config->maxProcessTimeMicroseconds = 5000;
    config->speedup = 1.0f;
    config->heatScale = 1.0f;
    config->monstermosEnabled = 1;
    config->excitedGroupsEnabled = 1;
    config->superconductionEnabled = 1;
    config->spacingEnabled = 1;
    config->spacingEscapeRatio = 0.9f;
    config->spacingMinGas = 2.0f;
    config->spacingMaxWind = 500.0f;
}

ATMOS_API int32_t atmos_get_active_tile_count(const GridAtmosState* state)
{
    return state ? state->activeTileCount : 0;
}

ATMOS_API int32_t atmos_get_tile_count(const GridAtmosState* state)
{
    return state ? state->tileCount : 0;
}

ATMOS_API TileAtmosData* atmos_get_tiles_ptr(GridAtmosState* state)
{
    return state ? state->tiles : nullptr;
}

ATMOS_API void atmos_archive_tile(TileAtmosData* tile)
{
    if (tile)
        archive_tile(tile);
}

ATMOS_API void atmos_archive_all(GridAtmosState* state)
{
    if (!state) return;

    for (int i = 0; i < state->tileCount; i++)
    {
        archive_tile(&state->tiles[i]);
    }
}

}

void ensure_tile_capacity(GridAtmosState* state, int32_t needed)
{
    if (state->tileCapacity >= needed) return;

    int32_t newCapacity = state->tileCapacity * 2;
    while (newCapacity < needed) newCapacity *= 2;

    state->tiles = (TileAtmosData*)realloc(state->tiles, newCapacity * sizeof(TileAtmosData));
    memset(state->tiles + state->tileCount, 0, (newCapacity - state->tileCount) * sizeof(TileAtmosData));
    state->tileCapacity = newCapacity;
}

void ensure_active_capacity(GridAtmosState* state, int32_t needed)
{
    if (state->activeTileCapacity >= needed) return;

    int32_t newCapacity = state->activeTileCapacity * 2;
    while (newCapacity < needed) newCapacity *= 2;

    state->activeTiles = (int32_t*)realloc(state->activeTiles, newCapacity * sizeof(int32_t));
    state->activeTileCapacity = newCapacity;
}

void ensure_excited_group_capacity(GridAtmosState* state, int32_t needed)
{
    if (state->excitedGroupCapacity >= needed) return;

    int32_t newCapacity = state->excitedGroupCapacity * 2;
    while (newCapacity < needed) newCapacity *= 2;

    state->excitedGroups = (ExcitedGroupData*)realloc(state->excitedGroups, newCapacity * sizeof(ExcitedGroupData));
    memset(state->excitedGroups + state->excitedGroupCapacity, 0, (newCapacity - state->excitedGroupCapacity) * sizeof(ExcitedGroupData));
    state->excitedGroupCapacity = newCapacity;
}