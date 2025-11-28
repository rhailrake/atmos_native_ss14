# atmos_native_ss14

C++ library for atmosphere processing in Space Station 14.

## advantages

- SIMD
- tiles stored contiguously in memory
- multithreaded processing
- tests

## risks

- harder to debug
- possible floating-point desync

## performance

2–5× faster than the managed implementation on large atmospheric events (tested on SS14 stable branch).
Use this only if the map contains more than 5,000 tiles. Otherwise, use the default SS14 atmos.

## how to use

- Create your own atmos manager to handle simulation/syncing/convertion.
- See [AtmosNative.cs](https://github.com/rhailrake/atmos_native_ss14/blob/master/res/AtmosNative.cs) for more.
- Replace default SS14 atmos calls to your atmos manager.
- Make sure that SS14 atmos data matches the format expected by the library.
- Fix any debug asserts.
