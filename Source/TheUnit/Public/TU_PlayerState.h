#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TUExecutionTypes.h"
#include "TU_PlayerState.generated.h"
UCLASS()
class THEUNIT_API ATU_PlayerState : public APlayerState
{
    GENERATED_BODY()
public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void CopyProperties(APlayerState* PlayerState) override;
    virtual void OverrideWith(APlayerState* PlayerState) override;
    UPROPERTY(Replicated, BlueprintReadOnly) FGuid PersistentPlayerId;
    UPROPERTY(Replicated, BlueprintReadOnly) TArray<FTUTaskProgress> TaskProgress;
};
