#include"skeletal_animation.h"

void SimpleSkeletonData::Create(const std::string& configFile)
{
    std::vector<char> buffer;
    LoadFileToVectorBinary(configFile, buffer);
    const byte* data_header = (const byte*)buffer.data();
    size_t offset = 0;
    const byte* ptr = data_header;
    size_t SkeletonBoneSize = 0;
    memcpy(&SkeletonBoneSize, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    mBoneTree.clear();
    for (size_t id = 0; id < SkeletonBoneSize; ++id)
    {
        SimpleSkeletonJoint newBoneData;
        size_t curBoneNameLength = 0;
        memcpy(&curBoneNameLength, ptr, sizeof(size_t));
        ptr += sizeof(size_t);

        char* namePtr = new char[curBoneNameLength + 1];
        namePtr[curBoneNameLength] = '\0';
        memcpy(namePtr, ptr, curBoneNameLength * sizeof(char));
        newBoneData.mBoneName = namePtr;
        ptr += curBoneNameLength * sizeof(char);
        delete[] namePtr;

        memcpy(&newBoneData.mBaseTranslation, ptr, sizeof(newBoneData.mBaseTranslation));
        ptr += sizeof(newBoneData.mBaseTranslation);
        memcpy(&newBoneData.mBaseRotation, ptr, sizeof(newBoneData.mBaseRotation));
        ptr += sizeof(newBoneData.mBaseRotation);
        memcpy(&newBoneData.mBaseScal, ptr, sizeof(newBoneData.mBaseScal));
        ptr += sizeof(newBoneData.mBaseScal);
        memcpy(&newBoneData.mParentIndex, ptr, sizeof(newBoneData.mParentIndex));
        ptr += sizeof(newBoneData.mParentIndex);

        mSearchIndex.insert({ newBoneData.mBoneName,(int32_t)mBoneTree.size() });
        mBoneTree.push_back(newBoneData);
    }
}

void SimpleAnimationData::Create(const std::string& configFile)
{
    std::vector<char> buffer;
    LoadFileToVectorBinary(configFile, buffer);
    const byte* data_header = (const byte*)buffer.data();
    size_t offset = 0;
    const byte* ptr = data_header;
    memcpy(&mLerpType, ptr, sizeof(mLerpType));
    ptr += sizeof(mLerpType);
    memcpy(&mFramePerSec, ptr, sizeof(mFramePerSec));
    ptr += sizeof(mFramePerSec);
    memcpy(&mAnimClipLength, ptr, sizeof(mAnimClipLength));
    ptr += sizeof(mAnimClipLength);
    uint32_t AnimTrackSize = 0;
    memcpy(&AnimTrackSize, ptr, sizeof(AnimTrackSize));
    ptr += sizeof(AnimTrackSize);
    for (uint32_t id = 0; id < AnimTrackSize; ++id)
    {
        //曲线名称
        size_t curBoneNameLength = 0;
        memcpy(&curBoneNameLength, ptr, sizeof(size_t));
        ptr += sizeof(size_t);
        char* namePtr = new char[curBoneNameLength + 1];
        namePtr[curBoneNameLength] = '\0';
        memcpy(namePtr, ptr, curBoneNameLength * sizeof(char));
        mBoneName.push_back(namePtr);
        mBoneNameIdRef.insert({ namePtr ,id });
        ptr += curBoneNameLength * sizeof(char);
        delete[] namePtr;

        //平移曲线
        mRawData.emplace_back();
        size_t posKeySize = 0;
        memcpy(&posKeySize, ptr, sizeof(size_t));
        ptr += sizeof(size_t);
        mRawData.back().mPosKeys.resize(posKeySize);
        memcpy(mRawData.back().mPosKeys.data(), ptr, posKeySize * sizeof(DirectX::XMFLOAT3));
        ptr += posKeySize * sizeof(DirectX::XMFLOAT3);

        //旋转曲线
        size_t rotKeySize = 0;
        memcpy(&rotKeySize, ptr, sizeof(size_t));
        ptr += sizeof(size_t);
        mRawData.back().mRotKeys.resize(rotKeySize);
        memcpy(mRawData.back().mRotKeys.data(), ptr, rotKeySize * sizeof(DirectX::XMFLOAT4));
        ptr += posKeySize * sizeof(DirectX::XMFLOAT4);

        //缩放曲线
        size_t scaleKeySize = 0;
        memcpy(&scaleKeySize, ptr, sizeof(size_t));
        ptr += sizeof(size_t);
        mRawData.back().mScalKeys.resize(scaleKeySize);
        memcpy(mRawData.back().mScalKeys.data(), ptr, scaleKeySize * sizeof(DirectX::XMFLOAT3));
        ptr += scaleKeySize * sizeof(DirectX::XMFLOAT3);
    }
    size_t timeKeySize = 0;
    memcpy(&timeKeySize, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    mKeyTimes.resize(timeKeySize);
    memcpy(mKeyTimes.data(), ptr, timeKeySize * sizeof(float));
    ptr += timeKeySize * sizeof(float);
}