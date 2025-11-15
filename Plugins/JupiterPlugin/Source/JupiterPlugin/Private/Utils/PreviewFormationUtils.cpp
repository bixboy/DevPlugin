#include "Utils/PreviewFormationUtils.h"

#include "Components/UnitFormationComponent.h"
#include "Components/UnitSpawnComponent.h"

namespace PreviewFormationUtils
{
    int32 GetEffectiveSpawnCount(const UUnitSpawnComponent* SpawnComponent)
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

    void BuildSpawnFormationOffsets(const UUnitSpawnComponent* SpawnComponent, int32 SpawnCount, float Spacing, TArray<FVector>& OutOffsets)
    {
        OutOffsets.Reset();

        if (SpawnCount <= 0)
        {
            return;
        }

        OutOffsets.Reserve(SpawnCount);

        const ESpawnFormation Formation = SpawnComponent ? SpawnComponent->GetSpawnFormation() : ESpawnFormation::Square;

        auto AddGridOffsets = [&OutOffsets, Spacing](int32 Columns, int32 Rows)
        {
            const float HalfWidth = (Columns - 1) * 0.5f * Spacing;
            const float HalfHeight = (Rows - 1) * 0.5f * Spacing;

            for (int32 Index = 0; Index < Columns * Rows; ++Index)
            {
                const int32 Column = Index % Columns;
                const int32 Row = Index / Columns;
                const float OffsetX = (Column * Spacing) - HalfWidth;
                const float OffsetY = (Row * Spacing) - HalfHeight;
                OutOffsets.Add(FVector(OffsetX, OffsetY, 0.f));
            }
        };

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
                const int32 MaxUnits = FMath::Min(SpawnCount, Columns * Rows);
                OutOffsets.Reserve(MaxUnits);

                const float HalfWidth = (Columns - 1) * 0.5f * Spacing;
                const float HalfHeight = (Rows - 1) * 0.5f * Spacing;

                for (int32 Index = 0; Index < MaxUnits; ++Index)
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
                AddGridOffsets(Columns, Rows);
                OutOffsets.SetNum(SpawnCount);
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

    void BuildCommandPreviewTransforms(const UUnitFormationComponent* FormationComponent, const FCommandData& BaseCommand, const TArray<AActor*>& SelectedUnits, TArray<FTransform>& OutTransforms)
    {
        OutTransforms.Reset();

        if (!FormationComponent || SelectedUnits.IsEmpty())
        {
            for (int32 Index = 0; Index < SelectedUnits.Num(); ++Index)
            {
                OutTransforms.Add(FTransform(BaseCommand.Rotation, BaseCommand.Location));
            }
            if (OutTransforms.IsEmpty())
            {
                OutTransforms.Add(FTransform(BaseCommand.Rotation, BaseCommand.Location));
            }
            return;
        }

        TArray<FCommandData> Commands;
        FormationComponent->BuildFormationCommands(BaseCommand, SelectedUnits, Commands);
        if (Commands.IsEmpty())
        {
            for (int32 Index = 0; Index < SelectedUnits.Num(); ++Index)
            {
                OutTransforms.Add(FTransform(BaseCommand.Rotation, BaseCommand.Location));
            }
            return;
        }

        OutTransforms.Reserve(Commands.Num());
        for (const FCommandData& Command : Commands)
        {
            OutTransforms.Add(FTransform(Command.Rotation, Command.Location));
        }
    }
}

