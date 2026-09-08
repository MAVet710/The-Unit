#include "TU_HideoutCommandCenter.h"

#include "TUHideoutProgressionComponent.h"
#include "TU_HideoutCommandCenterDecorator.h"
#include "Components/ChildActorComponent.h"

ATU_HideoutCommandCenter::ATU_HideoutCommandCenter()
{
    HideoutLayerComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("HideoutLayer"));
    HideoutLayerComponent->SetupAttachment(GetRootComponent());
    HideoutLayerComponent->SetChildActorClass(ATU_HideoutCommandCenterDecorator::StaticClass());
}

ATU_HideoutCommandCenterDecorator* ATU_HideoutCommandCenter::GetHideoutLayer() const
{
    return HideoutLayerComponent
        ? Cast<ATU_HideoutCommandCenterDecorator>(HideoutLayerComponent->GetChildActor())
        : nullptr;
}

UTUHideoutProgressionComponent* ATU_HideoutCommandCenter::GetHideoutProgression() const
{
    if (ATU_HideoutCommandCenterDecorator* Layer = GetHideoutLayer())
    {
        return Layer->GetProgression();
    }
    return nullptr;
}
