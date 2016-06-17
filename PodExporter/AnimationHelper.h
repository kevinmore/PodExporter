#pragma once
#include "Common.h"
#include <assimp/scene.h>
using namespace std;
class AnimationHelper
{
public:
	AnimationHelper(mat4& globalInverse, map<string, mat4>& offsetMapping);
	map<string, mat4> getBoneFinalTransformsAtFrame(uint frameIndex, aiAnimation* pAnimation, aiNode* pRootNode);

private:
	void calcFinalTransformsAtFrame(uint frameIndex, aiAnimation* pAnimation, aiNode* pNode, mat4 &parentTransform = mat4());
	aiNodeAnim* findNodeAnim(aiAnimation* pAnimation, aiString nodeName);

	mat4 m_globalInverseMatrix;
	map<string, mat4> m_BoneOffsetMatrixMapping; // maps a bone name to its offset matrix
	map<string, mat4> m_BoneFinalMatrixMapping; // maps a bone name to its final matrix
};

