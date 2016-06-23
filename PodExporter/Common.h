#pragma once
#include <assert.h>
#include <memory>
#include <iostream>
#include <cassert>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <map>
#include <assimp/types.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>

typedef unsigned int uint;
typedef aiMatrix3x3 mat3;
typedef aiMatrix4x4 mat4;
typedef aiVector2D vec2;
typedef aiVector3D vec3;
typedef aiQuaternion quat;
typedef aiColor3D color3D;
typedef aiColor4D color4D;

#define ZERO_MEM(a) memset(a, 0, sizeof(a))
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(*a))
#define FREE(X)		{ if(X) { free(X); (X) = 0; } }
#define SAFE_DELETE(MEM)                \
{                                   \
	if ((MEM)) {                    \
	delete ((MEM));             \
	(MEM) = NULL;               \
	}                               \
}

#define SAFE_DELETE_ARRAY(MEM)          \
{                                   \
	if ((MEM)) {                    \
	delete[] ((MEM));           \
	(MEM) = NULL;               \
	}                               \
}

template <typename T> 
T CLAMP(const T& value, const T& low, const T& high)
{
	return value < low ? low : (value > high ? high : value);
}

namespace 
{
	glm::mat4 toGLMMatrix4x4(aiMatrix4x4& m)
	{
		return glm::mat4(
			m.a1, m.a2, m.a3, m.a4,
			m.b1, m.b2, m.b3, m.b4,
			m.c1, m.c2, m.c3, m.c4,
			m.d1, m.d2, m.d3, m.d4);
	}

	aiMatrix4x4 toAssimpMatrix4x4(glm::mat4& m)
	{
		return aiMatrix4x4(m[0][0], m[0][1], m[0][2], m[0][3],
						   m[1][0], m[1][1], m[1][2], m[1][3],
						   m[2][0], m[2][1], m[2][2], m[2][3],
						   m[3][0], m[3][1], m[3][2], m[3][3]);
	}

	//display a given matrix
	void DisplayMat4(aiMatrix4x4& m, bool transpose = false)
	{
		printf("--------------------------\n");
		aiMatrix4x4 mat = m;
		if (transpose)
			mat.Transpose();

		for (int i = 0; i < 4; i++) {
			printf("%f %f %f %f\n", mat[i][0], mat[i][1], mat[i][2], mat[i][3]);
		}

		printf("--------------------------\n");
	}

	void DisplayMat4(glm::mat4& m, bool transpose = false)
	{
		DisplayMat4(toAssimpMatrix4x4(m), transpose);
	}

	void DecomposeAndDisplayMat4(aiMatrix4x4& m)
	{
		using namespace std;
		vec3 scaling, pos;
		quat rot;
		m.Decompose(scaling, rot, pos);
		cout << "-------------------------" << endl;
		cout << "Translation: " << pos.x << ", " << pos.y << ", " << pos.z << endl;
		cout << "Scaling: " << scaling.x << ", " << scaling.y << ", " << scaling.z << endl;
		cout << "Rotation: " << rot.x << ", " << rot.y << ", " << rot.z << ", " << rot.w << endl;
		cout << "-------------------------" << endl;
	}

	mat4 MultiplyMatrix(aiMatrix4x4& m, float k)
	{
		mat4 ret;
		for (uint i = 0; i < 4; ++i)
		{
			for (uint j = 0; j < 4; ++j)
			{
				ret[i][j] = k * m[i][j];
			}
		}

		return ret;
	}

	void SetMatrixZero(aiMatrix4x4& inOutMatrix)
	{
		for (uint i = 0; i < 4; ++i)
		{
			for (uint j = 0; j < 4; ++j)
			{
				inOutMatrix[i][j] = 0.0f;
			}
		}
	}

	mat4 AddTowMatrices(aiMatrix4x4& m1, aiMatrix4x4& m2)
	{
		mat4 ret;
		for (uint i = 0; i < 4; ++i)
		{
			for (uint j = 0; j < 4; ++j)
			{
				ret[i][j] = m1[i][j] + m2[i][j];
			}
		}

		return ret;
	}
}
