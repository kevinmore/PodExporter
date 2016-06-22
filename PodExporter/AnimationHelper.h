#pragma once
#include "Common.h"
#include <assimp/scene.h>
using namespace std;
class AnimationHelper
{
public:
	AnimationHelper() {}
	AnimationHelper(mat4& globalInverse, map<string, mat4>& offsetMapping);
	map<string, mat4> getBoneFinalTransformsAtFrame(uint frameIndex, aiAnimation* pAnimation, aiNode* pRootNode);
	void calcInterpolatedScaling(vec3& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void calcInterpolatedRotation(quat& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void calcInterpolatedPosition(vec3& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	aiNodeAnim* findNodeAnim(aiAnimation* pAnimation, aiString nodeName);

	void reSampleAnimation(aiAnimation* pAnimation);

	uint& getNumFrames() { return m_numFrames; }
	double& getFrameIntervalInTicks() { return m_frameIntervalInTicks; }

private:
	void calcFinalTransformsAtFrame(uint frameIndex, aiAnimation* pAnimation, aiNode* pNode, mat4 &parentTransform = mat4());

	uint findScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
	uint findRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
	uint findPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
	uint m_numFrames;
	double m_frameIntervalInTicks;
	mat4 m_globalInverseMatrix;
	map<string, mat4> m_BoneOffsetMatrixMapping; // maps a bone name to its offset matrix
	map<string, mat4> m_BoneFinalMatrixMapping; // maps a bone name to its final matrix
};

