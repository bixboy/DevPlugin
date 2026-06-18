#pragma once
#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "StellarThumbnailSubsystem.generated.h"


/**
 * Subsystem responsible for custom asset thumbnail operations in the Editor.
 * Instantiated automatically by the engine upon Editor startup.
 */
UCLASS()
class STELLAREDITORTOOLS_API UStellarThumbnailSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void RegisterMenu();

	void OnSetCustomThumbnailClicked();
};
