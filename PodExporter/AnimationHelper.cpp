#include "AnimationHelper.h"


AnimationHelper::AnimationHelper(mat4& globalInverse, map<string, mat4>& offsetMapping)
{
	m_globalInverseMatrix = globalInverse;
	m_BoneOffsetMatrixMapping = offsetMapping;
}

map<string, mat4> AnimationHelper::getBoneFinalTransformsAtFrame(uint frameIndex, aiAnimation* pAnimation, aiNode* pRootNode)
{
	calcFinalTransformsAtFrame(frameIndex, pAnimation, pRootNode);

	return m_BoneFinalMatrixMapping;
}

void AnimationHelper::calcFinalTransformsAtFrame(uint frameIndex, aiAnimation* pAnimation, aiNode* pNode, mat4 &parentTransform /*= mat4()*/)
{
	mat4 nodeTransform = pNode->mTransformation;

	aiNodeAnim* pNodeAnim = findNodeAnim(pAnimation, pNode->mName);

	if (pNodeAnim)
	{
		aiVectorKey pos = frameIndex < pNodeAnim->mNumPositionKeys ? pNodeAnim->mPositionKeys[frameIndex]
			: pNodeAnim->mPositionKeys[pNodeAnim->mNumPositionKeys - 1];

		aiQuatKey rot = frameIndex < pNodeAnim->mNumRotationKeys ? pNodeAnim->mRotationKeys[frameIndex]
			: pNodeAnim->mRotationKeys[pNodeAnim->mNumRotationKeys - 1];

		aiVectorKey scaling = frameIndex < pNodeAnim->mNumScalingKeys ? pNodeAnim->mScalingKeys[frameIndex]
			: pNodeAnim->mScalingKeys[pNodeAnim->mNumScalingKeys - 1];

		nodeTransform = mat4(scaling.mValue, rot.mValue, pos.mValue);
	}

	mat4 globalTransform = parentTransform * nodeTransform;
	std::string nodeName(pNode->mName.C_Str());
	if (m_BoneOffsetMatrixMapping.find(nodeName) != m_BoneOffsetMatrixMapping.end())
	{
		m_BoneFinalMatrixMapping[nodeName] = m_globalInverseMatrix * globalTransform * m_BoneOffsetMatrixMapping[nodeName];
	}

// 	bool printout = false;
// 	if (printout)
// 	{
// 		vec3 scaling, pos;
// 		quat rot;
// 		globalTransform.Decompose(scaling, rot, pos);
// 		cout << "-------------------------" << endl;
// 		cout << nodeName << endl;
// 		cout << "Translation: " << pos.x << ", " << pos.y << ", " << pos.z << endl;
// 		cout << "Scaling: " << scaling.x << ", " << scaling.y << ", " << scaling.z << endl;
// 		cout << "Rotation: " << rot.x << ", " << rot.y << ", " << rot.z << ", " << rot.w << endl;
// 		cout << "-------------------------" << endl;
// 	}

	for (uint i = 0; i < pNode->mNumChildren; ++i)
	{
		calcFinalTransformsAtFrame(frameIndex, pAnimation, pNode->mChildren[i], globalTransform);
	}
}

aiNodeAnim* AnimationHelper::findNodeAnim(aiAnimation* pAnimation, aiString nodeName)
{
	for (uint i = 0; i < pAnimation->mNumChannels; ++i)
	{
		aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

		if (pNodeAnim->mNodeName == nodeName)
		{
			return pNodeAnim;
		}
	}

	return NULL;
}

uint AnimationHelper::findScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	for (uint i = 0; i < pNodeAnim->mNumScalingKeys - 1; ++i) 
	{
		if (AnimationTime < pNodeAnim->mScalingKeys[i + 1].mTime) 
		{
			return i;
		}
	}

	return 0;
}

uint AnimationHelper::findRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	for (uint i = 0; i < pNodeAnim->mNumRotationKeys - 1; ++i)
	{
		if (AnimationTime < pNodeAnim->mRotationKeys[i + 1].mTime)
		{
			return i;
		}
	}

	return 0;
}

uint AnimationHelper::findPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	for (uint i = 0; i < pNodeAnim->mNumPositionKeys - 1; ++i)
	{
		if (AnimationTime < pNodeAnim->mPositionKeys[i + 1].mTime)
		{
			return i;
		}
	}

	return 0;
}

void AnimationHelper::calcInterpolatedScaling(vec3& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumScalingKeys == 1) 
	{
		Out = pNodeAnim->mScalingKeys[0].mValue;
		return;
	}

	uint ScalingIndex = findScaling(AnimationTime, pNodeAnim);
	uint NextScalingIndex = (ScalingIndex + 1);
	//assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
	float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
	//assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
	const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
	aiVector3D Delta = End - Start;
	Out = Start + Factor * Delta;
}

void AnimationHelper::calcInterpolatedRotation(quat& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	// we need at least two values to interpolate...
	if (pNodeAnim->mNumRotationKeys == 1) 
	{
		Out = pNodeAnim->mRotationKeys[0].mValue;
		return;
	}

	uint RotationIndex = findRotation(AnimationTime, pNodeAnim);
	uint NextRotationIndex = (RotationIndex + 1);
	//assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
	float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
	//assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
	const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
	aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
	Out = Out.Normalize();
}

void AnimationHelper::calcInterpolatedPosition(vec3& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mNumPositionKeys == 1) 
	{
		Out = pNodeAnim->mPositionKeys[0].mValue;
		return;
	}

	uint PositionIndex = findPosition(AnimationTime, pNodeAnim);
	uint NextPositionIndex = (PositionIndex + 1);
	//assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
	float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
	float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
	//assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
	const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
	aiVector3D Delta = End - Start;
	Out = Start + Factor * Delta;
}

