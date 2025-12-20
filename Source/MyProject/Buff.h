#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMesh.h"
#include "Engine/EngineTypes.h"
#include "Engine/StaticMeshActor.h"
#include "Camera/CameraComponent.h"
#include "Randarmor.generated.h"


USTRUCT(BlueprintType)
struct FSceneTheme_buff
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor LightColor = FLinearColor::White;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor BgColor = FLinearColor::Black;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinExposure = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxExposure = 12.0f;
};

UCLASS()
class MYPROJECT_API ABuff : public AActor
{
    GENERATED_BODY()

public:
    ABuff();

    int32 Selectednumber;
    float Selectedscale;

    UPROPERTY()
    TArray<AStaticMeshActor*> SpawnedArmors;



    UFUNCTION(BlueprintCallable, Category = "DataGeneration")
    void GenerateScene();


    UFUNCTION(BlueprintCallable, Category = "DataGeneration")
    void ClearScene();

    UFUNCTION(BlueprintCallable, Category = "CVSyntheticData")
    FVector GetPlaneUniformRandomDir(FRotator CamRotation, float HorzontalHalfAngleDeg, float AspectRatio);

protected:
    virtual void BeginPlay() override;
    UPROPERTY()
    UCameraComponent* TargetCameraComp;

    UPROPERTY(EditAnywhere, Category = "Config|Themes")
    TArray<FSceneTheme> ThemePresets;

    UPROPERTY(EditAnywhere, Category = "Config")
    TArray<UStaticMesh*> ArmorMeshes;


    UPROPERTY(EditAnywhere, Category = "Config", meta = (ToolTip = "X=最小数量, Y=最大数量"))
    FIntPoint SpawnCountRange = FIntPoint(1, 5);


    UPROPERTY(EditAnywhere, Category = "Config")
    FVector2D SpawnDistanceRange = FVector2D(100.0f, 100.0f);

    UPROPERTY(EditAnywhere, Category = "Config")
    float TargetAspectRatio = 1.333333f;


    UPROPERTY(EditAnywhere, Category = "Config")
    float SpawnHalfAngle = 40.0f;

    UPROPERTY(EditAnywhere, Category = "Config")
    FVector2D ScaleRange = FVector2D(0.5f, 2.0f);

    UPROPERTY(EditAnywhere, Category = "Config")
    float OverlapCheckRadius = 25.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* BackgroundPlane;

    // 存放背景材质的列表（在编辑器里把图片生成的材质拖进去）
    UPROPERTY(EditAnywhere, Category = "Config|BgMaterials")
    TArray<UMaterialInterface*> BackgroundMaterials;

    // 辅助函数：更新背景
    void UpdateBackground();

private:
    TArray<int32> Labelnumber;
    int32 GlobalIndex = 0;
    int32 FindCurrentMaxIndex();
    bool IsBackFacing(const FVector& ArmorLocation, const FRotator& ArmorRotation, const FVector& CameraLocation);
    bool IsOccluded(const FVector& ArmorLocation, const FVector& CameraLocation, const AActor* CurrentArmor);
    bool IsFullyInView(const AActor* TargetActor);
    bool IsSpaceOccupied(const FVector& Location, float Radius);
    FTimerHandle TimerHandle_Save;
    void ExecuteSave();
    FString LabelData;

    void ApplyTheme(int32 ThemeIndex);

};