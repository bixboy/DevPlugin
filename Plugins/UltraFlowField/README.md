# UltraFlowField Plugin

UltraFlowField provides a production-ready, async flow field navigation solution designed for enormous, fully dynamic worlds in Unreal Engine 5.6. It delivers scalable crowd steering for hundreds of agents while keeping per-frame overhead minimal.

## Features

- Multi-layered flow fields with asynchronous Dijkstra propagation.
- Chunked voxel grid with configurable voxel size and chunk resolution.
- Double-buffered flow data for lock-free agent sampling.
- Incremental rebuilds triggered by goal changes and obstacle edits.
- Lightweight `UUnitFlowComponent` for agents and optional local avoidance.
- Debug visualization and blueprint-friendly API.

## Getting Started

1. Enable the **UltraFlowField** plugin in your project.
2. Drop an `ADynamicFlowFieldManager` into your level. Configure world bounds and voxel sizing via the embedded `UFlowVoxelGridComponent`.
3. Add `UUnitFlowComponent` to each agent pawn or character. Optionally add `ULocalAvoidanceComponent` to incorporate steering.
4. On gameplay start, call `SetGlobalTarget` on the manager to define the shared destination. Units query the flow every tick via `SampleDirection()` and convert to velocity.

## Key Blueprint Functions

- `SetGlobalTarget(FVector Target)` – updates the goal for the entire field.
- `MarkObstacleBox(FVector Center, FVector Extents, bool bBlocked)` – dynamically toggle world obstacles.
- `GetFlowDirectionAt(FVector Location, EAgentLayer Layer)` – sample the current flow direction for any position.
- `ToggleThrottle(bool bEnabled)` – enable/disable per-frame throttling.

## Performance Tips

- Adjust `VoxelSize` and chunk dimensions to balance fidelity and rebuild speed.
- Use agent layers to provide custom traversal costs per archetype.
- Keep `MaxPerFrameTimeMs` conservative (1–2 ms) to maintain frame-rate stability.
- Avoid excessive `SetGlobalTarget` spam; the scheduler coalesces requests but each rebuild costs CPU time.

## Example Content

The plugin ships with simple sample actors (`AFlowFieldSampleManagerActor`, `AFlowFieldSampleUnit`) to demonstrate spawning hundreds of agents following a flow field. Bind input to call `SpawnUnits`, `SetTargetAtLocation`, and `ToggleDebug` to explore the system quickly.

## Testing

Core systems include automated tests validating field propagation, obstacle updates, and buffer integrity. Run `Automation RunTests FlowField` from the Unreal command line to execute.

