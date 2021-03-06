#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include"readManifest.h"

void printManifest(node* root) {
	node* ptr;
	for(ptr = root; ptr != NULL; ptr = ptr->next) {
		printf("%c %d %s %s\n", ptr->status, ptr->version, ptr->filePath, ptr->hash);
	}

}
int readNum(int fd){
	int n;
	char* str = (char*)malloc(256);
	char c;
	int i = 0;
	while((n=read(fd, &c, 1)) > 0){
		if(n==0) continue;
		if(n<0){
			printf("Error: unable to read int");
			exit(1);
		}
		if(c=='\n' || c=='\t' || c== ' ') break;
		str[i]=c;
		i++;
	}
	str[i]='\0';
	int x = atoi(str);
	return x;
}

char* readStr(int fd){
	int n;
	int length = 10;
	char* str = (char*)malloc(length);
	char c;
	int i = 0;
	while((n=read(fd, &c, 1)) > 0){
		if(n==0) continue;
		if(n<0){
			printf("Error: unable to read string");
			exit(1);
		}
		if(c=='\t' || c=='\n' || c==' '){
			str[i]='\0';
			return str;
		}
		if(i+1>=length){ // double length of char array
			length = length*2;
			char* temp = (char*)malloc(length);
			str[i]='\0';
			strcpy(temp, str);
			char* temp2 = str;
			str = temp;
			free(temp2);
		}
		str[i]=c;
		i++;
	}
	printf("Error: readStr not terminated by tab or newline\n");
	return str;
}

node* readManifest(int fd){
	manifestSize = 0;
	node* root = (node*)malloc(sizeof(node));
	node* curr = root;
	node* prev = NULL;
	int count = 0, count2 = 0;
	while(1){
		char c[3];
		int n;
		n=read(fd, &c, 2);
		if(n<=0){
			break;
		}
		if(c[0]=='\n') break;
		c[2] = '\0';
		curr->status = c[0];
		curr->version = readNum(fd);
		curr->filePath = readStr(fd);
		curr->hash = readStr(fd);
		if(prev!=NULL){
			prev->next = curr;
		}
		prev = curr;
		curr = (node*)malloc(sizeof(node));
		manifestSize++;
	}
	free(curr);
	if(prev==NULL) return NULL; // .manifest contains no entries
	prev->next = NULL;
	return root;
}
