#pragma once
#include "BoneInfo.h"
#include <map>

class Skeleton
{
public:
	Skeleton(Bone* root, mat4& globalInverseMatrix);
	~Skeleton();

	Bone* getBone(string boneName);
	int getSkeletonSize();
	map<string, Bone*> getBoneMap();
	vector<Bone*> getBoneList();
	void makeBoneListFrom(Bone* baseBone, vector<Bone*> &listOut);

	Bone* sortSkeleton(Bone* root);
	void sortPose(Bone* pBone, mat4 &parentTransform);


	// clean up the skeleton
	void clear();

	mat4 getBoneGlobalTransform(Bone* pBone);

	/** Method to print out the skeleton. **/
	void dumpSkeleton(Bone* pBone, uint level);

	bool isBoneInSkeleton(const string& boneName);

	bool isInTheSameChain(Bone* upperBone, Bone* lowerBone);
	bool isInTheSameChain(const string& upperBoneName, const string& lowerBoneName);

	float getDistanceBetween(Bone* upperBone, Bone* lowerBone);
	float getDistanceBetween(const string& upperBoneName, const string& lowerBoneName);

	uint getBoneCountBetween(Bone* upperBone, Bone* lowerBone);

	inline Bone* getRoot() { return m_root; }

	void getBoneChain(Bone* start, Bone* end, vector<Bone*> &boneChain);

	const mat4 getGlobalInverseMatrix() { return m_gloableInverseMatrix; }

private:

	void initialize(Bone* pBone);

	/** The root bone of the skeleton. **/
	Bone* m_root;

	/** The global inverse matrix, due to Assimp. **/
	mat4 m_gloableInverseMatrix;

	/** The bone map witch stores the bone name and its pointer. **/
	map<string, Bone*> m_BoneMap;
	vector<Bone*> m_BoneList;
};