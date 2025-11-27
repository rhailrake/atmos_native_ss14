#include "atmos_internal.h"

static void extinguish_hotspot_impl(GridAtmosState* state, int32_t tileIndex)
{
    if (!state || tileIndex < 0 || tileIndex >= state->tileCount)
        return;

    TileAtmosData* tile = &state->tiles[tileIndex];
    tile->flags &= ~TILE_FLAG_HOTSPOT;
    tile->hotspotTemperature = 0;
    tile->hotspotVolume = 0;
    tile->hotspotState = 0;

    for (int i = 0; i < state->hotspotTileCount; i++)
    {
        if (state->hotspotTiles[i] == tileIndex)
        {
            state->hotspotTiles[i] = state->hotspotTiles[--state->hotspotTileCount];
            break;
        }
    }
}

void process_hotspot(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config)
{
    if (!state || !config || tileIndex < 0 || tileIndex >= state->tileCount)
        return;

    TileAtmosData* tile = &state->tiles[tileIndex];

    if (!(tile->flags & TILE_FLAG_HOTSPOT))
        return;

    if (tile->hotspotTemperature < config->constants.fireMinimumTemperatureToExist)
    {
        extinguish_hotspot_impl(state, tileIndex);
        return;
    }

    if (tile->hotspotVolume > 1.0f)
        tile->hotspotVolume -= 1.0f;

    perform_hotspot_fire(state, tileIndex, config);
    expose_hotspot(state, tileIndex, config);

    if (tile->hotspotTemperature < config->constants.fireMinimumTemperatureToExist)
    {
        extinguish_hotspot_impl(state, tileIndex);
        return;
    }

    if (tile->hotspotTemperature > config->constants.fireMinimumTemperatureToSpread)
    {
        for (int i = 0; i < ATMOS_DIRECTIONS; i++)
        {
            if (!(tile->adjacentBits & (1 << i)))
                continue;

            int32_t adjIdx = tile->adjacentIndices[i];
            if (adjIdx < 0 || adjIdx >= state->tileCount)
                continue;

            TileAtmosData* adj = &state->tiles[adjIdx];
            if (adj->flags & TILE_FLAG_HOTSPOT)
                continue;
            if (adj->flags & TILE_FLAG_SPACE)
                continue;

            float plasma = adj->moles[GAS_PLASMA];
            float tritium = adj->moles[GAS_TRITIUM];
            float oxygen = adj->moles[GAS_OXYGEN];

            if ((plasma > 0.5f || tritium > 0.5f) && oxygen > 0.5f)
            {
                float spreadTemp = tile->hotspotTemperature * config->constants.fireSpreadRadiosityScale;
                if (spreadTemp > config->constants.fireMinimumTemperatureToExist)
                {
                    ignite_hotspot_impl(state, adjIdx, spreadTemp, 1.0f, config);
                }
            }
        }
    }

    uint8_t newState = 0;
    if (tile->hotspotTemperature > config->constants.plasmaUpperTemperature)
        newState = 3;
    else if (tile->hotspotTemperature > config->constants.plasmaMinimumBurnTemperature + 500.0f)
        newState = 2;
    else if (tile->hotspotTemperature > config->constants.plasmaMinimumBurnTemperature)
        newState = 1;

    tile->hotspotState = newState;
}

void ignite_hotspot_impl(GridAtmosState* state, int32_t tileIndex, float temperature, float volume, const AtmosConfig* config)
{
    if (!state || !config || tileIndex < 0 || tileIndex >= state->tileCount)
        return;

    TileAtmosData* tile = &state->tiles[tileIndex];

    if (tile->flags & TILE_FLAG_SPACE)
        return;

    float plasma = tile->moles[GAS_PLASMA];
    float tritium = tile->moles[GAS_TRITIUM];
    float oxygen = tile->moles[GAS_OXYGEN];

    if (plasma < 0.5f && tritium < 0.5f)
        return;

    if (oxygen < 0.5f)
        return;

    if (temperature < config->constants.plasmaMinimumBurnTemperature)
        return;

    if (tile->flags & TILE_FLAG_HOTSPOT)
    {
        if (temperature > tile->hotspotTemperature)
            tile->hotspotTemperature = temperature;
        if (volume > tile->hotspotVolume)
            tile->hotspotVolume = volume;
        return;
    }

    tile->hotspotTemperature = temperature;
    tile->hotspotVolume = volume;
    tile->flags |= TILE_FLAG_HOTSPOT;
    tile->hotspotState = 1;

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

    add_active_tile_impl(state, tileIndex);
}

void expose_hotspot(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config)
{
    if (!state || !config || tileIndex < 0 || tileIndex >= state->tileCount)
        return;

    TileAtmosData* tile = &state->tiles[tileIndex];

    if (!(tile->flags & TILE_FLAG_HOTSPOT))
        return;

    float heatCapacity = get_heat_capacity_impl(tile->moles, config->gasSpecificHeats, false);
    if (heatCapacity <= config->constants.minimumHeatCapacity)
        return;

    float energyReleased = tile->hotspotTemperature * tile->hotspotVolume * 0.5f;

    if (!(tile->flags & TILE_FLAG_IMMUTABLE))
    {
        tile->temperature += energyReleased / heatCapacity;
        tile->temperature = simd_clamp(tile->temperature, config->constants.TCMB, config->constants.Tmax);
    }
}

void perform_hotspot_fire(GridAtmosState* state, int32_t tileIndex, const AtmosConfig* config)
{
    if (!state || !config || tileIndex < 0 || tileIndex >= state->tileCount)
        return;

    TileAtmosData* tile = &state->tiles[tileIndex];

    if (tile->flags & TILE_FLAG_IMMUTABLE)
        return;

    float plasma = tile->moles[GAS_PLASMA];
    float oxygen = tile->moles[GAS_OXYGEN];
    float tritium = tile->moles[GAS_TRITIUM];

    float temperature = tile->hotspotTemperature;
    float energyReleased = 0.0f;
    float consumedFuel = 0.0f;
    float consumedOxygen = 0.0f;

    if (plasma > 0.5f && oxygen > 0.5f && temperature >= config->constants.plasmaMinimumBurnTemperature)
    {
        float temperatureScale = 1.0f;
        if (temperature > config->constants.plasmaUpperTemperature)
            temperatureScale = 1.0f;
        else
            temperatureScale = (temperature - config->constants.plasmaMinimumBurnTemperature) /
                              (config->constants.plasmaUpperTemperature - config->constants.plasmaMinimumBurnTemperature);

        if (temperatureScale > 0.0f)
        {
            float oxygenBurnRate = config->constants.oxygenBurnRateBase - temperatureScale;
            float plasmaBurnRate = temperatureScale * (oxygen > plasma * config->constants.plasmaOxygenFullburn
                ? 1.0f
                : oxygen / (plasma * config->constants.plasmaOxygenFullburn));

            plasmaBurnRate = simd_min(plasmaBurnRate, plasma);
            plasmaBurnRate = simd_min(plasmaBurnRate, oxygen / oxygenBurnRate);

            if (plasmaBurnRate > config->constants.gasMinMoles)
            {
                float burnedPlasma = plasmaBurnRate;
                float burnedOxygen = plasmaBurnRate * oxygenBurnRate;

                tile->moles[GAS_PLASMA] -= burnedPlasma;
                tile->moles[GAS_OXYGEN] -= burnedOxygen;
                tile->moles[GAS_CO2] += burnedPlasma * 0.75f;
                tile->moles[GAS_WATER_VAPOR] += burnedPlasma * 0.25f;

                energyReleased += config->constants.firePlasmaEnergyReleased * burnedPlasma;
                consumedFuel += burnedPlasma;
                consumedOxygen += burnedOxygen;
            }
        }
    }

    if (tritium > 0.5f && oxygen > 0.5f && temperature >= config->constants.plasmaMinimumBurnTemperature)
    {
        float burnedTritium = simd_min(tritium, oxygen / config->constants.tritiumBurnOxyFactor);
        burnedTritium = simd_min(burnedTritium, config->constants.tritiumBurnTritFactor);

        if (burnedTritium > config->constants.gasMinMoles)
        {
            float burnedOxygen = burnedTritium * config->constants.tritiumBurnOxyFactor;

            tile->moles[GAS_TRITIUM] -= burnedTritium;
            tile->moles[GAS_OXYGEN] -= burnedOxygen;
            tile->moles[GAS_WATER_VAPOR] += burnedTritium;

            energyReleased += config->constants.fireHydrogenEnergyReleased * burnedTritium;
            consumedFuel += burnedTritium;
            consumedOxygen += burnedOxygen;
        }
    }

    if (energyReleased > 0.0f)
    {
        float heatCapacity = get_heat_capacity_impl(tile->moles, config->gasSpecificHeats, false);
        if (heatCapacity > config->constants.minimumHeatCapacity)
        {
            tile->hotspotTemperature += energyReleased / heatCapacity / config->heatScale;
            tile->hotspotTemperature = simd_clamp(tile->hotspotTemperature, config->constants.TCMB, config->constants.Tmax);
        }
    }

    if (consumedFuel < 0.5f)
    {
        tile->hotspotTemperature -= 5.0f;
    }
}