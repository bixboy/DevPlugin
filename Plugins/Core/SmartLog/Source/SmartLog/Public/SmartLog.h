#pragma once
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Modules/ModuleInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSmart, Log, All);

class SMARTLOG_API FSmartLogModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
};

namespace SmartLogDetail
{
	inline FString ToStringInternal(const FString& Value) { return Value; }
	inline FString ToStringInternal(const FName& Value)   { return Value.ToString(); }
	inline FString ToStringInternal(const FText& Value)   { return Value.ToString(); }
	inline FString ToStringInternal(const char* Value)    { return FString(Value); }
	inline FString ToStringInternal(const TCHAR* Value)   { return FString(Value); }

	inline FString ToStringInternal(bool b) { return b ? TEXT("True") : TEXT("False"); }

	inline FString ToStringInternal(float Value)  { return FString::SanitizeFloat(Value); }
	inline FString ToStringInternal(double Value) { return FString::SanitizeFloat(Value); }
	inline FString ToStringInternal(int32 Value)  { return FString::Printf(TEXT("%d"), Value); }
	inline FString ToStringInternal(uint32 Value) { return FString::Printf(TEXT("%u"), Value); }
	inline FString ToStringInternal(int64 Value)  { return FString::Printf(TEXT("%lld"), Value); }
	inline FString ToStringInternal(uint64 Value) { return FString::Printf(TEXT("%llu"), Value); }

	template <typename EnumType>
	inline FString ToStringInternal(EnumType Enum, typename TEnableIf<TIsEnum<EnumType>::Value>::Type* = 0)
	{
		return StaticEnum<EnumType>()->GetNameStringByValue((int64)Enum);
	}

	// ==== Surcouches pour smart pointers Unreal ====
	template<typename U>
	inline FString ToStringInternal(const TObjectPtr<U>& Ptr)
	{
		U* Raw = Ptr.Get();
		return Raw
			? FString::Printf(TEXT("Object(%s)"), *Raw->GetName())
			: TEXT("null");
	}

	template<typename U>
	inline FString ToStringInternal(const TWeakObjectPtr<U>& Ptr)
	{
		U* Raw = Ptr.Get();
		return Raw
			? FString::Printf(TEXT("Object(%s)"), *Raw->GetName())
			: TEXT("null");
	}

	// Pointeurs bruts
	template <typename T>
	inline FString ToStringInternal(const T* Ptr)
	{
		return Ptr
			? FString::Printf(TEXT("Object(%s)"), *Ptr->GetName())
			: TEXT("null");
	}

	// FVector, FRotator, FColor, etc.
	inline FString ToStringInternal(const FVector& V)       { return FString::Printf(TEXT("V(%.2f, %.2f, %.2f)"), V.X, V.Y, V.Z); }
	inline FString ToStringInternal(const FRotator& R)      { return FString::Printf(TEXT("Rot(%.2f, %.2f, %.2f)"), R.Pitch, R.Yaw, R.Roll); }
	inline FString ToStringInternal(const FQuat& Q)         { return FString::Printf(TEXT("Quat(%.2f, %.2f, %.2f, %.2f)"), Q.X, Q.Y, Q.Z, Q.W); }
	inline FString ToStringInternal(const FColor& C)        { return FString::Printf(TEXT("Color(%d, %d, %d, %d)"), C.R, C.G, C.B, C.A); }
	inline FString ToStringInternal(const FLinearColor& LC) { return FString::Printf(TEXT("LinCol(%.2f, %.2f, %.2f, %.2f)"), LC.R, LC.G, LC.B, LC.A); }
	inline FString ToStringInternal(const FTransform& T)    { return FString::Printf(TEXT("Transform(Pos = %s, Rot = %s, Scale = %s)"),
																*ToStringInternal(T.GetLocation()), *ToStringInternal(T.GetRotation()),
																*ToStringInternal(T.GetScale3D())); }

	// TOptional
	template<typename T>
	inline FString ToStringInternal(const TOptional<T>& Optional)
	{
		return Optional.IsSet() ? FString::Printf(TEXT("Some(%s)"), *ToStringInternal(Optional.GetValue())) : TEXT("None");
	}

	// TArray
	template<typename T>
	inline FString ToStringInternal(const TArray<T>& Array)
	{
		FString Result = TEXT("[ ");
		
		for (int32 i = 0; i < Array.Num(); ++i)
		{
			Result += ToStringInternal(Array[i]);
			if (i < Array.Num() - 1)
			{
				Result += TEXT(", ");
			}
		}
		
		Result += TEXT(" ]");
		return Result;
	}

	// TMap
	template<typename K, typename V>
	inline FString ToStringInternal(const TMap<K, V>& Map)
	{
		FString Result = TEXT("{ ");
		int32 i = 0;
		
		for (const auto& Elem : Map)
		{
			Result += FString::Printf(TEXT("%s: %s"), *ToStringInternal(Elem.Key), *ToStringInternal(Elem.Value));
			if (i < Map.Num() - 1)
			{
				Result += TEXT(", ");
			}
			i++;
		}
		
		Result += TEXT(" }");
		return Result;
	}

	// FormatArgs
	inline void CollectFormatted(FString& Out, const FString& Labels)
	{
		Out += Labels;
	}

	template<typename T, typename... Args>
	void CollectFormatted(FString& Out, const FString& Labels, const T& Value, const Args&... Rest)
	{
		int32 CommaIndex;
		FString Label = Labels;
		FString RemainingLabels;

		if (Labels.FindChar(',', CommaIndex))
		{
			Label = Labels.Left(CommaIndex).TrimStartAndEnd();
			RemainingLabels = Labels.Mid(CommaIndex + 1);
		}
		else
		{
			RemainingLabels = TEXT("");
		}

		Out += FString::Printf(TEXT("%s = %s"), *Label, *ToStringInternal(Value));

		if constexpr (sizeof...(Rest) > 0)
		{
			Out += TEXT(", ");
			CollectFormatted(Out, RemainingLabels, Rest...);
		}
	}

	template<typename... Args>
	FString FormatArgs(const FString& Labels, const Args&... Values)
	{
		FString Result;
		CollectFormatted(Result, Labels, Values...);
		return Result;
	}
	
}


// ====== Macros ======


#define SMARTLOG_PREFIX() \
([]() -> const TCHAR* { \
	if (!GWorld) return TEXT(""); \
	switch (GWorld->GetNetMode()) \
	{ \
	case NM_Client: return TEXT("[Client] "); \
	case NM_DedicatedServer: \
	case NM_ListenServer: return TEXT("[Server] "); \
	default: return TEXT(""); } \
})()

#define LOGINFO(Pattern, ...) \
{ \
	const FString Formatted = SmartLogDetail::FormatArgs(Pattern, ##__VA_ARGS__); \
	UE_LOG(LogSmart, Log, TEXT("%s %s:%d: %s"), SMARTLOG_PREFIX(), TEXT(__FUNCTION__), __LINE__, *Formatted ); \
}

#define LOGWARNING(Pattern, ...) \
{ \
	const FString Formatted = SmartLogDetail::FormatArgs(Pattern, ##__VA_ARGS__); \
	UE_LOG(LogSmart, Warning, TEXT("%s %s:%d: %s"), SMARTLOG_PREFIX(), TEXT(__FUNCTION__), __LINE__, *Formatted ); \
}

#define LOGERROR(Pattern, ...) \
{ \
	const FString Formatted = SmartLogDetail::FormatArgs(Pattern, ##__VA_ARGS__); \
	UE_LOG(LogSmart, Error, TEXT("%s %s:%d: %s"), SMARTLOG_PREFIX(), TEXT(__FUNCTION__), __LINE__, *Formatted ); \
}

#define LOGVARS(...) \
{ \
	const FString Formatted = SmartLogDetail::FormatArgs(TEXT(#__VA_ARGS__), __VA_ARGS__); \
	UE_LOG(LogSmart, Log, TEXT("%s %s:%d: %s"), SMARTLOG_PREFIX(), TEXT(__FUNCTION__), __LINE__, *Formatted ); \
}



// ====== Basic Screen log ======

inline void ScreenLog(const FString& Message, float Duration = 5.f, FColor Color = FColor::White)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message);
}