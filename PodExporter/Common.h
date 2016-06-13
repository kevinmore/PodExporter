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

	glm::mat4 toGLMMatrix4x42(aiMatrix4x4& m)
	{
		return glm::mat4(
			m.a1, m.b1, m.c1, m.d1,
			m.a2, m.b2, m.c2, m.d2,
			m.a3, m.b3, m.c3, m.d3,
			m.a4, m.b4, m.c4, m.d4);
	}
}
