#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMesh.h"
#include "Engine/EngineTypes.h"
#include "Engine/StaticMeshActor.h"
#include "Randarmor.generated.h"


UCLASS()
class MYPROJECT_API ARandarmor : public AActor
{
    GENERATED_BODY()

public:
    ARandarmor();



    int32 Selectednumber;
    UPROPERTY()
    TArray<AStaticMeshActor*> SpawnedArmors;

   
    UFUNCTION(BlueprintCallable, Category = "DataGeneration")
    void GenerateScene();

  
    UFUNCTION(BlueprintCallable, Category = "DataGeneration")
    void ClearScene();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category = "Config")
    TArray<UStaticMesh*> ArmorMeshes;

    
    UPROPERTY(EditAnywhere, Category = "Config", meta = (ToolTip = "X=最小数量, Y=最大数量"))
    FIntPoint SpawnCountRange = FIntPoint(1, 8);

 
    UPROPERTY(EditAnywhere, Category = "Config")
    FVector2D SpawnDistanceRange = FVector2D(100.0f, 500.0f);

   
    UPROPERTY(EditAnywhere, Category = "Config")
    float SpawnConeHalfAngle = 40.0f;

    
    UPROPERTY(EditAnywhere, Category = "Config")
    float OverlapCheckRadius = 25.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* BackgroundPlane;

    // 存放背景材质的列表（在编辑器里把图片生成的材质拖进去）
    UPROPERTY(EditAnywhere, Category = "Config")
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
    
};
