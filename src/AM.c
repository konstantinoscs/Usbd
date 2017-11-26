#include "AM.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      printf("Error\n");      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }                           \



typedef enum AM_ErrorCode {
  OK,
  OPEN_SCANS_FULL
}AM_ErrorCode;

//openFiles holds the names of open files in the appropriate index
//TODO not char * but some other struct?
char * openFiles[20];

//insert_file takes the name of a file and inserts it into openFiles
//if openFiles is fulll then -1 is returned
int insert_file(char * fileName){
  for(int i=0; i<20; i++){
    //if a spot is free put the fileName in it and return
    //its index
    if(openFiles[i] == NULL ){
      openFiles[i] = malloc(strlen(fileName) +1);
      strcpy(openFiles[i], fileName);
      return i;
    }
  }
  return -1;
}

//close_file removes a file with index i from openFiles
void close_file(int i){
  free(openFiles[i]);
  openFiles[i] == NULL;
}

/************************************************
**************Scan*******************************
*************************************************/

typedef struct Scan {
	int fileDesc;			//the file that te scan refers to
	int block_num;		//last block that was checked
	int record_num;		//last record that was checked
	int op;			//the operation
	void *value;			//the target value
}Scan;

Scan* openScans[20];

int openScansInsert(Scan* scan);  //inserts a Scan in openScans[20] if possible and returns the position
int openScansFindEmptySlot();     //finds the first empty slot in openScans[20]
bool openScansFull();             //checks
AM_ErrorCode ScanInit(Scan* scan, int fileDesc, int op, void* value);


int openScansInsert(Scan* scan){
	int slot = openScansFindEmptySlot();
	openScans[slot] = scan;
	return slot;
}

int openScansFindEmptySlot(){
	int i;
	for(i=0; i<20; i++)
		if(openScans[i] == NULL)
			return i;
	return i;
}

bool openScansFull(){
	if(openScansFindEmptySlot() == 20)
		return true;
	return false;
}

AM_ErrorCode ScanInit(Scan* scan, int fileDesc, int op, void* value){
	scan->fileDesc = fileDesc;
	scan->op = op;
	scan->value = value;
	scan->block_num = -1;
	scan->record_num = -1;

	if(openScansFull() != true){	//make sure array there is space for one more scan
		return openScansInsert(scan);
	}
	else{
		free(scan);
		return OPEN_SCANS_FULL;
	}
}

/************************************************
**************Create*******************************
*************************************************/





/***************************************************
***************AM_Epipedo***************************
****************************************************/

int AM_errno = AME_OK;

void AM_Init() {
  BF_Init(MRU);
	return;
}


int AM_CreateIndex(char *fileName, char attrType1, int attrLength1, char attrType2, int attrLength2) {

  int type1,type2,len1,len2;

  if (attrType1 == INTEGER)
  {
    type1 = 1; //If type1 is equal to 1 the first attribute is int
  }else if (attrType1 == FLOAT)
  {
    type1 = 2; //If type1 is equal to 2 the first attribute is float
  }else if (attrType1 == STRING)
  {
    type1 = 3; //If type1 is equal to 3 the first attribute is string
  }else{
    return AME_WRONGARGS;
  }
  len1 = attrLength1;

  if (type1 == 1 || type1 == 2) // Checking if the argument type given matches the argument size given
  {
    if (len1 != 4)
    {
      return AME_WRONGARGS;
    }
  }else{
    if (len1 < 1 || len1 > 255)
    {
      return AME_WRONGARGS;
    }
  }

  if (attrType2 == INTEGER)
  {
    type2 = 1; //If type2 is equal to 1 the second attribute is int
  }else if (attrType2 == FLOAT)
  {
    type2 = 2; //If type2 is equal to 2 the second attribute is float
  }else if (attrType2 == STRING)
  {
    type2 = 3; //If type2 is equal to 3 the second attribute is string
  }else{
    return AME_WRONGARGS;
  }
  len2 = attrLength2;

  if (type2 == 1 || type2 == 2) // Checking if the argument type given matches the argument size given
  {
    if (len2 != 4)
    {
      return AME_WRONGARGS;
    }
  }else{
    if (len2 < 1 || len2 > 255)
    {
      return AME_WRONGARGS;
    }
  }

  /*attrMeta.type1 = type1;
  attrMeta.len1 = len1;
  attrMeta.type2 = type2;
  attrMeta.len2 = len2;*/


  BF_Block *tmpBlock;
  BF_Block_Init(&tmpBlock);

  int fd;
  //temporarily insert the file in openFiles
  int file_index = insert_file(fileName);
  if(file_index == -1)
    return AME_MAXFILES;

  CALL_OR_DIE(BF_CreateFile(fileName));
  CALL_OR_DIE(BF_OpenFile(fileName, &fd));

  char *data;
  char keyWord[15];
  //BF_AllocateBlock(fd, tmpBlock);
  CALL_OR_DIE(BF_AllocateBlock(fd, tmpBlock));//Allocating the first block that will host the metadaτa

  data = BF_Block_GetData(tmpBlock);
  strcpy(keyWord,"DIBLU$");
  memcpy(data, keyWord, sizeof(char)*15);//Copying the key-phrase DIBLU$ that shows us that this is a B+ file
  data += sizeof(char)*15;
  //Writing the attr1 and attr2 type and length right after the keyWord in the metadata block
  memcpy(data, &type1, sizeof(int));
  data += sizeof(int);
  memcpy(data, &len1, sizeof(int));
  data += sizeof(int);
  memcpy(data, &type2, sizeof(int));
  data += sizeof(int);
  memcpy(data, &len2, sizeof(int));

  BF_Block_SetDirty(tmpBlock);
  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));

  BF_Block_Destroy(&tmpBlock);
  CALL_OR_DIE(BF_CloseFile(fd));
  //remove the file from openFiles array
  close_file(file_index);

  return AME_OK;
}


int AM_DestroyIndex(char *fileName) {
  return AME_OK;
}


int AM_OpenIndex (char *fileName) {
  BF_Block *tmpBlock;
  int fileDesc, type1;
  BF_Block_Init(&tmpBlock);
  int file_index = insert_file(fileName);
  //check if we have reached the maximum number of files
  if(file_index == -1)
    return AME_MAXFILES;

  CALL_OR_DIE(BF_OpenFile(fileName, &fileDesc));

  //here should be the error checking

  char *data = NULL;
  CALL_OR_DIE(BF_GetBlock(fileDesc, 0, tmpBlock));//Getting the first block
  data = BF_Block_GetData(tmpBlock);//and its data

  if (data == NULL || strcmp(data, "DIBLU$"))//to check if this new opened file is a B+ tree file
  {
    printf("File: %s is not a B+ tree file. Exiting..\n", fileName);
    exit(-1);
  }

  CALL_OR_DIE(BF_UnpinBlock(tmpBlock));
  BF_Block_Destroy(&tmpBlock);
  return AME_OK;
}


int AM_CloseIndex (int fileDesc) {
  //TODO other stuff?

  //remove the file from the openFiles array
  close_file(fileDesc);
  return AME_OK;
}


int AM_InsertEntry(int fileDesc, void *value1, void *value2) {
  return AME_OK;
}


int AM_OpenIndexScan(int fileDesc, int op, void *value) {
  Scan scan;
  ScanInit(&scan, fileDesc, op, value);

  return AME_OK;
}


void *AM_FindNextEntry(int scanDesc) {

}


int AM_CloseIndexScan(int scanDesc) {
  return AME_OK;
}


void AM_PrintError(char *errString) {
  printf("%s\n", errString);
}

void AM_Close() {

}
