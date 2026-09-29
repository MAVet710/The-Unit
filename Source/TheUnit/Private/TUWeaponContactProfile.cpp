#include "TUWeaponContactProfile.h"

FVector TUWeaponContact::MagazineCenterInActor() { return FVector(14.344508f,.006448f,-4.742494f); }
FVector TUWeaponContact::ActionControlInActor() { return FVector(3.3424f,-2.4824f,7.8398f); }
FTransform TUWeaponContact::MagazineHandInActor()
{
    // Wrist below the measured magazine, palm on its left face. The entire
    // contact frame moves with the magazine, including its orientation.
    const FQuat Support(.9440f,.1345f,-.2021f,-.2234f);
    return FTransform((FRotator(90.f,0,0).Quaternion()*Support).GetNormalized(),FVector(15.f,-5.f,-14.f));
}
FTransform TUWeaponContact::ControlHandInActor()
{
    const FQuat Support(.9440f,.1345f,-.2021f,-.2234f);
    return FTransform(Support.GetNormalized(),FVector(-13.679467f,-8.512138f,10.706678f));
}
FTransform TUWeaponContact::FingerPose(const FName& Bone,const FTransform& AuthoredGrip,float MagazineWeight,float ControlWeight)
{
    FTransform Result=AuthoredGrip;
    const FString Name=Bone.ToString();
    if(!Name.EndsWith(TEXT("_l"))) return Result;
    int32 Joint=Name.Contains(TEXT("_01_"))?0:Name.Contains(TEXT("_02_"))?1:Name.Contains(TEXT("_03_"))?2:-1;
    if(Joint<0) return Result;
    const float W=FMath::Clamp(MagazineWeight,0.f,1.f);
    float Curl=0.f;
    if(Name.StartsWith(TEXT("index_"))) { const float A[]={-20.f,-18.f,-22.f}; Curl=A[Joint]; }
    if(Name.StartsWith(TEXT("middle_"))) { const float A[]={-8.f,-15.f,-25.f}; Curl=A[Joint]; }
    if(Name.StartsWith(TEXT("ring_"))) { const float A[]={-5.f,-20.f,-25.f}; Curl=A[Joint]; }
    if(Name.StartsWith(TEXT("pinky_"))) { const float A[]={-5.f,-25.f,-30.f}; Curl=A[Joint]; }
    if(Name.StartsWith(TEXT("thumb_"))) { const float A[]={0.f,-25.f,-25.f}; Curl=A[Joint]; }
    const FQuat Grasp=(AuthoredGrip.GetRotation()*FQuat(FVector::UpVector,FMath::DegreesToRadians(Curl))).GetNormalized();
    Result.SetRotation(FQuat::Slerp(AuthoredGrip.GetRotation(),Grasp,W).GetNormalized());
    // The index reaches the action control; the other fingers keep a compact
    // grasp rather than playing the open-palm keys from the unrelated clip.
    if(ControlWeight>0.f && Name.StartsWith(TEXT("index_"))) {
        const float A[]={-8.f,-8.f,-5.f};
        const FQuat Point(FVector::UpVector,FMath::DegreesToRadians(A[Joint]));
        Result.SetRotation(FQuat::Slerp(Result.GetRotation(),Point,FMath::Clamp(ControlWeight,0.f,1.f)).GetNormalized());
    }
    return Result;
}

FTUWeaponMountProfile TUWeaponContact::ResolveMountProfile(FName WeaponId)
{
    FTUWeaponMountProfile P;
    if (WeaponId == TEXT("WPN_AK105_Modernized"))
    {
        P.HipOffset = FVector(31.f, 2.0f, -10.5f);
        P.HipRotation = FRotator(2.5f, 0.f, -1.5f);
        P.ReloadOffset = FVector(28.5f, 3.5f, -11.5f);
        P.ReloadRotation = FRotator(8.f, 5.f, -3.f);
    }
    else if (WeaponId == TEXT("WPN_M110_PrecisionDMR"))
    {
        P.HipOffset = FVector(33.f, 2.5f, -10.5f);
        P.HipRotation = FRotator(1.5f, 0.f, -1.f);
        P.ReloadOffset = FVector(31.f, 3.5f, -12.f);
        P.ReloadRotation = FRotator(7.f, 4.f, -2.f);
        P.ReloadClipBodyWeight = 0.04f;
    }
    else if (WeaponId == TEXT("WPN_CQB9"))
    {
        P.HipOffset = FVector(29.f, 2.f, -9.5f);
        P.HipRotation = FRotator(3.f, 0.f, -1.f);
        P.ReloadOffset = FVector(27.5f, 3.f, -10.5f);
        P.ReloadRotation = FRotator(7.f, 4.f, -2.f);
    }
    else if (WeaponId == TEXT("WPN_G34CM_CompetitionPistol") || WeaponId == TEXT("WPN_RGRFive7_TacticalPistol"))
    {
        P.HipOffset = FVector(26.f, 1.5f, -8.f);
        P.HipRotation = FRotator(4.f, 0.f, 0.f);
        P.ReloadOffset = FVector(25.f, 2.5f, -9.f);
        P.ReloadRotation = FRotator(6.f, 3.f, -1.f);
        P.ReloadClipBodyWeight = 0.03f;
    }
    return P;
}


FTUWeaponPresentationProfile TUWeaponContact::ResolvePresentationProfile(FName WeaponId)
{
    FTUWeaponPresentationProfile P;
    if (WeaponId == TEXT("WPN_AK105_Modernized"))
    {
        P.WeaponTranslationScale = 0.62f;
        P.WeaponPitchScale = 0.92f;
        P.WeaponYawScale = 0.62f;
        P.AutoImpulseScale = 0.74f;
        P.WeaponRecoveryRate = 14.f;
    }
    else if (WeaponId == TEXT("WPN_M110_PrecisionDMR"))
    {
        P.WeaponTranslationScale = 0.72f;
        P.WeaponPitchScale = 1.02f;
        P.WeaponYawScale = 0.46f;
        P.CameraPitchScale = 0.31f;
        P.WeaponRecoveryRate = 12.5f;
        P.CameraRecoveryRate = 18.f;
        P.SwayLocationCm = 0.10f;
    }
    else if (WeaponId == TEXT("WPN_CQB9"))
    {
        P.WeaponTranslationScale = 0.42f;
        P.WeaponPitchScale = 0.68f;
        P.WeaponYawScale = 0.48f;
        P.AutoImpulseScale = 0.68f;
        P.WeaponRecoveryRate = 18.f;
        P.CameraRecoveryRate = 23.f;
    }
    else if (WeaponId == TEXT("WPN_G34CM_CompetitionPistol") ||
             WeaponId == TEXT("WPN_RGRFive7_TacticalPistol"))
    {
        P.WeaponTranslationScale = 0.48f;
        P.WeaponPitchScale = 1.08f;
        P.WeaponYawScale = 0.72f;
        P.WeaponRollScale = 0.20f;
        P.CameraPitchScale = 0.22f;
        P.WeaponRecoveryRate = 19.f;
        P.SwayLocationCm = 0.10f;
        P.SwayRotationDegrees = 0.14f;
    }
    return P;
}
