#define TINYGLTF_IMPLEMENTATION

#include<map>
#include<cstring>

#include "GLTFLoader.h"
#include "tiny_gltf.h"

struct
{
	int primitiveId;
	int materialId;
};

void LoadMeshes()