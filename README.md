# DevPlugin

## RTS Control Plugin (Refactored)

This project now exposes a modular RTS control plugin built around dedicated subsystems:

* **Selection** – `USelectionComponent` keeps replicated selections lightweight and forwards updates to `URTSUnitSelectionSubsystem`.
* **Orders** – `URTSOrderSystem` normalises orders through `FRTSOrderRequest` while remaining backwards compatible with `FCommandData`.
* **Production & Resources** – `URTSProductionSystem` and `URTSResourceSystem` provide extensible management of build queues and player economies.

### Example usage

```cpp
// Player controller issuing a move order to the current selection.
FRTSOrderRequest MoveRequest;
MoveRequest.OrderType = ERTSOrderType::Move;
MoveRequest.TargetLocation = DesiredLocation;
SelectionComponent->IssueOrderToSelection(MoveRequest);

// Game mode helper (see ARtsGameGameMode::IssueExampleMoveOrder)
IssueExampleMoveOrder(PlayerController, DesiredLocation);
```

### Suggested extensions

* Implement contextual command previews driven by `FRTSOrderRequest::OrderTags`.
* Extend `URTSProductionSystem` to broadcast Blueprint events when spawning completed units.
* Hook `URTSResourceSystem` into gathering units for fully automated economy balancing.
