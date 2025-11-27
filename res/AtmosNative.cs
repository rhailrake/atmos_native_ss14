using System.Runtime.InteropServices;

namespace Content.Server.Atmos.Native;

public static class AtmosGasId
{
    public const int Oxygen = 0;
    public const int Nitrogen = 1;
    public const int CO2 = 2;
    public const int Plasma = 3;
    public const int Tritium = 4;
    public const int WaterVapor = 5;
    public const int Ammonia = 6;
    public const int N2O = 7;
    public const int Frezon = 8;
    public const int GasCount = 9;
    public const int GasArraySize = 12;
}

public static class AtmosDirection
{
    public const int North = 0;
    public const int South = 1;
    public const int East = 2;
    public const int West = 3;
    public const int Count = 4;

    public const byte BitNorth = 1 << 0;
    public const byte BitSouth = 1 << 1;
    public const byte BitEast = 1 << 2;
    public const byte BitWest = 1 << 3;
    public const byte BitAll = 0x0F;
}

public static class TileFlags
{
    public const byte Space = 1 << 0;
    public const byte Excited = 1 << 1;
    public const byte Hotspot = 1 << 2;
    public const byte Immutable = 1 << 3;
    public const byte MapAtmos = 1 << 4;
    public const byte Superconduct = 1 << 5;
    public const byte Processed = 1 << 6;
}

[StructLayout(LayoutKind.Sequential)]
public unsafe struct TileAtmosData
{
    public fixed float Moles[12];
    public fixed float MolesArchived[12];
    public float Temperature;
    public float TemperatureArchived;
    public float HeatCapacity;
    public float ThermalConductivity;
    public float PressureDifference;
    public float LastShare;

    public int GridX;
    public int GridY;
    public fixed int AdjacentIndices[4];

    public byte AdjacentBits;
    public byte BlockedBits;
    public byte Flags;
    public byte HotspotState;

    public float HotspotTemperature;
    public float HotspotVolume;

    public float MoleDelta;
    public fixed float TransferDirections[4];
    public float CurrentTransferAmount;
    public int CurrentTransferDirection;

    public int LastCycle;
    public int LastQueueCycle;
    public int LastSlowQueueCycle;
    public int ExcitedGroupId;

    public byte FastDone;
    public fixed byte Padding[3];
}

[StructLayout(LayoutKind.Sequential)]
public struct AtmosConstants
{
    public float R;
    public float OneAtmosphere;
    public float TCMB;
    public float T0C;
    public float T20C;
    public float Tmax;
    public float CellVolume;
    public float GasMinMoles;
    public float OpenHeatTransferCoefficient;
    public float HeatCapacityVacuum;
    public float MinimumAirRatioToSuspend;
    public float MinimumAirRatioToMove;
    public float MinimumAirToSuspend;
    public float MinimumTemperatureToMove;
    public float MinimumMolesDeltaToMove;
    public float MinimumTemperatureDeltaToSuspend;
    public float MinimumTemperatureDeltaToConsider;
    public float MinimumTemperatureStartSuperConduction;
    public float MinimumTemperatureForSuperconduction;
    public float MinimumHeatCapacity;
    public float SpaceHeatCapacity;
    public float FireMinimumTemperatureToExist;
    public float FireMinimumTemperatureToSpread;
    public float FireSpreadRadiosityScale;
    public float FirePlasmaEnergyReleased;
    public float FireHydrogenEnergyReleased;
    public float FireGrowthRate;
    public float PlasmaMinimumBurnTemperature;
    public float PlasmaUpperTemperature;
    public float PlasmaOxygenFullburn;
    public float PlasmaBurnRateDelta;
    public float OxygenBurnRateBase;
    public float SuperSaturationThreshold;
    public float TritiumBurnOxyFactor;
    public float TritiumBurnTritFactor;
    public float FrezonCoolLowerTemperature;
    public float FrezonCoolMidTemperature;
    public float FrezonCoolMaximumEnergyModifier;
    public float FrezonNitrogenCoolRatio;
    public float FrezonCoolEnergyReleased;
    public float FrezonCoolRateModifier;
    public float WindowHeatTransferCoefficient;
    public float McellWithRatio;

    public int ExcitedGroupBreakdownCycles;
    public int ExcitedGroupsDismantleCycles;
    public int MonstermosHardTileLimit;
    public int MonstermosTileLimit;
}

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public unsafe struct AtmosConfig
{
    public fixed float GasSpecificHeats[12];
    public AtmosConstants Constants;
    public int MaxProcessTimeMicroseconds;
    public float Speedup;
    public float HeatScale;
    public byte MonstermosEnabled;
    public byte ExcitedGroupsEnabled;
    public byte SuperconductionEnabled;
    public byte SpacingEnabled;
    public float SpacingEscapeRatio;
    public float SpacingMinGas;
    public float SpacingMaxWind;
}

[StructLayout(LayoutKind.Sequential)]
public struct AtmosResult
{
    public int TilesProcessed;
    public int ActiveTilesCount;
    public int HotspotTilesCount;
    public int SuperconductTilesCount;
    public int ExcitedGroupsCount;
    public int ReactionsTriggered;
    public float MaxPressureDelta;
    public byte ProcessingComplete;
    public byte Padding1;
    public byte Padding2;
    public byte Padding3;
}

public static unsafe class AtmosNative
{
    private const string LibName = "atmos_native";

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern uint atmos_get_version();

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern uint atmos_get_simd_level();

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr atmos_create_grid(int initialCapacity);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void atmos_destroy_grid(IntPtr state);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void atmos_reset_grid(IntPtr state);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int atmos_add_tile(IntPtr state, TileAtmosData* tile);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void atmos_update_tile(IntPtr state, int index, TileAtmosData* tile);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern TileAtmosData* atmos_get_tile(IntPtr state, int index);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void atmos_set_adjacency(IntPtr state, int tileIndex, int direction, int adjacentIndex);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void atmos_add_active_tile(IntPtr state, int tileIndex);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void atmos_remove_active_tile(IntPtr state, int tileIndex);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern AtmosResult atmos_process(IntPtr state, AtmosConfig* config);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern AtmosResult atmos_process_revalidate(IntPtr state, AtmosConfig* config);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern AtmosResult atmos_process_active_tiles(IntPtr state, AtmosConfig* config);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern AtmosResult atmos_process_excited_groups(IntPtr state, AtmosConfig* config);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern AtmosResult atmos_process_hotspots(IntPtr state, AtmosConfig* config);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern AtmosResult atmos_process_superconductivity(IntPtr state, AtmosConfig* config);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern AtmosResult atmos_process_high_pressure(IntPtr state, AtmosConfig* config);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern AtmosResult atmos_equalize_pressure_zone(IntPtr state, int startTile, AtmosConfig* config);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern AtmosResult atmos_explosive_depressurize(IntPtr state, int startTile, AtmosConfig* config);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void atmos_ignite_hotspot(IntPtr state, int tileIndex, float temperature, float volume);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void atmos_extinguish_hotspot(IntPtr state, int tileIndex);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern float atmos_get_heat_capacity(TileAtmosData* tile, float* specificHeats);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern float atmos_get_heat_capacity_archived(TileAtmosData* tile, float* specificHeats);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern float atmos_get_thermal_energy(TileAtmosData* tile, float* specificHeats);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void atmos_merge(TileAtmosData* receiver, TileAtmosData* giver, float* specificHeats);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void atmos_remove_gas(TileAtmosData* tile, float amount, TileAtmosData* removed);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void atmos_remove_ratio(TileAtmosData* tile, float ratio, TileAtmosData* removed);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int atmos_react(TileAtmosData* tile, AtmosConfig* config);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void atmos_share(TileAtmosData* receiver, TileAtmosData* sharer, int adjacentCount, AtmosConfig* config);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern float atmos_temperature_share(TileAtmosData* receiver, TileAtmosData* sharer, float conductionCoefficient, AtmosConfig* config);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void atmos_config_init_default(AtmosConfig* config);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int atmos_get_active_tile_count(IntPtr state);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int atmos_get_tile_count(IntPtr state);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern TileAtmosData* atmos_get_tiles_ptr(IntPtr state);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void atmos_archive_tile(TileAtmosData* tile);

    [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
    public static extern void atmos_archive_all(IntPtr state);
}
