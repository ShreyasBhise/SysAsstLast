#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<string.h>
#include<netinet/in.h>
#include<pthread.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include"readManifest.h"
#include"server.h"

int destroyProject(int sockfd) { /*Send 1 if the project exists, and will be deleted. 0 if the project doesn't exist */
	char* buffer = getProjName(sockfd);
	if(!projectExists(buffer)) {
//		printf("Project [%s] does not exist.\n", buffer);
		write(sockfd, "0", 1);
		return 0;	
	}
	
	char* sysCall = (char*) malloc(256);
	strcat(sysCall, "rm -r ./");
	strcat(sysCall, buffer);
	system(sysCall);
	
	write(sockfd, "1", 1);
	return 1;
}
int createProject(int sockfd) { /* Send 1 if the project does not exist, and a manifest is to be sent. 0 if project already exists. */
	char* buffer = getProjName(sockfd);

	if(!projectExists(buffer)) {
		write(sockfd, "1", 1);
		mkdir(buffer, 0700);
		
		char backupDir[350];
		sprintf(backupDir, "mkdir %s/.Backups", buffer);
//		printf("%s\n", backupDir);
		system(backupDir); 
		strcat(buffer, "/.Manifest");
		int fd = open(buffer, O_RDWR | O_CREAT, 00600);
		write(fd, "0\n", 2);
		sendFile(sockfd, fd);
		return 0;
	}
//	printf("Project already exists.\n");
	write(sockfd, "0", 1);
	return 1;
}
int currentVersion(int sockfd) {
	char* buffer = getProjName(sockfd);
	
	if(projectExists(buffer)) {
		char manifestPath[256];
		sprintf(manifestPath, "%s/.Manifest", buffer);	
		int manfd = open(manifestPath, O_RDONLY);
		int x = readNum(manfd);
		node* root = readManifest(manfd);
		char numElements[10];
		sprintf(numElements, "%d\n", manifestSize);
//		printf("num elements on server: %s\n", numElements);
		write(sockfd, numElements, strlen(numElements));
		node* ptr;
		for(ptr = root; ptr!= NULL; ptr = ptr->next) {
			char toWrite[256];
			char* fileName = extractFileNameFromPath(ptr->filePath);
			char version[6];
			sprintf(version, "%d", ptr->version);
			sprintf(toWrite, "%s\t%s\n", version, fileName);
			printf("wrote to client [%s]", toWrite);
			write(sockfd, toWrite, strlen(toWrite));
			bzero(toWrite, 256);
			bzero(version, 6);
		}
		return 0;
	}
//	printf("Project does not exist on server.\n");	
	write(sockfd, "0", 1);
	return 1;

} 
int checkout(int sockfd) {
	char* pName = getProjName(sockfd);
	
	if(projectExists(pName)) { //If the project exists, compress it and send it to client.
		write(sockfd, "1", 1);
		char sysCall[256];
		char tarName[25];
		sprintf(tarName, "%s.tar.gz", pName);
		sprintf(sysCall, "tar -czf %s --exclude='.Backups/*' ./%s", tarName, pName);
//		printf("tar file name: %s\n", tarName);
		system(sysCall);
		
		int toSend = open(tarName, O_RDONLY);
		sendFile(sockfd, toSend);
		close(toSend);
		bzero(sysCall, 256);
		sprintf(sysCall, "rm %s", tarName);
		system(sysCall);
		return 1;
		 	
	}	
	write(sockfd, "0", 1);
}
int commit(int sockfd){
	char* buffer = getProjName(sockfd);
//	printf("%s\n", buffer);
	if(!projectExists(buffer)){
//		printf("Project does not exist on server.\n");
		write(sockfd, "0", 1);
		return 1;
	}
	projNode* projCommitted = searchPNode(buffer);
	if(projCommitted == NULL) {
		projCommitted = (projNode*) malloc(sizeof(projNode));
		projCommitted->projName = buffer;
		projCommitted->commitListRoot = NULL;
	}
//	printf("test\n");
	write(sockfd, "1", 1);
	char manifestPath[256];
	sprintf(manifestPath, "%s/.Manifest", buffer);	
	int manfd = open(manifestPath, O_RDONLY);
	sendFile(sockfd, manfd);
	write(sockfd, "\n\n", 2);
	int size = readNum(sockfd);
	char* commitFile = (char*)malloc(size+1);
	int n = read(sockfd, commitFile, size);
	commitFile[size]='\0';
	commitNode* newnode = (commitNode*) malloc(sizeof(commitNode));

	newnode->next = projCommitted->commitListRoot;
	projCommitted->commitListRoot = newnode;
	newnode->file = commitFile;
	newnode->size = size;

	write(1, projCommitted->commitListRoot->file, size);
	projCommitted->next = projRoot;
	projRoot = projCommitted;

	return 0;
}
int push(int sockfd) {
	char* buffer = getProjName(sockfd);
	printf("%s\n", buffer);
	if(!projectExists(buffer)){
//		printf("Project does not exist on server.\n");
		write(sockfd, "0", 1);
		return 1;
	}
	write(sockfd, "1", 1);
	projNode* projCommitted = searchPNode(buffer);
	if(projCommitted==NULL){
		printf("2.Commit file does not match.\n");
		write(sockfd, "0", 1);
		return 1;
	}
	int x = readNum(sockfd);
	char* commitFile = (char*)malloc(x+1);
	int n = read(sockfd, commitFile, x);
	commitFile[x]='\0';
	commitNode* ptr = projCommitted->commitListRoot;
	while(ptr!=NULL){
//		printf("%d, %d\n", ptr->size, x);
//		printf("%d\n", memcmp(ptr->file, commitFile, x));
		if(ptr->size==x){
			if(memcmp(ptr->file, commitFile, x)==0){
				printf("break successful\n");
				break;
			}
		}
		ptr = ptr->next;
	}
	if(ptr==NULL){
		printf(".Commit file does not match.\n");
		write(sockfd, "0", 1);
		return 1;
	}
	int projNumber = getVersion(buffer);
	/*TODO: Save current project in .Backup directory */
	write(sockfd, "1", 1);
	int size = readNum(sockfd);
	char* tarFile = (char*)malloc(size+1);
	n = read(sockfd, tarFile, size);
	int tarfd = open("push.tar.gz", O_RDWR | O_CREAT, 00600);
	write(tarfd, tarFile, size);
	char manifestPath[256];
	sprintf(manifestPath, "%s/.Manifest", buffer);	
	int manfd = open(manifestPath, O_RDONLY);
	int manversion = readNum(manfd);
	node* manRoot = readManifest(manfd);
	char cmd[128];
	char projBackup[128];
	sprintf(projBackup, "%s_%d", buffer, projNumber);
	sprintf(cmd, "mv %s %s", buffer, projBackup);
	system(cmd);
	
	char cmd2[128] = "tar -xzf push.tar.gz";
	system(cmd2);
	close(tarfd);
	remove("push.tar.gz");
//	char cmd3[64] = "rm push.tar.gz";
//	system(cmd3);
	char commitPath[256];
	sprintf(commitPath, "%s/.Commit", buffer);	
	int commitfd = open(commitPath, O_RDWR | O_CREAT, 00600);
	write(commitfd, commitFile, x);
	lseek(commitfd, 0, SEEK_SET);
	node* commitRoot = readManifest(commitfd);
	close(commitfd);
//	printf("Server Manifest:\n");
//	printManifest(manRoot);
//	printf("Commit File:\n");
//	printManifest(commitRoot);
	int newManfd = open(manifestPath, O_RDWR | O_CREAT, 00600);
	char version[10];
	sprintf(version, "%d\n", manversion+1);
	write(newManfd, version, strlen(version));
	writeCommit(newManfd, commitRoot);
	node* ptr2;
	for(ptr2 = manRoot; ptr2!=NULL; ptr2 = ptr2->next){
		int y = isChanged(ptr2->filePath, commitRoot);
		if(y==0) { // file is not already in the commit
			char moveFile[256];
			char* newPath = removeProjName(ptr2->filePath);
			sprintf(moveFile, "cp %s_%d%s %s", buffer, manversion, newPath, buffer);
//			printf(moveFile);
			system(moveFile);
			char toWrite[256];
			sprintf(toWrite, "M\t%d\t%s\t%s\n", ptr2->version, ptr2->filePath, ptr2->hash);
			n = write(newManfd, toWrite, strlen(toWrite)); 
		}
	}	
	close(manfd);
/*	char cmd4[128];	
	sprintf(cmd4, "cp -r %s_%d/.Backups %s", buffer, projNumber, buffer);
	system(cmd4);
	char cmd5[128];
	sprintf(cmd5, "mv %s_%d %s/.Backups/%s_%d", buffer, projNumber, buffer, buffer, projNumber);
	system(cmd5); */
	freeNodeList(commitRoot);
	cleanDirectory(buffer);
	lseek(newManfd, 0, SEEK_SET);
	sendFile(sockfd, newManfd);
	close(newManfd);
	saveToBackups(buffer, projBackup);
	return 0;
}
int update(int sockfd){
	char* buffer = getProjName(sockfd);
	printf("%s\n", buffer);
	if(!projectExists(buffer)){
		printf("Project does not exist on server.\n");
		write(sockfd, "0", 1);
		return 1;
	}
	char manifestPath[256];
	sprintf(manifestPath, "%s/.Manifest", buffer);	
	int manfd = open(manifestPath, O_RDONLY);
	if(manfd<0){
		printf("Unable to open .Manifest\n");
		write(sockfd, "0", 1);
		return 1;
	}
	write(sockfd, "1", 1);
	sendFile(sockfd, manfd);
	write(sockfd, "\n\n", 2);
	return 1;
}
int upgrade(int sockfd) {
	char* pName = getProjName(sockfd);
	printf("Project name (upgrade): %s\n", pName);
	if(!projectExists(pName)) {
		write(sockfd, "0", 1);
		return 1;
	}
	write(sockfd, "1", 1);
	int size = readNum(sockfd);
	node* updateRoot = readManifest(sockfd); // .Update is now in linked list.
	printManifest(updateRoot);
	node* ptr;
	int elements = 0;
	for(ptr = updateRoot; ptr != NULL; ptr = ptr->next) {
		if(ptr->status == 'M' || ptr->status == 'A')
			elements++;
	}
	char* fileNames = (char*) malloc(128 * elements);
	sprintf(fileNames, "tar -czf update.tar.gz %s/.Manifest ", pName);
	printf("%s\n", fileNames);
	for(ptr = updateRoot; ptr != NULL; ptr = ptr->next) {
		if(ptr->status == 'M' || ptr->status == 'A')
			strcat(fileNames, ptr->filePath);
			strcat(fileNames, " ");
	}
	printf("%s\n", fileNames);
	system(fileNames); //Creates tar file update.tar.gz
	int tarfd = open("update.tar.gz", O_RDONLY);
	sendFile(sockfd, tarfd);	
	close(tarfd);
	remove("update.tar.gz");
	return 0;
}

int history(int sockfd) {

	return 0;
}

int rollback(int sockfd) {
	char* projName = readStr(sockfd);
	printf("rollback readStr %s\n", projName);
	char* version = readStr(sockfd);
	printf("rollback readStr %s\n", version);
	printf("Read project %s and version %s\n", projName, version);
	if(!projectExists(projName)) {
		write(sockfd, "0", 1);
		return -1;
	}
	write(sockfd, "1", 1);
	char backupToSearch[100];
	sprintf(backupToSearch, "%s_%s.tar.gz", projName, version);
	char backupPath[400];
	sprintf(backupPath, "%s/.Backups/%s", projName, backupToSearch);
	if(fileExists(backupPath) == -1) {
		write(sockfd, "0", 1);
		return -1;	
	}
	//Move backup tar to working directory, delete project, untar project, rename untar to projectname)
	char sysCall[600];
	sprintf(sysCall, "mv %s ./%s", backupPath, backupToSearch);
	printf("%s\n", sysCall);
	system(sysCall);
	char removeDir[256];
	sprintf(removeDir, "rm -r %s", projName);
	system(removeDir);
	mkdir(projName, S_IRWXU);
	char untar[256];
	
	sprintf(untar, "tar -xzf %s %s", backupToSearch, projName);
	printf("%s\n", untar);
	system(untar);

	return 0;
}
