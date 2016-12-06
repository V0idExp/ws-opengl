// assimp headers
#include <cimport.h>     // Plain-C interface
#include <scene.h>       // Output data structure
#include <postprocess.h> // Post processing flags

#include <GL/glew.h>
#include <assert.h>
#include <stdlib.h>

GLuint
load_obj(const char *filename, unsigned int *num_triangles)
{
	GLuint vbo = 0;
	const struct aiScene *scene = aiImportFile(
		filename,
		(
			aiProcess_CalcTangentSpace |
			aiProcess_Triangulate |
			aiProcess_JoinIdenticalVertices |
			aiProcess_SortByPType
		)
	);
	if (!scene) {
		return 0;
	}
	assert(scene->mNumMeshes >= 1);

	// pick the first mesh
	struct aiMesh *mesh = scene->mMeshes[0];

	// allocate and initialize an array of vertex data
	size_t vertex_size = sizeof(GLfloat) * 3;
	size_t vdata_size = vertex_size * mesh->mNumFaces * 3;
	GLfloat *vdata = malloc(vdata_size);
	if (!vdata) {
		goto cleanup;
	}
	for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
		struct aiFace face = mesh->mFaces[f];
		size_t offset = f * 9;
		for (short i = 0; i < 3; i++) {
			struct aiVector3D v = mesh->mVertices[face.mIndices[i]];
			vdata[offset + i * 3 + 0] = v.x;
			vdata[offset + i * 3 + 1] = v.y;
			vdata[offset + i * 3 + 2] = v.z;
		}
	}

	// create and setup the OpenGL buffer
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(
		GL_ARRAY_BUFFER,   // target buffer
		vdata_size,        // size in machine units (bytes)
		vdata,             // data to submit
		GL_STATIC_DRAW     // usage hint
	);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	*num_triangles = mesh->mNumFaces;

cleanup:
	// we're done, release all resources associated with this import
	aiReleaseImport(scene);

	return vbo;
}