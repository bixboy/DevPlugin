#include "JupiterPlugin/Public/Player/PlayerCamera.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/UnitSelectionComponent.h"
#include "Components/UnitSpawnComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Math/RandomStream.h"
#include "Units/SoldierRts.h"
#include "Utilities/PreviewPoseMesh.h"

void APlayerCamera::CreatePreviewMesh()
{
        if (!Player || !Player->IsLocalController() || !PreviewUnitsClass)
        {
                return;
        }

        EnsurePreviewUnit();

        if (SpawnComponent)
        {
                ShowUnitPreview(SpawnComponent->GetUnitToSpawn());
        }
}

void APlayerCamera::Input_OnSpawnUnits()
{
        if (!bIsInSpawnUnits || !HasPreviewUnits())
        {
                return;
        }

        if (PreviewUnit && PreviewUnit->GetIsValidPlacement() && SpawnComponent)
        {
                const FVector SpawnLocation = SpawnRotationPreview.Center;
                const FRotator SpawnRotation = SpawnRotationPreview.bPreviewActive ? SpawnRotationPreview.CurrentRotation : FRotator::ZeroRotator;
                SpawnComponent->SpawnUnitsWithTransform(SpawnLocation, SpawnRotation);
        }

        SpawnRotationPreview.Deactivate();
}

void APlayerCamera::ShowUnitPreview(TSubclassOf<ASoldierRts> NewUnitClass)
{
        if (!HasPreviewUnits())
        {
                return;
        }

        if (NewUnitClass)
        {
                if (ASoldierRts* DefaultSoldier = NewUnitClass->GetDefaultObject<ASoldierRts>())
                {
                        const int32 InstanceCount = SpawnComponent ? GetEffectiveSpawnCount() : 1;

                        if (USkeletalMeshComponent* MeshComponent = DefaultSoldier->GetMesh())
                        {
                                if (USkeletalMesh* SkeletalMesh = MeshComponent->GetSkeletalMeshAsset())
                                {
                                        const FRotator BaseRotation(0.f, CameraComponent->GetComponentRotation().Yaw, 0.f);
                                        bIsInSpawnUnits = true;
                                        SpawnRotationPreview.Reset(BaseRotation);
                                        PreviewUnit->ShowPreview(SkeletalMesh, DefaultSoldier->GetCapsuleComponent()->GetRelativeScale3D(), InstanceCount);
                                        if (GetSelectionComponentChecked())
                                        {
                                                PreviewFollowMouse();
                                                return;
                                        }
                                        HidePreview();
                                        return;
                                }
                        }

                        if (UStaticMeshComponent* StaticMeshComponent = DefaultSoldier->FindComponentByClass<UStaticMeshComponent>())
                        {
                                if (UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh())
                                {
                                        const FRotator BaseRotation(0.f, CameraComponent->GetComponentRotation().Yaw, 0.f);
                                        bIsInSpawnUnits = true;
                                        SpawnRotationPreview.Reset(BaseRotation);
                                        PreviewUnit->ShowPreview(StaticMesh, StaticMeshComponent->GetRelativeScale3D(), InstanceCount);
                                        if (GetSelectionComponentChecked())
                                        {
                                                PreviewFollowMouse();
                                                return;
                                        }
                                        HidePreview();
                                        return;
                                }
                        }
                }
        }

        HidePreview();
}

void APlayerCamera::HidePreview()
{
        if (PreviewUnit)
        {
                PreviewUnit->HidePreview();
        }
        bIsInSpawnUnits = false;
        SpawnRotationPreview.Deactivate();
}

void APlayerCamera::PreviewFollowMouse()
{
        if (!bIsInSpawnUnits || !HasPreviewUnits() || !Player)
        {
                return;
        }

        FHitResult MouseHit;
        if (UUnitSelectionComponent* Selection = GetSelectionComponentChecked())
        {
                MouseHit = Selection->GetMousePositionOnTerrain();
        }
        else
        {
                return;
        }
        const FVector MouseLocation = MouseHit.Location;

        if (MouseLocation == FVector::ZeroVector)
        {
                return;
        }

        const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
        const bool bLeftMouseDown = Player->IsInputKeyDown(EKeys::LeftMouseButton);
        const bool bRightMouseDown = Player->IsInputKeyDown(EKeys::RightMouseButton);
        const bool bSpawnInputDown = bLeftMouseDown || bRightMouseDown;

        const bool bLeftMouseJustPressed = Player->WasInputKeyJustPressed(EKeys::LeftMouseButton);
        const bool bRightMouseJustPressed = Player->WasInputKeyJustPressed(EKeys::RightMouseButton);
        const bool bSpawnInputJustPressed = bLeftMouseJustPressed || bRightMouseJustPressed;

        const FRotator MouseRotation(0.f, CameraComponent->GetComponentRotation().Yaw, 0.f);

        if (bSpawnInputJustPressed)
        {
                SpawnRotationPreview.BeginHold(CurrentTime, MouseLocation, MouseRotation);
        }
        else if (!bSpawnInputDown)
        {
                SpawnRotationPreview.StopHold();
        }

        if (!SpawnRotationPreview.bPreviewActive)
        {
                SpawnRotationPreview.Center = MouseLocation;
                SpawnRotationPreview.BaseRotation = MouseRotation;
                SpawnRotationPreview.CurrentRotation = MouseRotation;
        }

        if (SpawnRotationPreview.bHoldActive && !SpawnRotationPreview.bPreviewActive)
        {
                if (SpawnRotationPreview.TryActivate(CurrentTime, RotationPreviewHoldTime, MouseLocation, MouseLocation))
                {
                        SpawnRotationPreview.InitialDirection = FRotationPreviewState::ResolvePlanarDirection(MouseLocation - SpawnRotationPreview.Center, SpawnRotationPreview.BaseRotation);
                }
        }

        if (SpawnRotationPreview.bPreviewActive)
        {
                SpawnRotationPreview.UpdateRotation(MouseLocation);
        }

        UpdatePreviewTransforms(SpawnRotationPreview.Center, SpawnRotationPreview.CurrentRotation);
}

void APlayerCamera::HandleSpawnCountChanged(int32 /*NewSpawnCount*/)
{
        RefreshPreviewInstances();
}

void APlayerCamera::HandleSpawnFormationChanged(ESpawnFormation /*NewFormation*/)
{
        RefreshPreviewInstances();
}

void APlayerCamera::HandleCustomFormationDimensionsChanged(FIntPoint /*NewDimensions*/)
{
        RefreshPreviewInstances();
}

void APlayerCamera::EnsurePreviewUnit()
{
        if (!Player || !Player->IsLocalController() || PreviewUnit)
        {
                return;
        }

        if (!PreviewUnitsClass)
        {
                return;
        }

        if (UWorld* World = GetWorld())
        {
                FActorSpawnParameters SpawnParams;
                SpawnParams.Instigator = this;
                SpawnParams.Owner = this;

                if (APreviewPoseMesh* Preview = World->SpawnActor<APreviewPoseMesh>(PreviewUnitsClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams))
                {
                        Preview->SetReplicates(false);
                        Preview->SetOwner(this);
                        Preview->SetActorHiddenInGame(true);
                        PreviewUnit = Preview;
                }
        }
}

void APlayerCamera::UpdatePreviewTransforms(const FVector& CenterLocation, const FRotator& FacingRotation)
{
        if (!HasPreviewUnits() || !PreviewUnit)
        {
                return;
        }

        const int32 SpawnCount = GetEffectiveSpawnCount();
        const float Spacing = SpawnComponent ? FMath::Max(SpawnComponent->GetFormationSpacing(), 0.f) : 0.f;

        TArray<FVector> FormationOffsets;
        BuildPreviewFormationOffsets(SpawnCount, Spacing, FormationOffsets);

        if (FormationOffsets.Num() == 0)
        {
                return;
        }

        TArray<FTransform> InstanceTransforms;
        InstanceTransforms.Reserve(FormationOffsets.Num());

        const FQuat RotationQuat = FRotator(0.f, FacingRotation.Yaw, 0.f).Quaternion();
        for (const FVector& Offset : FormationOffsets)
        {
                const FVector RotatedOffset = RotationQuat.RotateVector(Offset);
                const FVector FormationLocation = CenterLocation + RotatedOffset;
                InstanceTransforms.Add(FTransform(FacingRotation, FormationLocation));
        }

        PreviewUnit->UpdateInstances(InstanceTransforms);
}

void APlayerCamera::RefreshPreviewInstances()
{
        EnsurePreviewUnit();

        if (bIsInSpawnUnits && SpawnComponent)
        {
                ShowUnitPreview(SpawnComponent->GetUnitToSpawn());
                PreviewFollowMouse();
        }
}

void APlayerCamera::BuildPreviewFormationOffsets(int32 SpawnCount, float Spacing, TArray<FVector>& OutOffsets) const
{
        OutOffsets.Reset();

        if (SpawnCount <= 0)
        {
                return;
        }

        OutOffsets.Reserve(SpawnCount);

        const ESpawnFormation Formation = SpawnComponent ? SpawnComponent->GetSpawnFormation() : ESpawnFormation::Square;

        switch (Formation)
        {
                case ESpawnFormation::Line:
                {
                        const float HalfWidth = (SpawnCount - 1) * 0.5f * Spacing;
                        for (int32 Index = 0; Index < SpawnCount; ++Index)
                        {
                                const float OffsetX = (Index * Spacing) - HalfWidth;
                                OutOffsets.Add(FVector(OffsetX, 0.f, 0.f));
                        }
                        break;
                }
                case ESpawnFormation::Column:
                {
                        const float HalfHeight = (SpawnCount - 1) * 0.5f * Spacing;
                        for (int32 Index = 0; Index < SpawnCount; ++Index)
                        {
                                const float OffsetY = (Index * Spacing) - HalfHeight;
                                OutOffsets.Add(FVector(0.f, OffsetY, 0.f));
                        }
                        break;
                }
                case ESpawnFormation::Wedge:
                {
                        int32 UnitsPlaced = 0;
                        int32 Row = 0;
                        while (UnitsPlaced < SpawnCount)
                        {
                                const int32 UnitsInRow = FMath::Min(Row + 1, SpawnCount - UnitsPlaced);
                                const float RowHalfWidth = (UnitsInRow - 1) * 0.5f * Spacing;
                                const float RowOffsetY = Row * Spacing;
                                for (int32 IndexInRow = 0; IndexInRow < UnitsInRow; ++IndexInRow)
                                {
                                        const float OffsetX = (IndexInRow * Spacing) - RowHalfWidth;
                                        OutOffsets.Add(FVector(OffsetX, RowOffsetY, 0.f));
                                }

                                UnitsPlaced += UnitsInRow;
                                ++Row;
                        }
                        break;
                }
                case ESpawnFormation::Blob:
                {
                        OutOffsets.Add(FVector::ZeroVector);

                        if (SpawnCount > 1)
                        {
                                FRandomStream RandomStream(12345);
                                const float MaxRadius = (Spacing <= 0.f) ? 0.f : Spacing * FMath::Sqrt(static_cast<float>(SpawnCount));

                                for (int32 Index = 1; Index < SpawnCount; ++Index)
                                {
                                        const float Angle = RandomStream.FRandRange(0.f, 2.f * PI);
                                        const float Radius = (MaxRadius > 0.f) ? RandomStream.FRandRange(0.f, MaxRadius) : 0.f;
                                        const float OffsetX = Radius * FMath::Cos(Angle);
                                        const float OffsetY = Radius * FMath::Sin(Angle);
                                        OutOffsets.Add(FVector(OffsetX, OffsetY, 0.f));
                                }
                        }
                        break;
                }
                case ESpawnFormation::Custom:
                {
                        const FIntPoint Dimensions = SpawnComponent ? SpawnComponent->GetCustomFormationDimensions() : FIntPoint(1, 1);
                        const int32 Columns = FMath::Max(1, Dimensions.X);
                        const int32 Rows = FMath::Max(1, Dimensions.Y);
                        const float HalfWidth = (Columns - 1) * 0.5f * Spacing;
                        const float HalfHeight = (Rows - 1) * 0.5f * Spacing;

                        for (int32 Index = 0; Index < SpawnCount; ++Index)
                        {
                                const int32 Column = Index % Columns;
                                const int32 RowIndex = Index / Columns;

                                const float OffsetX = (Column * Spacing) - HalfWidth;
                                const float OffsetY = (RowIndex * Spacing) - HalfHeight;
                                OutOffsets.Add(FVector(OffsetX, OffsetY, 0.f));
                        }
                        break;
                }
                case ESpawnFormation::Square:
                default:
                {
                        const int32 Columns = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt(static_cast<float>(SpawnCount))));
                        const int32 Rows = FMath::Max(1, FMath::CeilToInt(static_cast<float>(SpawnCount) / static_cast<float>(Columns)));
                        const float HalfWidth = (Columns - 1) * 0.5f * Spacing;
                        const float HalfHeight = (Rows - 1) * 0.5f * Spacing;

                        for (int32 Index = 0; Index < SpawnCount; ++Index)
                        {
                                const int32 Column = Index % Columns;
                                const int32 RowIndex = Index / Columns;

                                const float OffsetX = (Column * Spacing) - HalfWidth;
                                const float OffsetY = (RowIndex * Spacing) - HalfHeight;
                                OutOffsets.Add(FVector(OffsetX, OffsetY, 0.f));
                        }
                        break;
                }
        }

        if (OutOffsets.Num() > 0 && (Formation == ESpawnFormation::Wedge || Formation == ESpawnFormation::Blob))
        {
                FVector AverageOffset = FVector::ZeroVector;
                for (const FVector& Offset : OutOffsets)
                {
                        AverageOffset += Offset;
                }

                AverageOffset /= static_cast<float>(OutOffsets.Num());

                for (FVector& Offset : OutOffsets)
                {
                        Offset -= FVector(AverageOffset.X, AverageOffset.Y, 0.f);
                }
        }
}

int32 APlayerCamera::GetEffectiveSpawnCount() const
{
        if (!SpawnComponent)
        {
                return 1;
        }

        if (SpawnComponent->GetSpawnFormation() == ESpawnFormation::Custom)
        {
                const FIntPoint Dimensions = SpawnComponent->GetCustomFormationDimensions();
                return FMath::Max(1, Dimensions.X * Dimensions.Y);
        }

        return FMath::Max(SpawnComponent->GetUnitsPerSpawn(), 1);
}

