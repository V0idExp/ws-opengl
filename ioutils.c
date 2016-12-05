#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

char*
read_file(const char *filename)
{
	assert(filename != NULL);

	size_t size = 0;
	char *buf = NULL;

	FILE *fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "unable to open file '%s'\n", filename);
		goto error;
	}

	// read file size
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);

	// allocate a buffer for its contents
	if (!(buf = malloc(size + 1))) {
		fprintf(stderr, "out of memory\n");
		goto error;
	}
	buf[size] = 0;  // NUL-terminator

	// read the file
	if (fread(buf, 1, size, fp) != size) {
		fprintf(stderr, "read error\n");
		goto error;
	}

cleanup:
	if (fp) {
		fclose(fp);
	}
	return buf;

error:
	free(buf);
	goto cleanup;
}
