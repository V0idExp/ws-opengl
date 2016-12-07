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

	size_t vertex_size = sizeof(GLfloat) * 5;
	size_t vdata_size = 0;

	// count the number of vertices in all the loaded meshes and compute the
	// total size of vertex data
	for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
		struct aiMesh *mesh = scene->mMeshes[m];
		vdata_size += vertex_size * mesh->mNumFaces * 3;
	}

	// allocate the array of vertex data
	GLfloat *vdata = malloc(vdata_size);
	if (!vdata) {
		goto cleanup;
	}

	// add each mesh data to the array
	size_t offset = 0;
	size_t face_count = 0;
	for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
		struct aiMesh *mesh = scene->mMeshes[m];
		for (unsigned int f = 0; f < mesh->mNumFaces; f++, face_count++) {
			struct aiFace face = mesh->mFaces[f];
			for (short i = 0; i < 3; i++) {
				struct aiVector3D v = mesh->mVertices[face.mIndices[i]];
				vdata[offset + i * 5 + 0] = v.x;
				vdata[offset + i * 5 + 1] = v.y;
				vdata[offset + i * 5 + 2] = v.z;

				struct aiVector3D t = mesh->mTextureCoords[0][face.mIndices[i]];
				vdata[offset + i * 5 + 3] = t.x;
				vdata[offset + i * 5 + 4] = t.y;
			}
			offset += 15;
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

	*num_triangles = face_count;

cleanup:
	// we're done, release all resources associated with this import
	aiReleaseImport(scene);

	return vbo;
}