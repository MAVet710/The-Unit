#include "TU_Karambit.h"

ATU_Karambit::ATU_Karambit()
{
    // Fixed-blade presentation: both authored locations are identical, so the
    // inherited readiness state changes without any telescoping blade motion.
    RetractedBladeLocation = FVector::ZeroVector;
    DeployedBladeLocation = FVector::ZeroVector;
    DeploymentDurationSeconds = 0.05f;
    RetractionDurationSeconds = 0.05f;

    // Fictionalized gameplay tuning, not real-world performance data.
    MeleeDamage = 42.0f;
    MeleeRangeCm = 150.0f;
    MeleeSweepRadiusCm = 8.0f;
}
