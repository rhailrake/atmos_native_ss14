#include "test_common.h"
#include <iostream>
#include <iomanip>

class PerformanceTest : public AtmosTestFixture {
protected:
    void SetUp() override {
        AtmosTestFixture::SetUp();
        config.monstermosEnabled = 1;
        config.excitedGroupsEnabled = 1;
        config.superconductionEnabled = 1;
        config.maxProcessTimeMicroseconds = 1000000;
    }
    
    void SetupLargeGrid(int width, int height) {
        atmos_destroy_grid(state);
        state = atmos_create_grid(width * height);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                TileAtmosData tile = CreateStandardTile(x, y);
                atmos_add_tile(state, &tile);
            }
        }
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = y * width + x;
                if (x < width - 1)
                    SetupAdjacency(idx, idx + 1, ATMOS_DIR_EAST);
                if (y < height - 1)
                    SetupAdjacency(idx, idx + width, ATMOS_DIR_SOUTH);
            }
        }
    }
    
    void PrintResult(const std::string& name, double timeMs, int operations) {
        std::cout << std::setw(40) << std::left << name 
                  << std::setw(12) << std::right << std::fixed << std::setprecision(3) << timeMs << " ms"
                  << std::setw(12) << operations << " ops"
                  << std::setw(12) << std::fixed << std::setprecision(0) << (operations / timeMs * 1000.0) << " ops/s"
                  << std::endl;
    }
};

TEST_F(PerformanceTest, HeatCapacityCalculation) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    
    const int iterations = 100000;
    PerformanceTimer timer;
    
    timer.Start();
    volatile float result = 0;
    for (int i = 0; i < iterations; i++) {
        result = atmos_get_heat_capacity(&tile, config.gasSpecificHeats);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("HeatCapacity", elapsed, iterations);
    EXPECT_LT(elapsed, 1000.0);
}

TEST_F(PerformanceTest, ThermalEnergyCalculation) {
    TileAtmosData tile = CreateStandardTile(0, 0);
    
    const int iterations = 100000;
    PerformanceTimer timer;
    
    timer.Start();
    volatile float result = 0;
    for (int i = 0; i < iterations; i++) {
        result = atmos_get_thermal_energy(&tile, config.gasSpecificHeats);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("ThermalEnergy", elapsed, iterations);
    EXPECT_LT(elapsed, 1000.0);
}

TEST_F(PerformanceTest, MergeOperation) {
    TileAtmosData receiver = CreateStandardTile(0, 0);
    TileAtmosData giver = CreateStandardTile(1, 0);
    giver.temperature = 400.0f;
    
    const int iterations = 100000;
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < iterations; i++) {
        receiver = CreateStandardTile(0, 0);
        atmos_merge(&receiver, &giver, config.gasSpecificHeats);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("Merge", elapsed, iterations);
    EXPECT_LT(elapsed, 1000.0);
}

TEST_F(PerformanceTest, ShareOperation) {
    TileAtmosData receiver = CreateStandardTile(0, 0);
    TileAtmosData sharer = CreateHighPressureTile(1, 0);
    
    const int iterations = 100000;
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < iterations; i++) {
        receiver = CreateStandardTile(0, 0);
        sharer = CreateHighPressureTile(1, 0);
        atmos_archive_tile(&receiver);
        atmos_archive_tile(&sharer);
        atmos_share(&receiver, &sharer, 4, &config);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("Share", elapsed, iterations);
    EXPECT_LT(elapsed, 2000.0);
}

TEST_F(PerformanceTest, ReactOperation) {
    TileAtmosData tile = CreatePlasmaTile(0, 0, 10.0f, 20.0f);
    tile.temperature = config.constants.plasmaUpperTemperature + 100.0f;
    
    const int iterations = 100000;
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < iterations; i++) {
        TileAtmosData testTile = CreatePlasmaTile(0, 0, 10.0f, 20.0f);
        testTile.temperature = config.constants.plasmaUpperTemperature + 100.0f;
        atmos_react(&testTile, &config);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("React", elapsed, iterations);
    EXPECT_LT(elapsed, 2000.0);
}

TEST_F(PerformanceTest, SmallGridProcessing) {
    SetupLargeGrid(10, 10);
    
    state->tiles[50].moles[GAS_OXYGEN] = 200.0f;
    state->tiles[50].moles[GAS_NITROGEN] = 800.0f;
    
    for (int i = 0; i < 100; i++) {
        atmos_add_active_tile(state, i);
    }
    
    const int iterations = 100;
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < iterations; i++) {
        atmos_process(state, &config);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("SmallGrid (10x10) process", elapsed, iterations);
    EXPECT_LT(elapsed / iterations, 10.0);
}

TEST_F(PerformanceTest, MediumGridProcessing) {
    SetupLargeGrid(50, 50);
    
    state->tiles[1275].moles[GAS_OXYGEN] = 500.0f;
    state->tiles[1275].moles[GAS_NITROGEN] = 2000.0f;
    
    for (int i = 0; i < 2500; i++) {
        atmos_add_active_tile(state, i);
    }
    
    const int iterations = 20;
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < iterations; i++) {
        atmos_process(state, &config);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("MediumGrid (50x50) process", elapsed, iterations);
    EXPECT_LT(elapsed / iterations, 100.0);
}

TEST_F(PerformanceTest, LargeGridProcessing) {
    SetupLargeGrid(100, 100);
    
    state->tiles[5050].moles[GAS_OXYGEN] = 1000.0f;
    state->tiles[5050].moles[GAS_NITROGEN] = 4000.0f;
    
    for (int i = 0; i < 10000; i++) {
        atmos_add_active_tile(state, i);
    }
    
    const int iterations = 5;
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < iterations; i++) {
        atmos_process(state, &config);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("LargeGrid (100x100) process", elapsed, iterations);
    EXPECT_LT(elapsed / iterations, 500.0);
}

TEST_F(PerformanceTest, EqualizePressureZone) {
    SetupLargeGrid(50, 50);
    
    state->tiles[1275].moles[GAS_OXYGEN] = 1000.0f;
    state->tiles[1275].moles[GAS_NITROGEN] = 4000.0f;
    
    const int iterations = 50;
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < iterations; i++) {
        for (int j = 0; j < 2500; j++) {
            state->tiles[j].lastCycle = 0;
            state->tiles[j].lastQueueCycle = 0;
        }
        state->updateCounter++;
        atmos_equalize_pressure_zone(state, 1275, &config);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("EqualizePressureZone (50x50)", elapsed, iterations);
    EXPECT_LT(elapsed / iterations, 50.0);
}

TEST_F(PerformanceTest, ExplosiveDepressurize) {
    SetupLargeGrid(30, 30);
    
    TileAtmosData spaceTile = CreateSpaceTile(30, 15);
    int32_t spaceIdx = atmos_add_tile(state, &spaceTile);
    SetupAdjacency(15 * 30 + 29, spaceIdx, ATMOS_DIR_EAST);
    
    const int iterations = 20;
    PerformanceTimer timer;
    
    timer.Start();
    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < 900; i++) {
            state->tiles[i].moles[GAS_OXYGEN] = 21.0f;
            state->tiles[i].moles[GAS_NITROGEN] = 79.0f;
            state->tiles[i].lastCycle = 0;
            state->tiles[i].lastQueueCycle = 0;
            state->tiles[i].lastSlowQueueCycle = 0;
        }
        state->highPressureTileCount = 0;
        state->updateCounter++;
        atmos_explosive_depressurize(state, 15 * 30 + 15, &config);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("ExplosiveDepressurize (30x30)", elapsed, iterations);
    EXPECT_LT(elapsed / iterations, 100.0);
}

TEST_F(PerformanceTest, HotspotProcessing) {
    SetupLargeGrid(20, 20);
    
    for (int i = 0; i < 50; i++) {
        state->tiles[i].moles[GAS_OXYGEN] = 30.0f;
        state->tiles[i].moles[GAS_PLASMA] = 20.0f;
        state->tiles[i].temperature = 600.0f;
        atmos_ignite_hotspot(state, i, 600.0f, 100.0f);
    }
    
    const int iterations = 100;
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < iterations; i++) {
        atmos_process_hotspots(state, &config);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("HotspotProcessing (50 hotspots)", elapsed, iterations);
    EXPECT_LT(elapsed / iterations, 10.0);
}

TEST_F(PerformanceTest, SuperconductivityProcessing) {
    SetupLargeGrid(20, 20);
    
    for (int i = 0; i < 100; i++) {
        state->tiles[i].temperature = 800.0f;
        state->tiles[i].thermalConductivity = 0.5f;
        consider_superconductivity(state, i, true, &config);
    }
    
    for (int i = 0; i < 400; i++) {
        atmos_archive_tile(&state->tiles[i]);
    }
    
    const int iterations = 100;
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < iterations; i++) {
        atmos_process_superconductivity(state, &config);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("SuperconductivityProcessing (100 tiles)", elapsed, iterations);
    EXPECT_LT(elapsed / iterations, 20.0);
}

TEST_F(PerformanceTest, ExcitedGroupProcessing) {
    SetupLargeGrid(30, 30);
    
    for (int g = 0; g < 50; g++) {
        int32_t groupId = create_excited_group(state);
        for (int t = 0; t < 10; t++) {
            int tileIdx = g * 10 + t;
            if (tileIdx < 900) {
                add_tile_to_excited_group(state, groupId, tileIdx);
            }
        }
        state->excitedGroups[groupId].breakdownCooldown = g % 5;
        state->excitedGroups[groupId].dismantleCooldown = g % 20;
    }
    
    const int iterations = 100;
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < iterations; i++) {
        atmos_process_excited_groups(state, &config);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("ExcitedGroupProcessing (50 groups)", elapsed, iterations);
    EXPECT_LT(elapsed / iterations, 20.0);
}

TEST_F(PerformanceTest, GridCreationAndDestruction) {
    const int iterations = 100;
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < iterations; i++) {
        GridAtmosState* testState = atmos_create_grid(10000);
        atmos_destroy_grid(testState);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("GridCreation/Destruction", elapsed, iterations);
    EXPECT_LT(elapsed / iterations, 5.0);
}

TEST_F(PerformanceTest, TileAddition) {
    const int tileCount = 10000;
    PerformanceTimer timer;
    
    atmos_destroy_grid(state);
    state = atmos_create_grid(tileCount);
    
    timer.Start();
    for (int i = 0; i < tileCount; i++) {
        TileAtmosData tile = CreateStandardTile(i % 100, i / 100);
        atmos_add_tile(state, &tile);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("TileAddition (10000 tiles)", elapsed, tileCount);
    EXPECT_LT(elapsed, 100.0);
}

TEST_F(PerformanceTest, ArchiveAllTiles) {
    SetupLargeGrid(100, 100);
    
    const int iterations = 100;
    PerformanceTimer timer;
    
    timer.Start();
    for (int i = 0; i < iterations; i++) {
        atmos_archive_all(state);
    }
    double elapsed = timer.ElapsedMs();
    
    PrintResult("ArchiveAll (10000 tiles)", elapsed, iterations);
    EXPECT_LT(elapsed / iterations, 5.0);
}

TEST_F(PerformanceTest, WorstCaseScenario) {
    SetupLargeGrid(25, 25);

    state->tiles[0].moles[GAS_OXYGEN] = 2000.0f;
    state->tiles[0].moles[GAS_NITROGEN] = 8000.0f;
    state->tiles[0].moles[GAS_PLASMA] = 50.0f;
    state->tiles[0].temperature = 1000.0f;

    for (int i = 0; i < 25; i++) {
        state->tiles[i].moles[GAS_PLASMA] = 20.0f;
        state->tiles[i].moles[GAS_OXYGEN] = 30.0f;
        state->tiles[i].temperature = 800.0f;
        atmos_ignite_hotspot(state, i, 800.0f, 500.0f);
    }

    for (int i = 0; i < 625; i++) {
        atmos_add_active_tile(state, i);
    }

    const int iterations = 5;
    PerformanceTimer timer;

    timer.Start();
    for (int i = 0; i < iterations; i++) {
        atmos_process(state, &config);
    }
    double elapsed = timer.ElapsedMs();

    PrintResult("WorstCase (25x25, fire)", elapsed, iterations);
    EXPECT_LT(elapsed / iterations, 500.0);
}

TEST_F(PerformanceTest, Throughput) {
    SetupLargeGrid(100, 100);
    
    for (int i = 0; i < 10000; i++) {
        if (i % 100 == 0) {
            state->tiles[i].moles[GAS_OXYGEN] = 100.0f;
            state->tiles[i].moles[GAS_NITROGEN] = 400.0f;
        }
        atmos_add_active_tile(state, i);
    }
    
    PerformanceTimer timer;
    timer.Start();
    
    int cycles = 0;
    while (timer.ElapsedMs() < 1000.0) {
        atmos_process(state, &config);
        cycles++;
    }
    
    double elapsed = timer.ElapsedMs();
    double tilesPerSecond = (cycles * 10000.0) / (elapsed / 1000.0);
    
    std::cout << std::setw(40) << std::left << "Throughput (tiles/second)"
              << std::setw(12) << std::right << std::fixed << std::setprecision(0) << tilesPerSecond
              << std::endl;
    
    EXPECT_GT(tilesPerSecond, 100000.0);
}