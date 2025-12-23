#include "SingleArmor.h"
#include "YoloDatasetGenerate.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Engine/EngineTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "HAL/FileManager.h"



    // 构造函数，初始化背景板
    ASingleArmor::ASingleArmor()
    {
        PrimaryActorTick.bCanEverTick = false;

        // 1. 创建背景板组件
        BackgroundPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackgroundPlane"));
        BackgroundPlane->SetupAttachment(RootComponent);

        // 2. 查找引擎自带的平面模型 (Plane)
        static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(TEXT("/Engine/BasicShapes/Plane"));
        if (PlaneMeshAsset.Succeeded())
        {
            BackgroundPlane->SetStaticMesh(PlaneMeshAsset.Object);
        }

        // 3. 关闭碰撞
        BackgroundPlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BackgroundPlane->SetCastShadow(false); // 背景板通常不需要投射阴影，看起来更像贴图
    }

    // 运行开始的初始化
    void ASingleArmor::BeginPlay()
    {
        // 启动GenerateScene
        Super::BeginPlay();

        //找到数据集的全局坐标
        GlobalIndex = FindCurrentMaxIndex();
        UE_LOG(LogTemp, Warning, TEXT("Dataset Generation Started. Next Index: %d"), GlobalIndex);
    }

    //核心函数
    void ASingleArmor::GenerateScene()
    {
        //换背景，清除上张图的数据
        UpdateBackground();
        if (ThemePresets.Num() == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("Add Theme Presets in Editor!"));
        }
        int32 RandomTheme = FMath::RandRange(0, ThemePresets.Num() - 1);
        ApplyTheme(RandomTheme);
        ClearScene();

        //初始检查
        UWorld* World = GetWorld();
        if (!World) return;

        if (ArmorMeshes.Num() == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("No Armor Meshes assigned in the generator!"));
            return;
        }

        APlayerController* MyPC = UGameplayStatics::GetPlayerController(this, 0);
        if (MyPC)
        {
            AActor* ViewTarget = MyPC->GetViewTarget();  //返回看世界的Target
            if (ViewTarget)
            {
                //从Actor身上找到相机组件
                TargetCameraComp = ViewTarget->FindComponentByClass<UCameraComponent>();
            }
        }
        APlayerCameraManager* CamManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
        if (!CamManager) return;



        FVector CamLoc = CamManager->GetCameraLocation();
        FVector CamFwd = CamManager->GetActorForwardVector();
        FRotator CameraRot = CamManager->GetCameraRotation();

        int32 SuccessCount = 0;
        int32 RandomTargetCount = FMath::RandRange(SpawnCountRange.X, SpawnCountRange.Y);
        int32 MaxAttempts = RandomTargetCount * 10;
        int32 Attempts = 0;

        //生成装甲板
        while (SuccessCount < RandomTargetCount && Attempts < MaxAttempts)
        {
            Attempts++;

            //1. 随机生成位置 (四棱锥范围)
            float RandomDist = FMath::RandRange(SpawnDistanceRange.X, SpawnDistanceRange.Y);
            FVector RandomDir = CamFwd;
            FVector SpawnLoc = CamLoc + (RandomDir * RandomDist);

            //2. 随机生成旋转 
            FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(SpawnLoc, CamLoc);
            float RandomOffset = FMath::RandRange(-45.0f, 45.0f);
            float RandomPitch = FMath::RandRange(0.0f, 15.0f);
            float FinalYaw = RandomOffset + LookAtRot.Yaw + 90.0f;
            FRotator SpawnRot = FRotator(0.0f, FinalYaw, 15.0f);

            Selectednumber = FMath::RandRange(0, ArmorMeshes.Num() - 1);
            Selectedscale = FMath::RandRange(ScaleRange.X, ScaleRange.Y);
            float CurrentCheckRadius = OverlapCheckRadius * Selectedscale;

            //检测 

             // 3.  背对检测：如果背面朝向摄像头，直接跳过
            if (IsBackFacing(SpawnLoc, SpawnRot, CamLoc))
            {
                continue;
            }

            // 4 . 空间占用检测：检查该位置是否已经有物体 (包含场景静态物体和其他装甲板)
            if (IsSpaceOccupied(SpawnLoc, CurrentCheckRadius))
            {
                continue;
            }

            // 生成实体 

            // 随机选一个 Mesh


            UStaticMesh* SelectedMesh = ArmorMeshes[Selectednumber];
            
            FTransform SpawnTransform(SpawnRot, SpawnLoc, FVector(Selectedscale));

            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            // 生成 StaticMeshActor
            AStaticMeshActor* NewArmor = World->SpawnActor<AStaticMeshActor>(
                AStaticMeshActor::StaticClass(),
                SpawnTransform,
                SpawnParams
            );

            if (NewArmor)
            {
                // 配置 StaticMeshActor
                UStaticMeshComponent* MeshComp = NewArmor->GetStaticMeshComponent();
                if (MeshComp)
                {
                    MeshComp->SetMobility(EComponentMobility::Movable); // 必须是可移动的
                    MeshComp->SetStaticMesh(SelectedMesh);

                    MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // 开启碰撞以便射线检测

                }



                //解决阻挡问题    
                if (IsOccluded(SpawnLoc, CamLoc, NewArmor))
                {
                    NewArmor->Destroy();
                    continue;
                }

                if (!IsFullyInView(NewArmor))
                {

                    NewArmor->Destroy();
                    continue;
                }


                SpawnedArmors.Add(NewArmor);
                Labelnumber.Add(Selectednumber);
                SuccessCount++;
            }
        }



        UE_LOG(LogTemp, Log, TEXT("Generation Complete. Attempts: %d, Success: %d"), Attempts, SuccessCount);

        if (SuccessCount == 0) return;


        for (int i = 0; i < SpawnedArmors.Num(); i++)
        {
            UStaticMeshComponent* MeshComp = SpawnedArmors[i]->GetStaticMeshComponent();

            {

                LabelData += FString::Printf(TEXT("%d \n"),
                    Labelnumber[i]);
            }

        }

        GetWorld()->GetTimerManager().SetTimer(
            TimerHandle_Save,
            this,
            &ASingleArmor::ExecuteSave,
            0.2f, // 延迟 0.2秒
            false
        );

    }

    void ASingleArmor::ExecuteSave()
    {
        UYoloDatasetGenerate::SaveDatasetEntry_single(GetWorld(), GlobalIndex, LabelData);
        GlobalIndex++;
    }

    void ASingleArmor::ClearScene()
    {
        for (AActor* Actor : SpawnedArmors)
        {
            if (Actor && IsValid(Actor))
            {
                Actor->Destroy();
            }
        }
        SpawnedArmors.Empty();
        Labelnumber.Empty();
    }

    bool ASingleArmor::IsBackFacing(const FVector& ArmorLocation, const FRotator& ArmorRotation, const FVector& CameraLocation)
    {
        // 计算：摄像头 -> 装甲板 的方向向量
        FVector CamToArmor = (ArmorLocation - CameraLocation).GetSafeNormal();

        // 获取装甲板的正面方向
        FVector ArmorForward = -FRotationMatrix(ArmorRotation).GetUnitAxis(EAxis::Y);

        return FVector::DotProduct(CamToArmor, ArmorForward) > 0.0f;
    }

    bool ASingleArmor::IsSpaceOccupied(const FVector& Location, float Radius)
    {
        UWorld* World = GetWorld();
        if (!World) return false;

        //以装甲板中心位置生成球体检测碰撞
        FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

        bool bHitDynamic = World->OverlapAnyTestByChannel(
            Location,
            FQuat::Identity,
            ECC_WorldDynamic, // 检测动态物体
            Sphere
        );

        bool bHitStatic = World->OverlapAnyTestByChannel(
            Location,
            FQuat::Identity,
            ECC_WorldStatic, // 检测静态物体
            Sphere
        );

        return bHitDynamic || bHitStatic;
    }

    bool ASingleArmor::IsOccluded(const FVector& ArmorLocation, const FVector& CameraLocation, const AActor* CurrentArmor)
    {
        FHitResult HitResult;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(CurrentArmor);

        bool bHit = GetWorld()->LineTraceSingleByChannel(
            HitResult,
            CameraLocation,
            ArmorLocation,
            ECC_Visibility, // 使用可见性通道
            QueryParams
        );

        return bHit;
    }


    bool ASingleArmor::IsFullyInView(const AActor* TargetActor)
    {
        if (!TargetActor) return false;

        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (!PC) return false;

        // 1. 获取屏幕分辨率
        int32 ScreenX, ScreenY;
        PC->GetViewportSize(ScreenX, ScreenY);

        // 2. 获取物体的包围盒 (Origin:中心点, Extent:长宽高的一半)
        FVector Origin, Extent;
        TargetActor->GetActorBounds(false, Origin, Extent);

        // 3. 定义包围盒的 8 个顶点的相对方向
        // (比如 +X+Y+Z, +X+Y-Z, -X-Y-Z 等等)
        TArray<FVector> Corners;
        Corners.Add(Origin + FVector(Extent.X, Extent.Y, Extent.Z));
        Corners.Add(Origin + FVector(Extent.X, Extent.Y, -Extent.Z));
        Corners.Add(Origin + FVector(Extent.X, -Extent.Y, Extent.Z));
        Corners.Add(Origin + FVector(Extent.X, -Extent.Y, -Extent.Z));
        Corners.Add(Origin + FVector(-Extent.X, Extent.Y, Extent.Z));
        Corners.Add(Origin + FVector(-Extent.X, Extent.Y, -Extent.Z));
        Corners.Add(Origin + FVector(-Extent.X, -Extent.Y, Extent.Z));
        Corners.Add(Origin + FVector(-Extent.X, -Extent.Y, -Extent.Z));

        // 4. 遍历这 8 个点，只要有一个点出界，就返回 false
        for (const FVector& Corner : Corners)
        {
            FVector2D ScreenPos;
            // ProjectWorldToScreen 将 3D 坐标转为 2D 屏幕坐标
            // 返回 false 表示该点在摄像机背后，必然看不见
            bool bOnScreen = UGameplayStatics::ProjectWorldToScreen(PC, Corner, ScreenPos);

            if (!bOnScreen)
            {
                return false;
            }

            // 检查坐标是否超出屏幕分辨率范围
            if (ScreenPos.X < 0 || ScreenPos.X > ScreenX ||
                ScreenPos.Y < 0 || ScreenPos.Y > ScreenY)
            {
                return false;
            }
        }
        return true;
    }

    void ASingleArmor::UpdateBackground()
    {
        APlayerCameraManager* CamManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
        if (!CamManager) return;

        FVector CamLoc = CamManager->GetCameraLocation();
        FVector CamFwd = CamManager->GetActorForwardVector();

        // 1. 计算背景板位置
        // 必须比你设定的最远生成距离 (SpawnDistanceRange.Y) 还要远
        // 比如加 2000cm (20米)，防止装甲板插进背景里
        float BgDistance = SpawnDistanceRange.Y + 2000.0f;

        FVector BgPos = CamLoc + (CamFwd * BgDistance);
        float HalfAngleRad = FMath::DegreesToRadians(40.0f);
        float WorldWidth = 2.0f * BgDistance * FMath::Tan(HalfAngleRad);
        float WorldHeight = WorldWidth ;


        FRotator BgRot = FRotationMatrix::MakeFromX(-CamFwd).Rotator();
        BackgroundPlane->SetWorldRotation(BgRot);
        BackgroundPlane->AddLocalRotation(FRotator(90.0f, 0.0f, 0.0f));
        BackgroundPlane->AddLocalRotation(FRotator(0.0f, 90.0f, 0.0f));

        BackgroundPlane->SetWorldLocation(BgPos);

        // 3. 设置大小（严格缩放使其刚好撑满视野范围，保证背景图不会变形）
        FVector TargetScale = FVector(WorldWidth / 100.0f, WorldHeight / 100.0f, 1.0f);
        BackgroundPlane->SetWorldScale3D(TargetScale);

        // 4. 随机换材质
        if (BackgroundMaterials.Num() > 0)
        {
            int32 RandomIndex = FMath::RandRange(0, BackgroundMaterials.Num() - 1);
            UMaterialInterface* BaseMat = BackgroundMaterials[RandomIndex];
            if (BaseMat)
            {
                UMaterialInstanceDynamic* DMI = UMaterialInstanceDynamic::Create(BaseMat, this);

                if (DMI)
                {
                    BackgroundPlane->SetMaterial(0, DMI);
                }
                else
                {
                    BackgroundPlane->SetMaterial(0, BaseMat);
                }
            }

        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("No background materials"));
        }
    }

    // 相机拍摄平面的为4:3(适应1440*1080)；
    FVector ASingleArmor::GetPlaneUniformRandomDir(FRotator CamRotation, float HorizontalHalfAngleDeg, float AspectRatio)
    {
        float RadH = FMath::DegreesToRadians(HorizontalHalfAngleDeg);

        float MaxTanH = FMath::Tan(RadH);
        float MaxTanV = MaxTanH / AspectRatio;

        float RandTanY = FMath::FRandRange(-1.0f, 1.0f) * MaxTanH;
        float RandTanZ = FMath::FRandRange(-1.0f, 1.0f) * MaxTanV;

        FVector LocalDir = FVector(1.0f, RandTanY, RandTanZ);
        LocalDir.Normalize();

        FQuat RotationQuat = FQuat(CamRotation);
        FVector WorldDir = RotationQuat.RotateVector(LocalDir);
        return WorldDir;
    }

    void ASingleArmor::ApplyTheme(int32 ThemeIndex)
    {
        if (!TargetCameraComp) return;

        const FSceneTheme_single& SelectedTheme = ThemePresets[ThemeIndex];

        //随机因子扰动
        float R_Offset = FMath::RandRange(-0.1f, 0.1f);
        float G_Offset = FMath::RandRange(-0.1f, 0.1f);
        float B_Offset = FMath::RandRange(-0.1f, 0.1f);

        // 应用扰动到 Gamma 颜色 (Global Color Grading)
        FLinearColor FinalGamma = SelectedTheme.LightColor + FLinearColor(R_Offset, G_Offset, B_Offset, 0.0f);

        // 应用扰动到背景颜色 (背景可以变化大一点)
        FLinearColor FinalBgColor = SelectedTheme.BgColor + FLinearColor(R_Offset * 0.5f, G_Offset * 0.5f, B_Offset * 0.5f, 0.0f);

        // --- 3. 设置相机后期处理 (Post Process) ---

        // 开启 Gamma (颜色分级) 覆盖
        TargetCameraComp->PostProcessSettings.bOverride_ColorGamma = true;
        TargetCameraComp->PostProcessSettings.ColorGamma = FVector4(FinalGamma.R, FinalGamma.G, FinalGamma.B, 1.0f);

        //改饱和度
        // TargetCameraComp->PostProcessSettings.bOverride_ColorSaturation = true;
        // TargetCameraComp->PostProcessSettings.ColorSaturation = FVector4(1.2f, 1.2f, 1.2f, 1.0f); 

        // 设置曝光 (在预设范围内随机)
        TargetCameraComp->PostProcessSettings.bOverride_AutoExposureMethod = true;
        TargetCameraComp->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
        TargetCameraComp->PostProcessSettings.bOverride_AutoExposureBias = true;
        TargetCameraComp->PostProcessSettings.AutoExposureBias = FMath::RandRange(SelectedTheme.MinExposure, SelectedTheme.MaxExposure);

        // --- 4. 设置背景板材质颜色 ---
        if (BackgroundPlane)
        {
            // 获取当前的动态材质 (假设你在 UpdateBackground 里已经创建了 DMI)
            UMaterialInstanceDynamic* DMI = Cast<UMaterialInstanceDynamic>(BackgroundPlane->GetMaterial(0));
            if (DMI)
            {
                // 设置我们在材质里预留的 "TintColor" 参数
                DMI->SetVectorParameterValue(FName("ColorTint"), FinalBgColor);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("ApplyTheme: Background material is not Dynamic!"));
            }
        }
    }

    //全局索引
    int32 ASingleArmor::FindCurrentMaxIndex()
    {
        FString SearchDir = FPaths::ProjectSavedDir() / TEXT("YoloDataset/train_single/images");
        IFileManager& FileManager = IFileManager::Get();

        if (!FileManager.DirectoryExists(*SearchDir))  return 0;

        TArray<FString> FoundFiles;

        FileManager.FindFiles(FoundFiles, *(SearchDir / TEXT("*.png")), true, false);

        if (FoundFiles.Num() == 0)
        {
            return 0;
        }

        int32 MaxIndex = -1;

        for (const FString& File : FoundFiles)
        {
            FString NameOnly = FPaths::GetBaseFilename(File);

            if (NameOnly.IsNumeric())
            {
                int32 FileIndex = FCString::Atoi(*NameOnly);
                if (FileIndex > MaxIndex)
                {
                    MaxIndex = FileIndex;
                }
            }
        }
        return MaxIndex + 1;
    }


