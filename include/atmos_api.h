#pragma once

#include "atmos_types.h"

#ifdef _WIN32
    #ifdef ATMOS_EXPORT
        #define ATMOS_API __declspec(dllexport)
    #else
        #define ATMOS_API __declspec(dllimport)
    #endif
#else
    #define ATMOS_API __attribute__((visibility("default")))
#endif

extern "C"
{

ATMOS_API uint32_t atmos_get_version();
ATMOS_API uint32_t atmos_get_simd_level();

ATMOS_API GridAtmosState* atmos_create_grid(int32_t initialCapacity);
ATMOS_API void atmos_destroy_grid(GridAtmosState* state);
ATMOS_API void atmos_reset_grid(GridAtmosState* state);

ATMOS_API int32_t atmos_add_tile(GridAtmosState* state, const TileAtmosData* tile);
ATMOS_API void atmos_update_tile(GridAtmosState* state, int32_t index, const TileAtmosData* tile);
ATMOS_API TileAtmosData* atmos_get_tile(GridAtmosState* state, int32_t index);
ATMOS_API void atmos_set_adjacency(GridAtmosState* state, int32_t tileIndex, int32_t direction, int32_t adjacentIndex);

ATMOS_API void atmos_add_active_tile(GridAtmosState* state, int32_t tileIndex);
ATMOS_API void atmos_remove_active_tile(GridAtmosState* state, int32_t tileIndex);

ATMOS_API AtmosResult atmos_process(GridAtmosState* state, const AtmosConfig* config);

ATMOS_API AtmosResult atmos_process_revalidate(GridAtmosState* state, const AtmosConfig* config);
ATMOS_API AtmosResult atmos_process_active_tiles(GridAtmosState* state, const AtmosConfig* config);
ATMOS_API AtmosResult atmos_process_excited_groups(GridAtmosState* state, const AtmosConfig* config);
ATMOS_API AtmosResult atmos_process_hotspots(GridAtmosState* state, const AtmosConfig* config);
ATMOS_API AtmosResult atmos_process_superconductivity(GridAtmosState* state, const AtmosConfig* config);
ATMOS_API AtmosResult atmos_process_high_pressure(GridAtmosState* state, const AtmosConfig* config);

ATMOS_API AtmosResult atmos_equalize_pressure_zone(GridAtmosState* state, int32_t startTile, const AtmosConfig* config);
ATMOS_API AtmosResult atmos_explosive_depressurize(GridAtmosState* state, int32_t startTile, const AtmosConfig* config);

ATMOS_API void atmos_ignite_hotspot(GridAtmosState* state, int32_t tileIndex, float temperature, float volume);
ATMOS_API void atmos_extinguish_hotspot(GridAtmosState* state, int32_t tileIndex);

ATMOS_API float atmos_get_heat_capacity(const TileAtmosData* tile, const float* specificHeats);
ATMOS_API float atmos_get_heat_capacity_archived(const TileAtmosData* tile, const float* specificHeats);
ATMOS_API float atmos_get_thermal_energy(const TileAtmosData* tile, const float* specificHeats);

ATMOS_API void atmos_merge(TileAtmosData* receiver, const TileAtmosData* giver, const float* specificHeats);
ATMOS_API void atmos_remove_gas(TileAtmosData* tile, float amount, TileAtmosData* removed);
ATMOS_API void atmos_remove_ratio(TileAtmosData* tile, float ratio, TileAtmosData* removed);

ATMOS_API int32_t atmos_react(TileAtmosData* tile, const AtmosConfig* config);

ATMOS_API void atmos_share(TileAtmosData* receiver, TileAtmosData* sharer, int32_t adjacentCount, const AtmosConfig* config);
ATMOS_API float atmos_temperature_share(TileAtmosData* receiver, TileAtmosData* sharer, float conductionCoefficient, const AtmosConfig* config);

ATMOS_API void atmos_config_init_default(AtmosConfig* config);

ATMOS_API int32_t atmos_get_active_tile_count(const GridAtmosState* state);
ATMOS_API int32_t atmos_get_tile_count(const GridAtmosState* state);
ATMOS_API TileAtmosData* atmos_get_tiles_ptr(GridAtmosState* state);

ATMOS_API void atmos_archive_tile(TileAtmosData* tile);
ATMOS_API void atmos_archive_all(GridAtmosState* state);

}