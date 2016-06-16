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
	void displayMat4(const aiMatrix4x4 &m, bool transpose = false)
	{
		using namespace std;

		aiMatrix4x4 mat = m;
		if (transpose)
			mat.Transpose();

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
				cout << mat[i][j] << " ";
			cout << "\n";
		}
		cout << "\n--------------------------\n\n";
	}

	void displayMat4(const glm::mat4 &m, bool transpose = false)
	{
		using namespace std;

		glm::mat4 mat = m;
		if (transpose)
			mat = glm::transpose(mat);

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
				cout << mat[i][j] << " ";
			cout << "\n";
		}
		cout << "\n--------------------------\n\n";
	}
}
