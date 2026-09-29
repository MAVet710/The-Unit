#include "TU_RaidCombatant.h"
#include "TUHealthComponent.h"
#include "TU_TacticalRifle.h"
#include "TU_GameMode.h"
#include "TU_ArmedOperatorCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ATU_RaidCombatant::ATU_RaidCombatant()
{
    bReplicates = true;
    SetReplicateMovement(true);
    AIControllerClass = ATUEnemyAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
    SetRootComponent(Capsule);
    Capsule->InitCapsuleSize(34.f, 88.f);
    Capsule->SetCollisionProfileName(TEXT("Pawn"));
    Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(Capsule);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetRelativeLocation(FVector(0,0,-88));
    Body->SetRelativeRotation(FRotator(0,-90,0));
    Health = CreateDefaultSubobject<UTUHealthComponent>(TEXT("Health"));
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Mesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    if (Mesh.Succeeded()) Body->SetSkeletalMeshAsset(Mesh.Object);
}
void ATU_RaidCombatant::BeginPlay()
{
    Super::BeginPlay();
    if (UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS"))) Body->PlayAnimation(Idle,true);
    if (!HasAuthority()) return;
    DeathEventId = FGuid::NewGuid();
    if(ATUEnemyAIController* AI=Cast<ATUEnemyAIController>(GetController())) AI->ConfigureArchetype(Archetype);
    Health->OnDeath.AddDynamic(this,&ATU_RaidCombatant::OnKilled);
    FActorSpawnParameters Params; Params.Owner=this; Params.Instigator=this;
    Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Weapon=GetWorld()->SpawnActor<ATU_TacticalRifle>(GetActorLocation(),GetActorRotation(),Params);
    if (Weapon)
    {
        Weapon->AttachToComponent(Capsule,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        Weapon->SetActorRelativeLocation(FVector(25,15,45));
        Weapon->SetFireMode(ETUFireMode::SemiAuto);
        if (UStaticMesh* Rifle=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Weapons/Rifle/Meshes/SM_Rifle.SM_Rifle")))
        {
            Weapon->GetWeaponBodyMesh()->SetStaticMesh(Rifle);
            Weapon->GetWeaponBodyMesh()->SetRelativeTransform(FTransform::Identity);
            Weapon->GetWeaponBodyMesh()->SetOnlyOwnerSee(false);
        }
    }
    GetWorld()->GetTimerManager().SetTimer(ThinkTimer,this,&ATU_RaidCombatant::Think,.25f,true,1.f);
}
void ATU_RaidCombatant::EndPlay(const EEndPlayReason::Type Reason)
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(ThinkTimer);
    if (HasAuthority() && IsValid(Weapon)) Weapon->Destroy();
    Super::EndPlay(Reason);
}
bool ATU_RaidCombatant::HasLineOfSightTo(const APawn* Pawn) const
{
    if (!Pawn || !GetWorld()) return false;
    const FVector Eye=GetActorLocation()+FVector(0,0,50);
    const FVector Delta=Pawn->GetPawnViewLocation()-Eye;
    if (Delta.SizeSquared()>FMath::Square(SightRangeCm) || FVector::DotProduct(GetActorForwardVector(),Delta.GetSafeNormal())<.35f) return false;
    FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(RaidAISight),false,this);
    if (Weapon) Query.AddIgnoredActor(Weapon);
    return !GetWorld()->LineTraceSingleByChannel(Hit,Eye,Pawn->GetPawnViewLocation(),ECC_Visibility,Query) || Hit.GetActor()==Pawn;
}
void ATU_RaidCombatant::Think()
{
    if (!HasAuthority() || Health->IsDead() || !Weapon) return;
    APawn* Target=nullptr;
    for (TActorIterator<ATU_ArmedOperatorCharacter> It(GetWorld());It;++It)
    {
        ATU_ArmedOperatorCharacter* Player=*It;
        if (Player->GetPrimaryWeapon()) Player->GetPrimaryWeapon()->OnShotFired.AddUniqueDynamic(this,&ATU_RaidCombatant::HearShot);
        if (Player->GetSecondaryWeapon()) Player->GetSecondaryWeapon()->OnShotFired.AddUniqueDynamic(this,&ATU_RaidCombatant::HearShot);
        if (!Player->IsCombatDisabled() && HasLineOfSightTo(Player)) { Target=Player; break; }
    }
    if (!Target) { Weapon->StopFire(); if(ATUEnemyAIController* AI=Cast<ATUEnemyAIController>(GetController())) AI->LoseContact(); return; }
    if(ATUEnemyAIController* AI=Cast<ATUEnemyAIController>(GetController())) AI->ReportStimulus(Target->GetActorLocation(),true);
    SetActorRotation((Target->GetPawnViewLocation()-Weapon->GetWorldMuzzleLocation()).Rotation());
    if (Weapon->IsReloading()) return;
    if (Weapon->GetCurrentAmmo() == 0) { Weapon->StartReload(); return; }
    Weapon->FireSingleShot();
}
void ATU_RaidCombatant::HearShot(FTUWeaponShotResult Shot)
{
    if (!HasAuthority() || Health->IsDead() || !Shot.bFired) return;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(RaidAIHearing),false,this); FHitResult Hit;
    const bool Occluded=GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation(),Shot.TraceStart,ECC_Visibility,Query);
    const float Range=HearingRangeCm*(Occluded?.35f:1.f);
    if (FVector::DistSquared(GetActorLocation(),Shot.TraceStart)<=FMath::Square(Range))
    {
        SetActorRotation(FRotator(0,(Shot.TraceStart-GetActorLocation()).Rotation().Yaw,0));
        if(ATUEnemyAIController* AI=Cast<ATUEnemyAIController>(GetController())) AI->ReportStimulus(Shot.TraceStart,false);
    }
    // Hearing turns toward a last audible event; it never supplies a target through walls.
}
float ATU_RaidCombatant::TakeDamage(float Damage,const FDamageEvent& Event,AController* EventInstigator,AActor* Causer)
{
    if (!HasAuthority() || !FMath::IsFinite(Damage) || Damage<=0 || Health->IsDead()) return 0;
    APawn* AttackingPawn = nullptr;
    if (EventInstigator) AttackingPawn = EventInstigator->GetPawn();
    else if (Causer) AttackingPawn = Cast<APawn>(Causer->GetOwner());
    LastAttacker = AttackingPawn;
    const float Before=Health->GetTotalHealth(); Health->ApplyRegionalDamage(ETUBodyRegion::Chest,Damage);
    return Before-Health->GetTotalHealth();
}
void ATU_RaidCombatant::OnKilled(AActor* DeadActor)
{
    if(ATUEnemyAIController* AI=Cast<ATUEnemyAIController>(GetController())) AI->MarkDead();
    if (Weapon) { Weapon->StopFire(); Weapon->InterruptWeaponAction(); }
    GetWorld()->GetTimerManager().ClearTimer(ThinkTimer);
    if (ATU_GameMode* Raid=GetWorld()->GetAuthGameMode<ATU_GameMode>())
        Raid->RecordTaskEvent(LastAttacker.Get(),ETUTaskCondition::Eliminate,TEXT("RaidGuard"),1,DeathEventId);
}
