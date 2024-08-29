#pragma once
#include"globel_device_value.h"

struct SimpleSkeletonJoint
{
    std::string mBoneName;
    uint32_t mParentIndex;
    DirectX::XMFLOAT3 mBaseTranslation;
    DirectX::XMFLOAT4 mBaseRotation;
    DirectX::XMFLOAT3 mBaseScal;
};

class SimpleSkeletonData
{
public:
    std::vector<SimpleSkeletonJoint> mBoneTree;
    std::unordered_map<std::string, int32_t> mSearchIndex;
public:
    void Create(const std::string& configFile);
};

enum class SimpleAnimLerpType : uint8_t
{
    LerpLinear,
    LerpStep
};

struct AnimClickTrackData
{
    std::vector<DirectX::XMFLOAT3>   mPosKeys;
    std::vector<DirectX::XMFLOAT4>   mRotKeys;
    std::vector<DirectX::XMFLOAT3>   mScalKeys;
};

class SimpleAnimationData
{
    SimpleAnimLerpType mLerpType;

    uint32_t      mFramePerSec;

    float         mAnimClipLength;

    std::vector<AnimClickTrackData> mRawData;

    std::vector<std::string> mBoneName;

    std::unordered_map<std::string, int32_t> mBoneNameIdRef;

    std::vector<float> mKeyTimes;
public:
    void Create(const std::string& configFile);
};