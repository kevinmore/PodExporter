#pragma once
#include "Common.h"
#define NUM_BONES_PER_VEREX 4

struct VertexBoneData
{
	uint8_t IDs[NUM_BONES_PER_VEREX];
	float Weights[NUM_BONES_PER_VEREX];

	VertexBoneData()
	{
		Reset();
	};

	void Reset()
	{
		ZERO_MEM(IDs);
		ZERO_MEM(Weights);
	}

	void AddBoneData(uint8_t BoneID, float Weight)
	{
		for (uint i = 0; i < ARRAY_SIZE_IN_ELEMENTS(IDs); ++i) 
		{
			if (Weights[i] == 0.0) 
			{
				IDs[i] = BoneID;
				Weights[i] = Weight;
				return;
			}
		}

		// should never get here - more bones than we have space for
		assert(0);
	}
};

class Bone
{
public:
	/** ID and Name. **/
	uint m_ID;
	string m_name;

	/** Matrixes. **/
	// bind pose matrix
	mat4 m_offsetMatrix;

	// the bone's transform in bone space
	mat4 m_boneSpaceTransform;

	// the bone's transform in model space
	mat4 m_modelSpaceTransform;

	// final matrix for the shader
	mat4 m_finalTransform;

	/** Parent bode. NULL if this node is the root. **/
	Bone* m_parent;

	/** Member functions **/

	Bone()
	{
		m_parent = NULL;
	}

	Bone(Bone* parent)
	{
		// init the current bone
		m_parent = parent;

		// add the current bone to its parent if it's not the root
		if (parent) parent->addChild(this);
	}

	~Bone()
	{}

	void addChild(Bone* child)
	{
		m_children.push_back(child);
	}

	inline vector<Bone*> getChildren()
	{
		return m_children;
	}

	inline Bone* getChild(uint i)
	{
		if (childCount() == 0) return NULL;
		else return m_children[i];
	}

	inline int childCount()
	{
		return m_children.size();
	}

	/** Utility functions **/

	// this function decompose the node transform matrix in model space into
	// 3 components: scaling, rotation, position
	void decomposeModelSpaceTransform()
	{
		m_modelSpaceTransform.Decompose(m_modelSpaceScaling, m_modelSpaceRotation, m_modelSpacePosition);
	}

	inline vec3 getModelSpacePosition()
	{
		return m_modelSpacePosition;
	}

	inline quat getModelSpaceRotation()
	{
		return m_modelSpaceRotation;
	}

	inline vec3 getModelSpaceScaling()
	{
		return m_modelSpaceScaling;
	}


private:

	/** Position, rotation, scaling in model space. **/
	vec3 m_modelSpacePosition;
	vec3 m_modelSpaceScaling;
	quat m_modelSpaceRotation;

	/** The child bones of this bone. **/
	vector<Bone*> m_children;
};