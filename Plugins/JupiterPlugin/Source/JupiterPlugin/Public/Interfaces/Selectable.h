#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Data/AiData.h"
#include "Data/UnitsSelectionData.h"
#include "Selectable.generated.h"

UINTERFACE()
class USelectable : public UInterface
{
	GENERATED_BODY()
};

class JUPITERPLUGIN_API ISelectable
{
	GENERATED_BODY()

public:
	UFUNCTION()
	virtual void Select() = 0;
	
	UFUNCTION()
	virtual void Deselect() = 0;

	UFUNCTION()
	virtual void Highlight(const bool Highlight) = 0;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool GetIsSelected();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	ESelectionType GetSelectionType();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	ETeams GetCurrentTeam();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SetBehavior(const ECombatBehavior NewBehavior);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	ECombatBehavior GetBehavior();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void CommandMove(FCommandData CommandData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FCommandData GetCurrentCommand();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool GetIsInAttack();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool GetCanAttack();
	
};
