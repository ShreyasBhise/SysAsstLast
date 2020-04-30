#ifndef READMANIFEST_H
#define READMANIFEST_H

typedef struct node{
	char status;
	struct node* next;
	int version;
	char* filePath;
	char* hash;
} node;

int manifestVersion(int fd);
node* readManifest(int fd);

#endif
