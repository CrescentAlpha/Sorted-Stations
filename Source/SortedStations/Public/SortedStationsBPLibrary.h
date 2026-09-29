#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FGActorRepresentation.h"
#include "SortedStationsBPLibrary.generated.h"

class UFGActorRepresentation;

UCLASS()
class SORTEDSTATIONS_API USortedStationsBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 対象がModのサポートする表現タイプ（車両、トラックステーション、駅、列車、ドローンポート、ドローン）か判定 */
	UFUNCTION(BlueprintPure, Category = "Sorted Stations")
	static bool IsSupportedRepresentationType(ERepresentationType Type);

	/** 対象アクターのバニラ公式表示名（mDisplayName）を取得 */
	UFUNCTION(BlueprintPure, Category = "Sorted Stations")
	static FText GetVanillaDisplayName(const UFGActorRepresentation* Representation);

	/** 対象アクターの生の名前（駅名や列車名など、プレフィックスなし）を取得 */
	UFUNCTION(BlueprintPure, Category = "Sorted Stations")
	static FText GetRawActorName(const UFGActorRepresentation* Representation);

	/** 実体アクターから正式な IFGActorRepresentationInterface を解決・取得 */
	static class IFGActorRepresentationInterface* ResolveRepresentationInterface(AActor* RealActor);

	/**
	 * マップマーカー表示の即時自動再ソートを実行
	 */
	UFUNCTION(BlueprintCallable, Category = "Sorted Stations", meta = (WorldContext = "WorldContextObject"))
	static void RequestImmediateMapSort(UObject* WorldContextObject);

	/**
	 * 車両（Wheeled Vehicle）の名前を変更し、即座にマップを再ソート
	 */
	UFUNCTION(BlueprintCallable, Category = "Sorted Stations", meta = (WorldContext = "WorldContextObject"))
	static bool RenameVehicle(UObject* WorldContextObject, UFGActorRepresentation* Representation, const FText& NewName);

	/** 車両のマップアイコンカラーを設定・保存し、即座にマップを更新 */
	UFUNCTION(BlueprintCallable, Category = "Sorted Stations|Vehicle", meta = (WorldContext = "WorldContextObject"))
	static bool SetVehicleColor(UObject* WorldContextObject, UFGActorRepresentation* Representation, FLinearColor NewColor);

	/** 車両の現在のマップアイコンカラーを取得（未設定ならバニラのデフォルト色を返す） */
	UFUNCTION(BlueprintPure, Category = "Sorted Stations|Vehicle", meta = (WorldContext = "WorldContextObject"))
	static FLinearColor GetVehicleColor(UObject* WorldContextObject, UFGActorRepresentation* Representation);

	/** JSON設定ファイルに保存されたカスタム色が存在するか判定し、存在すれば取得 */
	UFUNCTION(BlueprintPure, Category = "Sorted Stations|Vehicle")
	static bool TryGetSavedVehicleColor(const UFGActorRepresentation* Representation, FLinearColor& OutColor);

	/** 指定された表現アクターの保存カラーを削除してバニラデフォルトに復元 */
	UFUNCTION(BlueprintCallable, Category = "Sorted Stations", meta = (WorldContext = "WorldContextObject"))
	static void ClearVehicleColor(UObject* WorldContextObject, UFGActorRepresentation* Representation);

	/**
	 * Mod全体の設定データ（カスタムタグ、車両色）を初期デフォルト値で上書きリセット
	 * 物理削除は行わず、安全に初期値で上書き保存します
	 */
	UFUNCTION(BlueprintCallable, Category = "Sorted Stations")
	static void ResetAllModSettings();

	/** 白シルエットとの視認性を確保するため、明色を適切なコントラストに自動補正 */
	UFUNCTION(BlueprintPure, Category = "Sorted Stations|Vehicle")
	static FLinearColor AdjustColorForContrast(const FLinearColor& InColor);

	/** 登録済みカスタムタグ一覧を取得（初回はデフォルトタグ [IN], [OUT], [RAW], [PROD], [HUB] を返す） */
	UFUNCTION(BlueprintCallable, Category = "Sorted Stations|Tags")
	static TArray<FString> GetCustomTags();

	/** 新しいカスタムタグを追加して保存 */
	UFUNCTION(BlueprintCallable, Category = "Sorted Stations|Tags")
	static void AddCustomTag(const FString& NewTag);

	/** 既存のカスタムタグ名を変更して保存 */
	UFUNCTION(BlueprintCallable, Category = "Sorted Stations|Tags")
	static void UpdateCustomTag(int32 Index, const FString& NewTag);

	/** 指定インデックスのカスタムタグを削除して保存 */
	UFUNCTION(BlueprintCallable, Category = "Sorted Stations|Tags")
	static void RemoveCustomTag(int32 Index);

	/** バニラ公式の多言語StringTableからテキストを取得するヘルパー */
	UFUNCTION(BlueprintPure, Category = "Sorted Stations|Localization")
	static FText GetVanillaString(FName TableId, const FString& Key, const FText& Fallback = FText::GetEmpty());

	/** ソートモードに応じた文字列比較（1: Unicode順, 2: Windows自然順） */
	UFUNCTION(BlueprintPure, Category = "Sorted Stations")
	static bool CompareStrings(const FString& A, const FString& B, int32 Mode = 1);

	/** 現在のワールド内の全車両表現の色をデフォルトに即時リセット */
	UFUNCTION(BlueprintCallable, Category = "Sorted Stations")
	static void ResetAllVehicleColorsInWorld(UObject* WorldContextObject);
};
