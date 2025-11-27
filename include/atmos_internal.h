#pragma once

#include "atmos_types.h"
#include "atmos_simd.h"

float get_heat_capacity_impl(const float* moles, const float* specificHeats, bool space);
float get_heat_capacity_archived_impl(const TileAtmosData* tile, const float* specificHeats);
float get_thermal_energy_impl(const TileAtmosData* tile, const float* specificHeats);

void archive_tile(TileAtmosData* tile);
int compare_exchange(const TileAtmosData* a, const TileAtmosData* b, const AtmosConfig* config);

void share_impl(TileAtmosData* receiver, TileAtmosData* sharer, int adjacentCount, const AtmosConfig* config);
float temperature_share_impl(TileAtmosData* receiver, TileAtmosData* sharer, float conductionCoefficient, const AtmosConfig* config);
float temperature_share_solid(TileAtmosData* receiver, float conductionCoefficient, float sharerTemp, float sharerHeatCapacity, const AtmosConfig* config);

void process_cell(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config);
void last_share_check(GridAtmosState* state, TileAtmosData* tile, const AtmosConfig* config);

void add_active_tile_impl(GridAtmosState* state, int32_t tileIndex);
void remove_active_tile_impl(GridAtmosState* state, int32_t tileIndex, bool disposeGroup);

int32_t create_excited_group(GridAtmosState* state);
void add_tile_to_excited_group(GridAtmosState* state, int32_t groupId, int32_t tileIndex);
void remove_tile_from_excited_group(GridAtmosState* state, int32_t groupId, int32_t tileIndex);
void merge_excited_groups(GridAtmosState* state, int32_t group1, int32_t group2);
void dispose_excited_group(GridAtmosState* state, int32_t groupId);
void reset_excited_group_cooldowns(GridAtmosState* state, int32_t groupId);
void excited_group_self_breakdown(GridAtmosState* state, int32_t groupId, const AtmosConfig* config);
void deactivate_group_tiles(GridAtmosState* state, int32_t groupId);

void equalize_pressure_in_zone(GridAtmosState* state, int32_t startTile, const AtmosConfig* config);
void explosive_depressurize(GridAtmosState* state, int32_t startTile, const AtmosConfig* config);
void adjust_eq_movement(TileAtmosData* tile, TileAtmosData* adj, int direction, float amount);
void finalize_eq(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config);

void process_hotspot(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config);
void ignite_hotspot_impl(GridAtmosState* state, int32_t tileIndex, float temperature, float volume, const AtmosConfig* config);
void expose_hotspot(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config);
void perform_hotspot_fire(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config);

void superconduct(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config);
bool consider_superconductivity(GridAtmosState* state, int32_t tileIndex, bool starting, const AtmosConfig* config);
void neighbor_conduct_with_source(GridAtmosState* state, int32_t tileIndex, int32_t sourceIndex, const AtmosConfig* config);
void radiate_to_space(TileAtmosData* tile, const AtmosConfig* config);
void finish_superconductivity(GridAtmosState* state, int32_t tileIndex, float temperature, const AtmosConfig* config);

int react_impl(TileAtmosData* tile, const AtmosConfig* config);
int plasma_fire_reaction(TileAtmosData* tile, const AtmosConfig* config);
int tritium_fire_reaction(TileAtmosData* tile, const AtmosConfig* config);
int frezon_coolant_reaction(TileAtmosData* tile, const AtmosConfig* config);
int frezon_production_reaction(TileAtmosData* tile, const AtmosConfig* config);
int water_vapor_reaction(TileAtmosData* tile, const AtmosConfig* config);
int n2o_decomposition_reaction(TileAtmosData* tile, const AtmosConfig* config);
int ammonia_oxygen_reaction(TileAtmosData* tile, const AtmosConfig* config);

void merge_impl(TileAtmosData* receiver, const float* giverMoles, float giverTemp, const float* specificHeats, float minTempDelta, float minHeatCapacity);
void remove_gas_impl(TileAtmosData* tile, float amount, TileAtmosData* removed);
void remove_ratio_impl(TileAtmosData* tile, float ratio, TileAtmosData* removed, float gasMinMoles);

void high_pressure_movements(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config);
void consider_pressure_difference(GridAtmosState* state, int32_t tileIndex, int direction, float pressureDiff);

void ensure_tile_capacity(GridAtmosState* state, int32_t needed);
void ensure_active_capacity(GridAtmosState* state, int32_t needed);
void ensure_excited_group_capacity(GridAtmosState* state, int32_t needed);