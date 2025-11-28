#pragma once

#include "atmos_types.h"
#include "atmos_simd.h"

#ifdef ATMOS_EXPORT
    #ifdef _WIN32
        #define ATMOS_INTERNAL __declspec(dllexport)
    #else
        #define ATMOS_INTERNAL __attribute__((visibility("default")))
    #endif
#else
    #ifdef _WIN32
        #define ATMOS_INTERNAL __declspec(dllimport)
    #else
        #define ATMOS_INTERNAL
    #endif
#endif

ATMOS_INTERNAL float get_heat_capacity_impl(const float* moles, const float* specificHeats, bool space);
ATMOS_INTERNAL float get_heat_capacity_archived_impl(const TileAtmosData* tile, const float* specificHeats);
ATMOS_INTERNAL float get_thermal_energy_impl(const TileAtmosData* tile, const float* specificHeats);

ATMOS_INTERNAL void archive_tile(TileAtmosData* tile);
ATMOS_INTERNAL int compare_exchange(const TileAtmosData* a, const TileAtmosData* b, const AtmosConfig* config);

ATMOS_INTERNAL void share_impl(TileAtmosData* receiver, TileAtmosData* sharer, int adjacentCount, const AtmosConfig* config);
ATMOS_INTERNAL float temperature_share_impl(TileAtmosData* receiver, TileAtmosData* sharer, float conductionCoefficient, const AtmosConfig* config);
ATMOS_INTERNAL float temperature_share_solid(TileAtmosData* receiver, float conductionCoefficient, float sharerTemp, float sharerHeatCapacity, const AtmosConfig* config);

ATMOS_INTERNAL void process_cell(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config);
ATMOS_INTERNAL void last_share_check(GridAtmosState* state, TileAtmosData* tile, const AtmosConfig* config);

ATMOS_INTERNAL void add_active_tile_impl(GridAtmosState* state, int32_t tileIndex);
ATMOS_INTERNAL void remove_active_tile_impl(GridAtmosState* state, int32_t tileIndex, bool disposeGroup);

ATMOS_INTERNAL int32_t create_excited_group(GridAtmosState* state);
ATMOS_INTERNAL void add_tile_to_excited_group(GridAtmosState* state, int32_t groupId, int32_t tileIndex);
ATMOS_INTERNAL void remove_tile_from_excited_group(GridAtmosState* state, int32_t groupId, int32_t tileIndex);
ATMOS_INTERNAL void merge_excited_groups(GridAtmosState* state, int32_t group1, int32_t group2);
ATMOS_INTERNAL void dispose_excited_group(GridAtmosState* state, int32_t groupId);
ATMOS_INTERNAL void reset_excited_group_cooldowns(GridAtmosState* state, int32_t groupId);
ATMOS_INTERNAL void excited_group_self_breakdown(GridAtmosState* state, int32_t groupId, const AtmosConfig* config);
ATMOS_INTERNAL void deactivate_group_tiles(GridAtmosState* state, int32_t groupId);

ATMOS_INTERNAL void equalize_pressure_in_zone(GridAtmosState* state, int32_t startTile, const AtmosConfig* config);
ATMOS_INTERNAL void explosive_depressurize(GridAtmosState* state, int32_t startTile, const AtmosConfig* config);
ATMOS_INTERNAL void adjust_eq_movement(TileAtmosData* tile, TileAtmosData* adj, int direction, float amount);
ATMOS_INTERNAL void finalize_eq(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config);

ATMOS_INTERNAL void process_hotspot(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config);
ATMOS_INTERNAL void ignite_hotspot_impl(GridAtmosState* state, int32_t tileIndex, float temperature, float volume, const AtmosConfig* config);
ATMOS_INTERNAL void expose_hotspot(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config);
ATMOS_INTERNAL void perform_hotspot_fire(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config);

ATMOS_INTERNAL void superconduct(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config);
ATMOS_INTERNAL bool consider_superconductivity(GridAtmosState* state, int32_t tileIndex, bool starting, const AtmosConfig* config);
ATMOS_INTERNAL void neighbor_conduct_with_source(GridAtmosState* state, int32_t tileIndex, int32_t sourceIndex, const AtmosConfig* config);
ATMOS_INTERNAL void radiate_to_space(TileAtmosData* tile, const AtmosConfig* config);
ATMOS_INTERNAL void finish_superconductivity(GridAtmosState* state, int32_t tileIndex, float temperature, const AtmosConfig* config);

ATMOS_INTERNAL int react_impl(TileAtmosData* tile, const AtmosConfig* config);
ATMOS_INTERNAL int plasma_fire_reaction(TileAtmosData* tile, const AtmosConfig* config);
ATMOS_INTERNAL int tritium_fire_reaction(TileAtmosData* tile, const AtmosConfig* config);
ATMOS_INTERNAL int frezon_coolant_reaction(TileAtmosData* tile, const AtmosConfig* config);
ATMOS_INTERNAL int frezon_production_reaction(TileAtmosData* tile, const AtmosConfig* config);
ATMOS_INTERNAL int water_vapor_reaction(TileAtmosData* tile, const AtmosConfig* config);
ATMOS_INTERNAL int n2o_decomposition_reaction(TileAtmosData* tile, const AtmosConfig* config);
ATMOS_INTERNAL int ammonia_oxygen_reaction(TileAtmosData* tile, const AtmosConfig* config);

ATMOS_INTERNAL void merge_impl(TileAtmosData* receiver, const float* giverMoles, float giverTemp, const float* specificHeats, float minTempDelta, float minHeatCapacity);
ATMOS_INTERNAL void remove_gas_impl(TileAtmosData* tile, float amount, TileAtmosData* removed);
ATMOS_INTERNAL void remove_ratio_impl(TileAtmosData* tile, float ratio, TileAtmosData* removed, float gasMinMoles);

ATMOS_INTERNAL void high_pressure_movements(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config);
ATMOS_INTERNAL void consider_pressure_difference(GridAtmosState* state, int32_t tileIndex, int direction, float pressureDiff);

ATMOS_INTERNAL void ensure_tile_capacity(GridAtmosState* state, int32_t needed);
ATMOS_INTERNAL void ensure_active_capacity(GridAtmosState* state, int32_t needed);
ATMOS_INTERNAL void ensure_excited_group_capacity(GridAtmosState* state, int32_t needed);