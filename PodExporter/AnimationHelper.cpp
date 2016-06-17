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
