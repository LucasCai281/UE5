// Fill out your copyright notice in the Description page of Project Settings.
#include "YoloDatasetGenerate.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "ImageUtils.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"

// 1. 辅助函数实现
void UYoloDatasetGenerate::EnsureDirectoriesExist(FString BasePath)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*(BasePath / TEXT("images"))))
    {
        PlatformFile.CreateDirectoryTree(*(BasePath / TEXT("images")));
    }
    if (!PlatformFile.DirectoryExists(*(BasePath / TEXT("labels"))))
    {
        PlatformFile.CreateDirectoryTree(*(BasePath / TEXT("labels")));
    }
}

// 2. 计算函数实现
FYoloData UYoloDatasetGenerate::CalculateYoloFromSockets(UStaticMeshComponent* MeshComp, APlayerController* PC)
{
    FYoloData Result;
    Result.ClassID = 0;
    Result.Sockets.Init(FVector2D::ZeroVector, 4);

    if (!MeshComp || !PC) return Result;

    //获取屏幕长宽
    int32 ScreenWidth = 0, ScreenHeight = 0;
    PC->GetViewportSize(ScreenWidth, ScreenHeight);

    if (ScreenWidth == 0 || ScreenHeight == 0) return Result;

    float MinX = 0.0f;
    float MinY = 0.0f;
    float MaxX = 0.0f;
    float MaxY = 0.0f;

    for (int32 i = 0; i < 4; i++)
    {
        FName SocketName = FName(*FString::Printf(TEXT("Socket_%d"), i));
        if (!MeshComp->DoesSocketExist(SocketName)) continue;

        FVector WorldLoc = MeshComp->GetSocketLocation(SocketName);
        FVector2D ScreenPos;

        if(UGameplayStatics::ProjectWorldToScreen(PC, WorldLoc, ScreenPos))
        {
            
            Result.Sockets[i] = ScreenPos;
        }
    }

    MinX = Result.Sockets[0].X < Result.Sockets[1].X ? Result.Sockets[0].X : Result.Sockets[1].X;
    MinY = Result.Sockets[0].Y < Result.Sockets[3].Y ? Result.Sockets[0].Y : Result.Sockets[3].Y;
    MaxX = Result.Sockets[2].X < Result.Sockets[3].X ? Result.Sockets[3].X : Result.Sockets[2].X;
    MaxY = Result.Sockets[1].Y < Result.Sockets[2].Y ? Result.Sockets[2].Y : Result.Sockets[1].Y;


    float BoxW = MaxX - MinX;
    float BoxH = MaxY - MinY;
    float BoxCenterX = MinX + BoxW / 2.0f;
    float BoxCenterY = MinY + BoxH / 2.0f;

    //归一化
    Result.CenterX = BoxCenterX / (float)ScreenWidth;
    Result.CenterY = BoxCenterY / (float)ScreenHeight;
    Result.Width = BoxW / (float)ScreenWidth;
    Result.Height = BoxH / (float)ScreenHeight;
    Result.bIsValid = true;
    for (int i = 0; i < 4; ++i)
    {
        Result.Sockets[i].X /= (float)ScreenWidth;
        Result.Sockets[i].Y /= (float)ScreenHeight;
    }

    return Result;
}

// 3. 保存函数实现 (注意参数里的 WorldContext)
void UYoloDatasetGenerate::SaveDatasetEntry(UWorld* WorldContext, int32 Index, const FString& LabelString)
{
    if (!WorldContext) return;

    FString BaseDir = FPaths::ProjectSavedDir() / TEXT("YoloDataset/train");
    EnsureDirectoriesExist(BaseDir);

    FString FileName = FString::Printf(TEXT("%06d"), Index);
    FString ImgFilename = BaseDir / TEXT("images") / (FileName + TEXT(".png"));
    FString TxtFilename = BaseDir / TEXT("labels") / (FileName + TEXT(".txt"));

    // 保存 TXT
    FFileHelper::SaveStringToFile(LabelString, *TxtFilename);

    // 保存 PNG
    UGameViewportClient* ViewportClient = WorldContext->GetGameViewport();
    if (!ViewportClient) return;

    FViewport* Viewport = ViewportClient->Viewport;
    if (!Viewport) return;

    TArray<FColor> Bitmap;
    Viewport->ReadPixels(Bitmap); // 这是一个耗时操作

    for (FColor& Pixel : Bitmap) Pixel.A = 255;

    int32 Width = Viewport->GetSizeXY().X;
    int32 Height = Viewport->GetSizeXY().Y;

    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

    if (ImageWrapper.IsValid())
    {
        if (ImageWrapper->SetRaw(Bitmap.GetData(), Bitmap.Num() * sizeof(FColor), Width, Height, ERGBFormat::BGRA, 8))
        {
            const TArray64<uint8>& CompressedData = ImageWrapper->GetCompressed();
            FFileHelper::SaveArrayToFile(CompressedData, *ImgFilename);
        }
    }
}