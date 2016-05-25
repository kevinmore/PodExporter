#include "Skeleton.h"

Skeleton::Skeleton(Bone* root, mat4& globalInverseMatrix)
	: m_root(root),
	m_gloableInverseMatrix(globalInverseMatrix)
{
	initialize(m_root);

	// make the bone list
	m_BoneList.resize(getSkeletonSize());
	for (auto it = m_BoneMap.begin(); it != m_BoneMap.end(); ++it)
	{
		m_BoneList[it->second->m_ID] = it->second;
	}
}


Skeleton::~Skeleton()
{
	clear();
}

void Skeleton::initialize(Bone* pBone)
{
	// add this bone to the map
	if (m_BoneMap.find(pBone->m_name) == m_BoneMap.end() && pBone->m_name != "Skeleton ROOT")
	{
		m_BoneMap[pBone->m_name] = pBone;

		// do some calculations by the way
		pBone->decomposeModelSpaceTransform();
	}

	for (int i = 0; i < pBone->childCount(); ++i)
	{
		initialize(pBone->getChild(i));
	}
}

void Skeleton::sortPose(Bone* pBone, mat4 &parentTransform)
{
	// calculate the global transform
	pBone->m_modelSpaceTransform = parentTransform * pBone->m_boneSpaceTransform;  // P * B
	pBone->m_finalTransform = m_gloableInverseMatrix * pBone->m_modelSpaceTransform * pBone->m_offsetMatrix;

	pBone->decomposeModelSpaceTransform();

	for (int i = 0; i < pBone->childCount(); ++i)
	{
		sortPose(pBone->getChild(i), pBone->m_modelSpaceTransform);
	}
}

void Skeleton::dumpSkeleton(Bone* pBone, uint level)
{
	for (uint i = 0; i < level; ++i)
		printf("  ");

	cout << pBone->m_ID << pBone->m_name << endl;

	for (int i = 0; i < pBone->childCount(); ++i)
		dumpSkeleton(pBone->getChild(i), level + 1);
}

void Skeleton::clear()
{
	for (uint i = 0; i < m_BoneList.size(); ++i)
	{
		SAFE_DELETE(m_BoneList[i]);
	}
}

float Skeleton::getDistanceBetween(Bone* upperBone, Bone* lowerBone)
{
	// if they are the same bone
	if (upperBone == lowerBone) return 0.0f;

	// check if they are in the same chain
	if (!isInTheSameChain(upperBone, lowerBone))
	{
		cout << "getLengthBetween Failed! The Bones are not in the same  Chain" << upperBone->m_name << lowerBone->m_name << endl;
		return -1;
	}

	// calculate the world distance
	return (upperBone->getModelSpacePosition() - lowerBone->getModelSpacePosition()).Length();
}

float Skeleton::getDistanceBetween(const string& upperBoneName, const string& lowerBoneName)
{
	// check input
	if (!isBoneInSkeleton(lowerBoneName) || !isBoneInSkeleton(upperBoneName)) return -1.0;

	return getDistanceBetween(m_BoneMap[upperBoneName], m_BoneMap[lowerBoneName]);
}

Bone* Skeleton::getBone(string boneName)
{
	if (isBoneInSkeleton(boneName)) return m_BoneMap[boneName];
	else return NULL;
}

bool Skeleton::isBoneInSkeleton(const string& boneName)
{
	if (m_BoneMap.find(boneName) == m_BoneMap.end())
	{
		cout << "Cannot find bone" << boneName << "in the skeleton!" << endl;
		return false;
	}
	return true;
}

bool Skeleton::isInTheSameChain(Bone* upperBone, Bone* lowerBone)
{
	// same bone
	if (upperBone == lowerBone) return false;

	bool inTheSameChain = false;
	Bone* temp = lowerBone;
	while (temp->m_parent)
	{
		if (temp->m_parent == upperBone)
		{
			inTheSameChain = true;
			break;
		}

		temp = temp->m_parent;
	}

	return inTheSameChain;
}

bool Skeleton::isInTheSameChain(const string& upperBoneName, const string& lowerBoneName)
{
	// check input
	if (!isBoneInSkeleton(lowerBoneName) || !isBoneInSkeleton(upperBoneName)) return false;

	return isInTheSameChain(m_BoneMap[upperBoneName], m_BoneMap[lowerBoneName]);
}

uint Skeleton::getBoneCountBetween(Bone* upperBone, Bone* lowerBone)
{
	if (upperBone == lowerBone || !isInTheSameChain(upperBone, lowerBone))
	{
		return 0;
	}

	uint count = 1;
	Bone* temp = lowerBone;
	while (temp != upperBone)
	{
		++count;
		temp = temp->m_parent;
	}

	return count;
}

int Skeleton::getSkeletonSize()
{
	return m_BoneMap.size();
}

map<string, Bone*> Skeleton::getBoneMap()
{
	return m_BoneMap;
}

vector<Bone*> Skeleton::getBoneList()
{
	return m_BoneList;
}

mat4 Skeleton::getBoneGlobalTransform(Bone* pBone)
{
	mat4 globalTransform;

	while (pBone->m_parent && pBone->m_parent->m_name != "Skeleton ROOT")
	{
		globalTransform = pBone->m_parent->m_offsetMatrix * globalTransform;
		pBone = pBone->m_parent;
	}

	return globalTransform;
}


void Skeleton::getBoneChain(Bone* start, Bone* end, vector<Bone*> &boneChain)
{
	uint chainCount = getBoneCountBetween(start, end);
	boneChain.resize(chainCount);
	uint index = chainCount - 1;
	Bone* temp = end;
	while (temp != start->m_parent)
	{
		boneChain[index] = temp;
		--index;
		temp = temp->m_parent;
	}
}


void Skeleton::makeBoneListFrom(Bone* baseBone, vector<Bone*> &listOut)
{
	if (!baseBone) return;

	listOut.push_back(baseBone);

	for (int i = 0; i < baseBone->childCount(); ++i)
	{
		makeBoneListFrom(baseBone->getChild(i), listOut);
	}

}
