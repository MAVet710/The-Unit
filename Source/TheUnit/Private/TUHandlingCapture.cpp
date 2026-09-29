#include "TUHandlingCapture.h"
#include "TUWeaponClearance.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "TUWeaponPartsComponent.h"
#include "TUWorldItem.h"
#include "EngineUtils.h"
#include "TU_ArmedOperatorCharacter.h"
#include "TU_WeaponBase.h"
#include "TUWeaponPresentationComponent.h"
#include "TUHandlingSight.h"
#include "TUHandlingAnimInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace
{
const TCHAR* StageNames[] = { TEXT("hip"), TEXT("ads"), TEXT("pitch_up_ads"), TEXT("pitch_down_ads"), TEXT("crouched_ads"), TEXT("lean_left_ads"), TEXT("lean_right_ads"), TEXT("reload"), TEXT("fire"), TEXT("drop_reload"), TEXT("pickup"),TEXT("empty_fire"),TEXT("empty_reload"),TEXT("wall_reload") };
int32 PartsRounds(const FTUItemLedger& L) { int32 N=L.LooseCartridges.Num(); for(const auto& W:L.Weapons) N+=W.ChamberAmmoId.IsNone()?0:1; for(const auto& M:L.Magazines) N+=M.Cartridges.Num(); return N; }
}

ATUHandlingCapture::ATUHandlingCapture()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}
bool ATUHandlingCapture::IsSafeCaptureId(const FString& Value)
{
    if (Value.IsEmpty() || Value.Len() > 96) return false;
    for (TCHAR C : Value) if (!FChar::IsAlnum(C) && C != TCHAR('_') && C != TCHAR('-')) return false;
    return true;
}
bool ATUHandlingCapture::IsSupportedRate(int32 Value) { return Value == 30 || Value == 60 || Value == 144; }

void ATUHandlingCapture::BeginPlay()
{
    Super::BeginPlay();
    StartedAt = FPlatformTime::Seconds();
#if UE_BUILD_SHIPPING
    Destroy();
    return;
#else
    if (!FParse::Value(FCommandLine::Get(), TEXT("TUHandlingCapture="), CaptureId)) { Destroy(); return; }
    if (!IsSafeCaptureId(CaptureId)) CaptureId = TEXT("invalid_capture_id");
    Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/TUHandling"), CaptureId);
    // An id is an immutable evidence run, never overwrite an existing one.
    if (IFileManager::Get().DirectoryExists(*Directory))
    {
        Directory += TEXT("_rejected_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
        IFileManager::Get().MakeDirectory(*Directory, true);
        Finish(false, TEXT("capture id already exists")); return;
    }
    IFileManager::Get().MakeDirectory(*Directory, true);
    FParse::Value(FCommandLine::Get(), TEXT("TUHandlingHz="), Rate);
    bContactReview=FParse::Param(FCommandLine::Get(),TEXT("TUContactReview"));
    bPartsExtended = bContactReview || FParse::Param(FCommandLine::Get(), TEXT("TUWeaponPartsExtended"));
    FString Slot;
    FParse::Value(FCommandLine::Get(), TEXT("TUProfileSlot="), Slot);
    if (CaptureId == TEXT("invalid_capture_id") || !IsSupportedRate(Rate)
        || !Slot.StartsWith(TEXT("TU_Automation_")) || !IsSafeCaptureId(Slot)
        || !FParse::Param(FCommandLine::Get(), TEXT("RenderOffscreen")) || FParse::Param(FCommandLine::Get(), TEXT("nullrhi")))
    { Finish(false, TEXT("requires safe unique capture id isolated profile RenderOffscreen and rate 30 60 or 144")); return; }
    ScreenshotHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(this, &ATUHandlingCapture::OnPixels);
    FApp::SetFixedDeltaTime(1. / Rate);
    FApp::SetUseFixedTimeStep(true);
#endif
}

void ATUHandlingCapture::SetStage(int32 NewStage)
{
    Stage = NewStage;
    StageTime = 0.f;
    Operator->ApplyHandlingCapturePose(Stage > 0 && Stage < 7, Stage == 4,
        Stage == 5 ? -1.f : Stage == 6 ? 1.f : 0.f, Stage == 2 ? 35.f : Stage == 3 ? -35.f : 0.f);
    if(Stage==11) { auto* W=Operator->GetCurrentWeapon();W->SetFireMode(ETUFireMode::FullAuto);W->StartFire(); }
    if(Stage==12) {
        auto* W=Operator->GetCurrentWeapon();W->StopFire();
        if(W->GetCurrentAmmo()!=0) { Finish(false,TEXT("empty reload test failed to exhaust real magazine"));return; }
        if(!W->RequestReload(FGuid::NewGuid(),W->ExportItemLedger().Revision,ETUReloadPolicy::Retain)) { Finish(false,TEXT("empty reload rejected"));return; }
    }
    if(Stage==13) {
        const FTransform Eye=Operator->GetHandlingEyeWorld();
        ContactWall=GetWorld()->SpawnActor<AActor>();
        auto* Box=NewObject<UBoxComponent>(ContactWall);ContactWall->SetRootComponent(Box);
        Box->SetBoxExtent(FVector(4.f,160.f,160.f));Box->SetCollisionProfileName(TEXT("BlockAll"));
        Box->SetMobility(EComponentMobility::Movable);Box->RegisterComponent();
        ContactWall->SetActorLocationAndRotation(Eye.GetLocation()+Eye.GetRotation().GetForwardVector()*70.f,Eye.GetRotation());
        // Let the normal character tick observe the obstruction before issuing reload,
        // mirroring a player who walks up to a wall and then presses reload.
        auto* Visual=NewObject<UStaticMeshComponent>(ContactWall);Visual->SetupAttachment(Box);
        Visual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        Visual->SetRelativeScale3D(FVector(.08f,3.2f,3.2f));Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);Visual->RegisterComponent();
    }
    if (Stage == 8) {
        auto* W=Operator->GetCurrentWeapon(); PartsInitialRounds=PartsRounds(W->ExportItemLedger());
        W->StartFire(); W->StopFire();
        bPartsShotPassed=PartsRounds(W->ExportItemLedger())==PartsInitialRounds-1;
    }
    if (Stage == 9) {
        auto* W=Operator->GetCurrentWeapon();
        bPartsDropRequested=W->RequestReload(FGuid::NewGuid(),W->ExportItemLedger().Revision,ETUReloadPolicy::Drop);
        PartsDropId=W->GetActionState().OutgoingMagazineId;
    }
    if (Stage == 7)
    {
        ATU_WeaponBase* Weapon = Operator->GetCurrentWeapon();
        bReloadRequested = Weapon->RequestReload(FGuid::NewGuid(), Weapon->ExportItemLedger().Revision, ETUReloadPolicy::Retain);
    }
}

void ATUHandlingCapture::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bFinished) return;
    if (FPlatformTime::Seconds() - StartedAt > 180.) { Finish(false, TEXT("capture wall clock timeout")); return; }
    if (!Operator)
    {
        Operator = Cast<ATU_ArmedOperatorCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
        if (!Operator || !Operator->GetCurrentWeapon()) { Operator = nullptr; return; }
        APlayerController* PC = Cast<APlayerController>(Operator->GetController());
        if (!PC) { Operator = nullptr; return; }
        if (PC->PlayerInput) PC->PlayerInput->FlushPressedKeys();
        Operator->DisableInput(PC);
        PC->SetIgnoreMoveInput(true);
        PC->SetIgnoreLookInput(true);
        AddTickPrerequisiteActor(Operator);
        AddTickPrerequisiteComponent(Operator->GetOwnerBodyMesh());
        AddTickPrerequisiteComponent(Operator->GetMesh());
        Operator->ApplyHandlingCapturePose(false, false, 0.f, 0.f);
    }
    Warmup += DeltaSeconds;
    if (Warmup < 2.f) return;
    if (bPending) { Finish(false, TEXT("render did not return previous frame pixels")); return; }
    if (Stage < 0) SetStage(0);
    if (StageTime >= (Stage==7?6.f:(Stage==9 || Stage==11 || Stage==12?4.f:(Stage==13?5.f:1.f))))
    {
        if(Stage==10 && bContactReview) { bContactPickupPassed=VerifyPartsPickup();if(!bContactPickupPassed) { Finish(false,TEXT("pickup precondition failed"));return; } }
        if ((Stage == 7 && !bPartsExtended) || (Stage == 10 && !bContactReview) || Stage==13)
        {
            Finish(!bInvalidMetrics && bReloadRequested && ReloadPhases.Num() >= 3 && EvolvingSamples >= 5 && Images == Frame && (!bPartsExtended || (bContactReview?bContactPickupPassed:VerifyPartsPickup())),
                TEXT("capture complete metrics require external analysis and visual review")); return;
        }
        SetStage(Stage + 1);
    }
    StageTime += DeltaSeconds;
    if(Stage==13 && ContactWall) {
        // Normal movement against a stationary world collider. Once the weapon
        // actually reports obstruction, issue reload through the player's guarded
        // input path so the capture verifies the real policy rather than bypassing it.
        if(StageTime<2.5f) Operator->AddMovementInput(Operator->GetActorForwardVector(),.3f,true);
        if(StageTime>=0.5f && StageTime<0.5f+DeltaSeconds && Operator->IsWeaponClearanceBlocked())
            Operator->TryReloadFromGameplayInput(false);
    }
    APlayerController* PC = Cast<APlayerController>(Operator->GetController());
    ATU_WeaponBase* Weapon = Operator->GetCurrentWeapon();
    UTUWeaponPresentationComponent* Presentation = Operator->GetWeaponPresentation();
    USkeletalMeshComponent* Body = Operator->GetOwnerBodyMesh();
    if (!PC || !PC->PlayerCameraManager || !Weapon || !Presentation || !Body
        || Body->GetBoneIndex(TEXT("hand_r")) == INDEX_NONE || Body->GetBoneIndex(TEXT("hand_l")) == INDEX_NONE)
    { Finish(false, TEXT("missing evaluated body camera weapon or hand bones")); return; }
    const FTransform Right = Body->GetSocketTransform(TEXT("hand_r"), RTS_World);
    const FTransform Left = Body->GetSocketTransform(TEXT("hand_l"), RTS_World);
    const float RightGap = FVector::Distance(Right.GetLocation(), Presentation->GetRightHandGripWorld());
    const float LeftGap = FVector::Distance(Left.GetLocation(), Presentation->GetLeftHandGripWorld());
    const float RightStep = Frame ? FVector::Distance(Right.GetLocation(), PreviousRight) : 0.f;
    const float LeftStep = Frame ? FVector::Distance(Left.GetLocation(), PreviousLeft) : 0.f;
    PreviousRight = Right.GetLocation(); PreviousLeft = Left.GetLocation();
    const float AnimationTime = Presentation->GetEvaluatedAnimationTime();
    const FTUWeaponActionState Action = Weapon->GetActionState();
    if (Stage == 7 && Action.bActive)
    {
        ++ReloadSamples; ReloadPhases.Add(static_cast<int32>(Action.Phase));
        if (PreviousAnimationTime >= 0.f && FMath::Abs(AnimationTime - PreviousAnimationTime) > KINDA_SMALL_NUMBER) ++EvolvingSamples;
    }
    PreviousAnimationTime = AnimationTime;
    FTUHandlingSightProfile Profile;
    const bool bSight = TUHandlingSight::ResolveProfile(Weapon->GetWeaponBodyMesh(), Profile);
    FVector2D RearScreen(-1.f, -1.f), FrontScreen(-1.f, -1.f);
    int32 Width = 0, Height = 0;
    PC->GetViewportSize(Width, Height);
    const bool bRearProjected = bSight && PC->ProjectWorldLocationToScreen((Profile.SightInActor * Weapon->GetActorTransform()).GetLocation(), RearScreen);
    const bool bFrontProjected = bSight && Profile.bHasFrontSight && PC->ProjectWorldLocationToScreen(Weapon->GetActorTransform().TransformPosition(Profile.FrontSightInActor), FrontScreen);
    const FVector2D Center(Width * .5f, Height * .5f);
    const double RearError = bRearProjected ? FVector2D::Distance(RearScreen, Center) : -1.;
    const double FrontError = bFrontProjected ? FVector2D::Distance(FrontScreen, Center) : -1.;
    const bool bContacts = Presentation->HasCalibratedHandContacts();
    bInvalidMetrics |= !bSight || !bContacts || Right.ContainsNaN() || Left.ContainsNaN() || !FMath::IsFinite(AnimationTime);
    PendingImage = FString::Printf(TEXT("frame_%06d.bmp"), Frame);
    PendingMetrics = FString::Printf(TEXT("{\"frame\":%d,\"image\":\"%s\",\"stage\":\"%s\",\"stage_time\":%.6f,\"dt\":%.6f,\"rate\":%d,\"phase\":%d,\"action_active\":%s,\"revision\":%d,\"animation_time\":%.6f,\"right_gap_cm\":%.6f,\"left_gap_cm\":%.6f,\"right_step_cm\":%.6f,\"left_step_cm\":%.6f,\"rear_center_px\":%.6f,\"front_center_px\":%.6f,\"sight_valid\":%s,\"contacts_valid\":%s,\"ads_alpha\":%.6f,\"fov\":%.6f}\n"),
        Frame, *PendingImage, StageNames[Stage], StageTime, DeltaSeconds, Rate, static_cast<int32>(Action.Phase), Action.bActive ? TEXT("true") : TEXT("false"), Action.Revision,
        AnimationTime, RightGap, LeftGap, RightStep, LeftStep, RearError, FrontError, bSight ? TEXT("true") : TEXT("false"), bContacts ? TEXT("true") : TEXT("false"),
        Operator->GetHandlingADSAlpha(), PC->PlayerCameraManager->GetFOVAngle());
    PendingMetrics.RemoveFromEnd(TEXT("}\n"));
    const FVector R = Right.GetLocation(), L = Left.GetLocation();
    const FQuat RQ = Right.GetRotation(), LQ = Left.GetRotation();
    const UTUHandlingAnimInstance* Anim = Cast<UTUHandlingAnimInstance>(Body->GetAnimInstance());
    const FVector WorldR = Operator->GetMesh()->GetSocketLocation(TEXT("hand_r"));
    const FVector WorldL = Operator->GetMesh()->GetSocketLocation(TEXT("hand_l"));
    PendingMetrics += FString::Printf(TEXT(",\"action_weight\":%.6f,\"right_hand_world\":[%.6f,%.6f,%.6f],\"left_hand_world\":[%.6f,%.6f,%.6f],\"right_hand_quat\":[%.6f,%.6f,%.6f,%.6f],\"left_hand_quat\":[%.6f,%.6f,%.6f,%.6f],\"observer_right_gap_cm\":%.6f,\"observer_left_gap_cm\":%.6f"),
        Anim ? Anim->ActionWeight : -1.f, R.X, R.Y, R.Z, L.X, L.Y, L.Z, RQ.X, RQ.Y, RQ.Z, RQ.W, LQ.X, LQ.Y, LQ.Z, LQ.W,
        FVector::Distance(WorldR, Presentation->GetRightHandGripWorld()), FVector::Distance(WorldL, Presentation->GetLeftHandGripWorld()));
    for (int32 Side = 0; Side < 2; ++Side)
    {
        const bool bRight = Side == 0;
        const FTransform Socket = Body->GetSocketTransform(bRight ? TEXT("HandGrip_R") : TEXT("HandGrip_L"), RTS_World);
        const FVector S = Socket.GetLocation(); const FQuat Q = Socket.GetRotation();
        const FVector U = Body->GetSocketLocation(bRight ? TEXT("upperarm_r") : TEXT("upperarm_l"));
        const FVector Lw = Body->GetSocketLocation(bRight ? TEXT("lowerarm_r") : TEXT("lowerarm_l"));
        const FVector H = bRight ? R : L;
        const FVector Target = bRight ? Presentation->GetRightHandGripWorld() : Presentation->GetLeftHandGripWorld();
        const double ArmLength = FVector::Distance(U, Lw) + FVector::Distance(Lw, H);
        PendingMetrics += FString::Printf(TEXT(",\"%s_grip_socket_world\":[%.6f,%.6f,%.6f],\"%s_grip_socket_quat\":[%.6f,%.6f,%.6f,%.6f],\"%s_target_reach_cm\":%.6f,\"%s_arm_length_cm\":%.6f,\"%s_target_beyond_reach\":%s"),
            bRight ? TEXT("right") : TEXT("left"), S.X, S.Y, S.Z, bRight ? TEXT("right") : TEXT("left"), Q.X, Q.Y, Q.Z, Q.W,
            bRight ? TEXT("right") : TEXT("left"), FVector::Distance(U, Target), bRight ? TEXT("right") : TEXT("left"), ArmLength,
            bRight ? TEXT("right") : TEXT("left"), FVector::Distance(U, Target) > ArmLength + .1 ? TEXT("true") : TEXT("false"));
        bInvalidMetrics |= !Body->DoesSocketExist(bRight ? TEXT("HandGrip_R") : TEXT("HandGrip_L"));
    }
    // Read the solver's actual evaluated target, distinct from the fixed support contact.
    const bool bEvaluatedTargetValid = Anim && Anim->bEvaluatedTargetsValid;
    const FVector LeftTarget = bEvaluatedTargetValid
        ? Body->GetComponentTransform().TransformPosition(Anim->EvaluatedLeftTargetComponent.GetLocation())
        : Presentation->GetLeftHandGripWorld();
    const FVector LeftUpper = Body->GetSocketLocation(TEXT("upperarm_l"));
    const FVector LeftLower = Body->GetSocketLocation(TEXT("lowerarm_l"));
    const double LeftLength = FVector::Distance(LeftUpper, LeftLower) + FVector::Distance(LeftLower, L);
    PendingMetrics += FString::Printf(TEXT(",\"evaluated_target_valid\":%s,\"left_manipulation_target_world\":[%.6f,%.6f,%.6f],\"left_manipulation_gap_cm\":%.6f,\"left_manipulation_reach_cm\":%.6f,\"left_manipulation_beyond_reach\":%s"),
        bEvaluatedTargetValid ? TEXT("true") : TEXT("false"), LeftTarget.X, LeftTarget.Y, LeftTarget.Z,
        FVector::Distance(L, LeftTarget), FVector::Distance(LeftUpper, LeftTarget),
        FVector::Distance(LeftUpper, LeftTarget) > LeftLength + .1 ? TEXT("true") : TEXT("false"));
    // Screen coordinates are geometric visibility diagnostics, never proof of rendered visibility.
    auto AppendScreenPoint = [&](const TCHAR* Name, const FVector& World)
    {
        FVector2D Pixel(-1.f, -1.f);
        const bool bProjected = PC->ProjectWorldLocationToScreen(World, Pixel);
        const bool bOnScreen = bProjected && Width > 0 && Height > 0
            && Pixel.X >= 0. && Pixel.Y >= 0. && Pixel.X < Width && Pixel.Y < Height;
        PendingMetrics += FString::Printf(TEXT(",\"%s_screen\":[%.6f,%.6f],\"%s_on_screen\":%s"),
            Name, Pixel.X, Pixel.Y, Name, bOnScreen ? TEXT("true") : TEXT("false"));
    };
    AppendScreenPoint(TEXT("weapon_center"), Weapon->GetWeaponBodyMesh()->Bounds.Origin);
    AppendScreenPoint(TEXT("right_hand"), R);
    AppendScreenPoint(TEXT("left_hand"), L);
    AppendScreenPoint(TEXT("right_grip"), Body->GetSocketLocation(TEXT("HandGrip_R")));
    AppendScreenPoint(TEXT("left_grip"), Body->GetSocketLocation(TEXT("HandGrip_L")));
    // Conservative whole-rifle diagnostic: a visible receiver does not prove that
    // the rear stock clears the rendered camera near plane. Local bounds corners
    // include empty space, so a negative margin requires pixel/vertex inspection.
    const FMinimalViewInfo& View = PC->PlayerCameraManager->GetCameraCacheView();
    const FVector CameraForward = View.Rotation.Vector();
    const float NearPlane = View.GetFinalPerspectiveNearClipPlane();
    auto CameraDepth = [&](const FVector& World) { return FVector::DotProduct(World - View.Location, CameraForward); };
    FVector BoundsMin, BoundsMax;
    Weapon->GetWeaponBodyMesh()->GetLocalBounds(BoundsMin, BoundsMax);
    double RifleMinDepth = TNumericLimits<double>::Max();
    for (int32 Corner = 0; Corner < 8; ++Corner)
    {
        const FVector Local((Corner & 1) ? BoundsMax.X : BoundsMin.X,
            (Corner & 2) ? BoundsMax.Y : BoundsMin.Y, (Corner & 4) ? BoundsMax.Z : BoundsMin.Z);
        RifleMinDepth = FMath::Min(RifleMinDepth, CameraDepth(Weapon->GetWeaponBodyMesh()->GetComponentTransform().TransformPosition(Local)));
    }
    PendingMetrics += FString::Printf(TEXT(",\"camera_near_plane_cm\":%.6f,\"rifle_bounds_min_camera_depth_cm\":%.6f,\"rifle_bounds_near_margin_cm\":%.6f"),
        NearPlane, RifleMinDepth, RifleMinDepth - NearPlane);
    // Joint coordinates diagnose the owner-body loop without treating bare bone
    // centers as surface clearance or hiding any rendered body geometry.
    const FTransform CameraWorld(View.Rotation, View.Location);
    for (const TCHAR* Joint : { TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("upperarm_l"), TEXT("lowerarm_l") })
    {
        const FVector CameraPoint = CameraWorld.InverseTransformPosition(Body->GetSocketLocation(Joint));
        PendingMetrics += FString::Printf(TEXT(",\"%s_camera_cm\":[%.6f,%.6f,%.6f]"), Joint, CameraPoint.X, CameraPoint.Y, CameraPoint.Z);
    }
    PendingMetrics += FString::Printf(TEXT(",\"viewport\":[%d,%d]"), Width, Height);
    PendingMetrics += TEXT("}\n");
    if (auto* Parts = Weapon->PartsPresentation.Get(); Parts && Parts->IsSupported()) {
        const auto& P = Parts->GetFrame();
        const FVector M = Parts->GetMovingVisual()->Bounds.Origin;
        PendingMetrics.RemoveFromEnd(TEXT("}\n"));
        PendingMetrics += FString::Printf(TEXT(",\"parts_supported\":true,\"inserted_mag_id\":\"%s\",\"moving_mag_id\":\"%s\",\"moving_mag_visible\":%s,\"moving_mag_world\":[%.4f,%.4f,%.4f],\"weapon_kick_cm\":%.6f}\n"),
            *P.InsertedId.ToString(), *P.MovingId.ToString(), Parts->GetMovingVisual()->IsVisible()?TEXT("true"):TEXT("false"), M.X,M.Y,M.Z,Parts->GetWeaponKickCm());
    }
    if(auto* Parts=Weapon->PartsPresentation.Get();Parts && Parts->IsSupported()) {
        auto* Mag=Parts->GetFrame().MovingId.IsValid()?Parts->GetMovingVisual():Parts->GetInsertedVisual();
        PendingMetrics.RemoveFromEnd(TEXT("}\n"));
        PendingMetrics+=FString::Printf(TEXT(",\"mag_grip_alpha\":%.6f,\"control_grip_alpha\":%.6f,\"control_press\":%.6f,\"action_travel_cm\":%.6f,\"cycle_required\":%s,\"clearance_blocked\":%s"),
            Parts->GetFrame().MagazineGripAlpha,Parts->GetFrame().ControlGripAlpha,Parts->GetFrame().ControlPress,Parts->GetFrame().ActionTravelCm,
            Action.bRequiresChamberCycle?TEXT("true"):TEXT("false"),Operator->IsWeaponClearanceBlocked()?TEXT("true"):TEXT("false"));
        if(Mag && Mag->GetStaticMesh()) {
            const auto B=Mag->GetStaticMesh()->GetBounds();
            for(const TCHAR* Finger:{TEXT("index"),TEXT("middle"),TEXT("ring"),TEXT("pinky"),TEXT("thumb")}) {
                const FVector Point=Mag->GetComponentTransform().InverseTransformPosition(Body->GetSocketLocation(FName(*FString::Printf(TEXT("%s_03_l"),Finger))));
                const FVector D=(Point-B.Origin).GetAbs()-B.BoxExtent;
                const double Distance=D.ComponentMax(FVector::ZeroVector).Size()+FMath::Min(D.GetMax(),0.);
                PendingMetrics+=FString::Printf(TEXT(",\"%s_surface_gap_cm\":%.6f"),Finger,Distance);
            }
        }
        PendingMetrics += TEXT("}\n");
    }
    if(Stage==13 && Weapon->PartsPresentation && Weapon->PartsPresentation->IsSupported()) {
        auto* Parts=Weapon->PartsPresentation.Get();
        auto Clear=[&](UStaticMeshComponent* Mesh) {
            if(!Mesh || !Mesh->IsVisible()) return true;
            FVector A,B;Mesh->GetLocalBounds(A,B);
            return TUWeaponClearance::IsClear(GetWorld(),Operator,Weapon,Mesh->GetComponentTransform(),FBox(A,B));
        };
        const bool ReceiverClear=Clear(Parts->GetReceiverVisual());
        const bool MagazineClear=Clear(Parts->GetInsertedVisual()) && Clear(Parts->GetMovingVisual());
        PendingMetrics.RemoveFromEnd(TEXT("}\n"));
        PendingMetrics+=FString::Printf(TEXT(",\"receiver_clear\":%s,\"magazine_clear\":%s,\"muzzle_ready\":%s}\n"),
            ReceiverClear?TEXT("true"):TEXT("false"),MagazineClear?TEXT("true"):TEXT("false"),Weapon->CanFire()?TEXT("true"):TEXT("false"));
    }
    ++Frame;
    bPending = true;
    FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory, PendingImage), false, false);
}

void ATUHandlingCapture::OnPixels(int32 Width, int32 Height, const TArray<FColor>& Pixels)
{
    if (!bPending || bFinished) return;
    if(Pixels.Num()!=Width*Height) { Finish(false,TEXT("invalid viewport pixel count"));return; }
    if(!FFileHelper::CreateBitmap(*FPaths::Combine(Directory,PendingImage),Width,Height,Pixels.GetData()))
    { Finish(false,TEXT("viewport bitmap write failed"));return; }
    CapturedMetrics+=PendingMetrics;
    ++Images; bPending = false;
}
void ATUHandlingCapture::Finish(bool bSucceeded, const TCHAR* Reason)
{
    if (bFinished) return;
    bFinished = true;
    if(!FFileHelper::SaveStringToFile(CapturedMetrics,*FPaths::Combine(Directory,TEXT("frames.jsonl")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    { bSucceeded=false;Reason=TEXT("final telemetry write failed"); }
    const FString Json = FString::Printf(TEXT("{\"capture\":\"%s\",\"capture_complete\":%s,\"visual_pass\":false,\"reason\":\"%s\",\"rate\":%d,\"frames\":%d,\"images\":%d,\"reload_samples\":%d,\"reload_phases\":%d,\"evolving_samples\":%d,\"exit_code\":%d}\n"), *CaptureId,
        bSucceeded ? TEXT("true") : TEXT("false"), Reason, Rate, Frame, Images, ReloadSamples, ReloadPhases.Num(), EvolvingSamples, bSucceeded ? 0 : 1);
    FFileHelper::SaveStringToFile(Json, *FPaths::Combine(Directory, TEXT("exit.json")));
    UE_LOG(LogTemp, Display, TEXT("TUHandlingCapture %s"), *Json);
    FPlatformMisc::RequestExitWithStatus(false, bSucceeded ? 0 : 1);
}
void ATUHandlingCapture::EndPlay(const EEndPlayReason::Type Reason)
{
    if (ScreenshotHandle.IsValid()) UGameViewportClient::OnScreenshotCaptured().Remove(ScreenshotHandle);
    Super::EndPlay(Reason);
}

bool ATUHandlingCapture::VerifyPartsPickup()
{
 if (!bPartsShotPassed || !bPartsDropRequested || !PartsDropId.IsValid()) return false;
 auto* W=Operator->GetCurrentWeapon();
 for (TActorIterator<ATUWorldItem> It(GetWorld());It;++It) {
  const auto M=It->GetMagazine(); if(M.InstanceId!=PartsDropId) continue;
  if(!It->GetMagazineVisual() || !It->GetMagazineVisual()->IsVisible()) return false;
  FHitResult Floor; FCollisionQueryParams Query(SCENE_QUERY_STAT(TUPartsDropFloor),false,*It);
  const FVector Position=It->GetActorLocation();
  if(!GetWorld()->LineTraceSingleByObjectType(Floor,Position+FVector(0,0,10),Position-FVector(0,0,100),FCollisionObjectQueryParams(ECC_WorldStatic),Query)) return false;
  const float Height=Position.Z-Floor.ImpactPoint.Z;
  if(Height<0.f || Height>12.f) return false;
  if(!It->TryPickup(W)) return false;
  const auto After=W->ExportItemLedger();
  const auto* Found=After.Magazines.FindByPredicate([&](const auto& V){return V.InstanceId==M.InstanceId;});
  const bool Good=Found && Found->Cartridges==M.Cartridges && PartsRounds(After)==PartsInitialRounds-1;
  UE_LOG(LogTemp,Display,TEXT("TUWeaponPartsProbe %s drop=%s count=%d floorHeight=%.3f totalBefore=%d totalAfter=%d"),Good?TEXT("PASS"):TEXT("FAIL"),*M.InstanceId.ToString(),M.Cartridges.Num(),Height,PartsInitialRounds,PartsRounds(After));
  return Good;
 }
 UE_LOG(LogTemp,Error,TEXT("TUWeaponPartsProbe dropped magazine not found"));
 return false;
}
