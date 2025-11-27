#include "atmos_internal.h"

#define REACTION_NONE      0
#define REACTION_REACTING  1
#define REACTION_STOP      2

int plasma_fire_reaction(TileAtmosData* tile, const AtmosConfig* config)
{
    if (!tile || !config)
        return REACTION_NONE;

    float oxygen = tile->moles[GAS_OXYGEN];
    float plasma = tile->moles[GAS_PLASMA];

    if (plasma < 0.5f || oxygen < 0.5f)
        return REACTION_NONE;

    if (tile->temperature < config->constants.plasmaMinimumBurnTemperature)
        return REACTION_NONE;

    float temperatureScale;
    if (tile->temperature > config->constants.plasmaUpperTemperature)
        temperatureScale = 1.0f;
    else
        temperatureScale = (tile->temperature - config->constants.plasmaMinimumBurnTemperature) /
                          (config->constants.plasmaUpperTemperature - config->constants.plasmaMinimumBurnTemperature);

    if (temperatureScale <= 0.0f)
        return REACTION_NONE;

    float oxygenBurnRate = config->constants.oxygenBurnRateBase - temperatureScale;
    float plasmaBurnRate;

    if (oxygen > plasma * config->constants.plasmaOxygenFullburn)
        plasmaBurnRate = plasma * temperatureScale / config->constants.plasmaBurnRateDelta;
    else
        plasmaBurnRate = (temperatureScale * (oxygen / plasma)) / (config->constants.plasmaBurnRateDelta * config->constants.plasmaOxygenFullburn);

    if (plasmaBurnRate > config->constants.gasMinMoles)
    {
        plasmaBurnRate = simd_min(plasmaBurnRate, plasma);
        plasmaBurnRate = simd_min(plasmaBurnRate, oxygen / oxygenBurnRate);
        plasma -= plasmaBurnRate;
        oxygen -= plasmaBurnRate * oxygenBurnRate;

        float heatCapacity = get_heat_capacity_impl(tile->moles, config->gasSpecificHeats, false);

        if (!(tile->flags & TILE_FLAG_IMMUTABLE))
        {
            tile->moles[GAS_PLASMA] = plasma;
            tile->moles[GAS_OXYGEN] = oxygen;
            tile->moles[GAS_CO2] += plasmaBurnRate * 0.75f;
            tile->moles[GAS_WATER_VAPOR] += plasmaBurnRate * 0.25f;

            float energyReleased = config->constants.firePlasmaEnergyReleased * plasmaBurnRate;
            if (heatCapacity > config->constants.minimumHeatCapacity)
            {
                tile->temperature += energyReleased / heatCapacity / config->heatScale;
                tile->temperature = simd_clamp(tile->temperature, config->constants.TCMB, config->constants.Tmax);
            }
        }

        return REACTION_REACTING;
    }

    return REACTION_NONE;
}

int tritium_fire_reaction(TileAtmosData* tile, const AtmosConfig* config)
{
    if (!tile || !config)
        return REACTION_NONE;

    float oxygen = tile->moles[GAS_OXYGEN];
    float tritium = tile->moles[GAS_TRITIUM];

    if (tritium < 0.5f || oxygen < 0.5f)
        return REACTION_NONE;

    if (tile->temperature < config->constants.plasmaMinimumBurnTemperature)
        return REACTION_NONE;

    float burnedTritium = simd_min(tritium, oxygen / config->constants.tritiumBurnOxyFactor);
    if (burnedTritium < config->constants.gasMinMoles)
        return REACTION_NONE;

    float energyReleased = config->constants.fireHydrogenEnergyReleased * burnedTritium;
    float heatCapacity = get_heat_capacity_impl(tile->moles, config->gasSpecificHeats, false);

    if (burnedTritium * config->constants.tritiumBurnOxyFactor > oxygen)
    {
        burnedTritium = oxygen / config->constants.tritiumBurnOxyFactor;
    }

    if (!(tile->flags & TILE_FLAG_IMMUTABLE))
    {
        tile->moles[GAS_TRITIUM] -= burnedTritium;
        tile->moles[GAS_OXYGEN] -= burnedTritium * config->constants.tritiumBurnOxyFactor;
        tile->moles[GAS_WATER_VAPOR] += burnedTritium;

        if (heatCapacity > config->constants.minimumHeatCapacity)
        {
            tile->temperature += energyReleased / heatCapacity / config->heatScale;
            tile->temperature = simd_clamp(tile->temperature, config->constants.TCMB, config->constants.Tmax);
        }
    }

    return REACTION_REACTING;
}

int frezon_coolant_reaction(TileAtmosData* tile, const AtmosConfig* config)
{
    if (!tile || !config)
        return REACTION_NONE;

    float frezon = tile->moles[GAS_FREZON];
    float nitrogen = tile->moles[GAS_NITROGEN];

    if (frezon < 0.5f || nitrogen < 0.5f)
        return REACTION_NONE;

    if (tile->temperature < config->constants.frezonCoolLowerTemperature)
        return REACTION_NONE;

    float temperatureScale;
    if (tile->temperature < config->constants.frezonCoolMidTemperature)
        temperatureScale = (tile->temperature - config->constants.frezonCoolLowerTemperature) /
                          (config->constants.frezonCoolMidTemperature - config->constants.frezonCoolLowerTemperature);
    else
        temperatureScale = 1.0f + (tile->temperature - config->constants.frezonCoolMidTemperature) / config->constants.frezonCoolMidTemperature;

    temperatureScale = simd_clamp(temperatureScale, 0.0f, config->constants.frezonCoolMaximumEnergyModifier);

    float heatCapacity = get_heat_capacity_impl(tile->moles, config->gasSpecificHeats, false);
    if (heatCapacity < config->constants.minimumHeatCapacity)
        return REACTION_NONE;

    float coolingRate = frezon * temperatureScale * config->constants.frezonCoolRateModifier;
    float nitrogenConsumed = simd_min(nitrogen, frezon * config->constants.frezonNitrogenCoolRatio);

    if (coolingRate < config->constants.gasMinMoles)
        return REACTION_NONE;

    float energyReleased = config->constants.frezonCoolEnergyReleased * coolingRate;

    if (!(tile->flags & TILE_FLAG_IMMUTABLE))
    {
        tile->moles[GAS_NITROGEN] -= nitrogenConsumed;
        tile->temperature += energyReleased / heatCapacity / config->heatScale;
        tile->temperature = simd_max(tile->temperature, config->constants.TCMB);
    }

    return REACTION_REACTING;
}

int frezon_production_reaction(TileAtmosData* tile, const AtmosConfig* config)
{
    if (!tile || !config)
        return REACTION_NONE;

    float oxygen = tile->moles[GAS_OXYGEN];
    float tritium = tile->moles[GAS_TRITIUM];
    float nitrogen = tile->moles[GAS_NITROGEN];

    if (tritium < 0.5f || oxygen < 0.5f || nitrogen < 0.5f)
        return REACTION_NONE;

    if (tile->temperature > config->constants.frezonCoolMidTemperature)
        return REACTION_NONE;

    float efficiency;
    if (tile->temperature < 73.15f)
        efficiency = 1.0f;
    else
        efficiency = 1.0f - (tile->temperature - 73.15f) / (config->constants.frezonCoolMidTemperature - 73.15f);

    if (efficiency <= 0.0f)
        return REACTION_NONE;

    float tritiumUsed = simd_min(tritium, oxygen / 50.0f);
    tritiumUsed = simd_min(tritiumUsed, nitrogen * 10.0f);
    float frezonProduced = tritiumUsed * efficiency / 50.0f;

    if (frezonProduced < config->constants.gasMinMoles)
        return REACTION_NONE;

    if (!(tile->flags & TILE_FLAG_IMMUTABLE))
    {
        tile->moles[GAS_TRITIUM] -= tritiumUsed;
        tile->moles[GAS_OXYGEN] -= tritiumUsed * 50.0f;
        tile->moles[GAS_NITROGEN] -= tritiumUsed / 10.0f;
        tile->moles[GAS_FREZON] += frezonProduced;
    }

    return REACTION_REACTING;
}

int water_vapor_reaction(TileAtmosData* tile, const AtmosConfig* config)
{
    if (!tile || !config)
        return REACTION_NONE;

    float water = tile->moles[GAS_WATER_VAPOR];
    if (water < 0.5f)
        return REACTION_NONE;

    if (tile->temperature > config->constants.T0C + 100.0f)
        return REACTION_NONE;

    float condensed = water * 0.05f;
    if (condensed < config->constants.gasMinMoles)
        return REACTION_NONE;

    if (!(tile->flags & TILE_FLAG_IMMUTABLE))
    {
        tile->moles[GAS_WATER_VAPOR] -= condensed;
    }

    return REACTION_REACTING;
}

int n2o_decomposition_reaction(TileAtmosData* tile, const AtmosConfig* config)
{
    if (!tile || !config)
        return REACTION_NONE;

    float n2o = tile->moles[GAS_N2O];
    if (n2o < 0.5f)
        return REACTION_NONE;

    if (tile->temperature < config->constants.T0C + 250.0f)
        return REACTION_NONE;

    float reacted = n2o / 2.0f;
    if (reacted < config->constants.gasMinMoles)
        return REACTION_NONE;

    float energyReleased = reacted * 20000.0f;
    float heatCapacity = get_heat_capacity_impl(tile->moles, config->gasSpecificHeats, false);

    if (!(tile->flags & TILE_FLAG_IMMUTABLE))
    {
        tile->moles[GAS_N2O] -= reacted;
        tile->moles[GAS_NITROGEN] += reacted;
        tile->moles[GAS_OXYGEN] += reacted * 0.5f;

        if (heatCapacity > config->constants.minimumHeatCapacity)
        {
            tile->temperature += energyReleased / heatCapacity / config->heatScale;
            tile->temperature = simd_clamp(tile->temperature, config->constants.TCMB, config->constants.Tmax);
        }
    }

    return REACTION_REACTING;
}

int ammonia_oxygen_reaction(TileAtmosData* tile, const AtmosConfig* config)
{
    if (!tile || !config)
        return REACTION_NONE;

    float ammonia = tile->moles[GAS_AMMONIA];
    float oxygen = tile->moles[GAS_OXYGEN];

    if (ammonia < 0.5f || oxygen < 0.5f)
        return REACTION_NONE;

    if (tile->temperature < config->constants.T0C + 100.0f)
        return REACTION_NONE;

    float ammoniaUsed = simd_min(ammonia, oxygen / 0.75f) / 10.0f;
    if (ammoniaUsed < config->constants.gasMinMoles)
        return REACTION_NONE;

    if (!(tile->flags & TILE_FLAG_IMMUTABLE))
    {
        tile->moles[GAS_AMMONIA] -= ammoniaUsed;
        tile->moles[GAS_OXYGEN] -= ammoniaUsed * 0.75f;
        tile->moles[GAS_NITROGEN] += ammoniaUsed * 0.5f;
        tile->moles[GAS_WATER_VAPOR] += ammoniaUsed * 1.5f;
    }

    return REACTION_REACTING;
}

int react_impl(TileAtmosData* tile, const AtmosConfig* config)
{
    if (!tile || !config)
        return REACTION_NONE;

    if (tile->flags & TILE_FLAG_IMMUTABLE)
        return REACTION_NONE;

    int result = REACTION_NONE;
    float energy = get_thermal_energy_impl(tile, config->gasSpecificHeats);

    if (energy < 1000.0f)
        return REACTION_NONE;

    int r = plasma_fire_reaction(tile, config);
    if (r == REACTION_STOP) return r;
    if (r != REACTION_NONE) result = r;

    r = tritium_fire_reaction(tile, config);
    if (r == REACTION_STOP) return r;
    if (r != REACTION_NONE) result = r;

    r = frezon_production_reaction(tile, config);
    if (r == REACTION_STOP) return r;
    if (r != REACTION_NONE) result = r;

    r = frezon_coolant_reaction(tile, config);
    if (r == REACTION_STOP) return r;
    if (r != REACTION_NONE) result = r;

    r = water_vapor_reaction(tile, config);
    if (r == REACTION_STOP) return r;
    if (r != REACTION_NONE) result = r;

    r = n2o_decomposition_reaction(tile, config);
    if (r == REACTION_STOP) return r;
    if (r != REACTION_NONE) result = r;

    r = ammonia_oxygen_reaction(tile, config);
    if (r == REACTION_STOP) return r;
    if (r != REACTION_NONE) result = r;

    return result;
}