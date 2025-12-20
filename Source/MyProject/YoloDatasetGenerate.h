// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YoloDatasetGenerate.generated.h"

// 前置声明
class UStaticMeshComponent;
class APlayerController;

// 1. 将结构体搬到这里，并加上 USTRUCT 宏，使其成为 UE 类型
USTRUCT(BlueprintType)
struct FYoloData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 ClassID = 0;

    UPROPERTY(BlueprintReadOnly)
    float CenterX = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float CenterY = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float Width = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float Height = 0.f;

    UPROPERTY(BlueprintReadOnly)
    TArray<FVector2D> Sockets;

    UPROPERTY(BlueprintReadOnly)
    bool bIsValid = false;
};


UCLASS()
class MYPROJECT_API UYoloDatasetGenerate : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** 计算 YOLO 坐标数据 */
    static FYoloData CalculateYoloFromSockets(UStaticMeshComponent* MeshComp, APlayerController* PC);

    /** * 保存单条数据（截图 + txt）
     * 注意：因为是 static 函数，无法直接调用 GetWorld()，所以需要传入 World 或者 ViewportClient
     */
    static void SaveDatasetEntry(UWorld* WorldContext, int32 Index, const FString& LabelString);

private:
    // 辅助函数：确保文件夹存在 
    static void EnsureDirectoriesExist(FString BasePath);
    static void EnsureDirectoriesExist_Single(FString BasePath);
};
