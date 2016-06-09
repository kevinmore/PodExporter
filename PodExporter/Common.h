#pragma once
#include <assert.h>
#include <memory>
#include <iostream>
#include <cassert>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <assimp/types.h>
using namespace std;

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