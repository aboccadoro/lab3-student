#include "fs.h"

using namespace std;

fstream disk;

myFileSystem::myFileSystem(char diskName[16]) {
  // open the file with the above name
  // this file will act as the "disk" for your file system
  disk.open(diskName);
}

int myFileSystem::create_file(char name[8], int size) {
  //create a file with this name and this size

  // high level pseudo code for creating a new file

  // Step 1: Check to see if we have sufficient free space on disk by
  //   reading in the free block list. To do this:
  // Move the file pointer to the start of the disk file.
  // Read the first 128 bytes (the free/in-use block information)
  // Scan the list to make sure you have sufficient free blocks to
  //   allocate a new file of this size
  if(size > 8) return -1;
  char blockList[128];
  disk.seekg(0);
  disk.read(blockList, 128);
  int space = 0;
  for(int i = 0; i < 128; i++){
    if(blockList[i] == 0) space++;
  }
  if(space < size) return -1;

  // Step 2: we look for a free inode on disk
  // Read in an inode
  // Check the "used" field to see if it is free
  // If not, repeat the above two steps until you find a free inode
  // Set the "used" field to 1
  // Copy the filename to the "name" field
  // Copy the file size (in units of blocks) to the "size" field
  int nodes = 0;
  char currNode[48];
  idxNode* inode = (idxNode* )malloc(sizeof(idxNode));
  while(true) {
    if(nodes > 15) return -1;
    disk.seekg(128 + nodes * 48);
    disk.read(currNode, 48);
    memcpy(inode, &currNode, 48);
    if(!inode->used) break;
    nodes++;
  }
  inode->used = 1;
  strcpy(inode->name, name);
  inode->size = size;

  // Step 3: Allocate data blocks to the file
  // for(i=0;i<size;i++)
  // Scan the block list that you read in Step 1 for a free block
  // Once you find a free block, mark it as in-use (Set it to 1)
  // Set the blockPointer[i] field in the inode to this block number.
  // end for
  int count = 0;
  for(int i = 0; i < size; i++) {
    while(blockList[count] != 0) {
      count++;
    }
    blockList[count] = 1;
    inode->blockPointers[i] = count;
  }

  // Step 4: Write out the inode and free block list to disk
  // Move the file pointer to the start of the file
  // Write out the 128 byte free block list
  // Move the file pointer to the position on disk where this inode was stored
  // Write out the inode
  disk.seekg(0);
  disk.write(blockList, 128);
  disk.seekg(128 + nodes * 48);
  memcpy(&currNode, inode, 48);
  disk.write(currNode, 48);
  free(inode);

  return 1;
} // End Create

int myFileSystem::delete_file(char name[8]){
  // Delete the file with this name

  // Step 1: Locate the inode for this file
  // Move the file pointer to the 1st inode (129th byte)
  // Read in an inode
  // If the inode is free, repeat above step.
  // If the inode is in use, check if the "name" field in the
  //   inode matches the file we want to delete. If not, read the next
  //   inode and repeat
  disk.sync();
  int nodes = 0;
  char currNode[48];
  idxNode* inode = (idxNode *)malloc(sizeof(idxNode));
  while(true) {
    if(nodes > 15) return -1;
    disk.seekg(128 + nodes * 48);
    disk.read(currNode, 48);
    memcpy(inode, &currNode, 48);
    if(inode->used && strcmp(inode->name, name) == 0) break;
    nodes++;
  }

  // Step 2: free blocks of the file being deleted
  // Read in the 128 byte free block list (move file pointer to start
  //   of the disk and read in 128 bytes)
  // Free each block listed in the blockPointer fields as follows:
  // for(i=0;i< inode.size; i++)
  // freeblockList[ inode.blockPointer[i] ] = 0;
  disk.seekg(0);
  char blockList[128];
  disk.read(blockList, 128);
  for(int i = 0; i < inode->size; i++) {
    blockList[inode->blockPointers[i]] = 0;
  }

  // Step 3: mark inode as free
  // Set the "used" field to 0.
  inode->used = 0;

  // Step 4: Write out the inode and free block list to disk
  // Move the file pointer to the start of the file
  // Write out the 128 byte free block list
  // Move the file pointer to the position on disk where this inode was stored
  // Write out the inode
  disk.seekg(0);
  disk.write(blockList, 128);
  disk.seekg(128 + nodes * 48);
  memcpy(&currNode, inode, 48);
  disk.write(currNode, 48);
  disk.flush();
  free(inode);

  return 0;
} // End Delete


int myFileSystem::ls(){
  // List names of all files on disk

  // Step 1: read in each inode and print
  // Move file pointer to the position of the 1st inode (129th byte)
  // for(i=0;i<16;i++)
  // Read in an inode
  // If the inode is in-use
  // print the "name" and "size" fields from the inode
  // end for
  disk.sync();
  char currNode[48];
  idxNode* inode = (idxNode* )malloc(sizeof(idxNode));
  for(int i = 0; i < 16; i++) {
    disk.seekg(128 + i * 48);
    disk.read(currNode, 48);
    memcpy(&currNode, inode, 48);
    if(inode->used) printf("%s %d\n", inode->name, inode->size);
  }
  disk.flush();
  free(inode);

  return 0;
}; // End ls

int myFileSystem::read(char name[8], int blockNum, char buf[1024]){
  // read this block from this file

  // Step 1: locate the inode for this file
  // Move file pointer to the position of the 1st inode (129th byte)
  // Read in an inode
  // If the inode is in use, compare the "name" field with the above file
  // If the file names don't match, repeat
  disk.sync();
  int nodes = 0;
  char currNode[48];
  idxNode* inode = (idxNode* )malloc(sizeof(idxNode));
  while(true) {
    if(nodes > 15) return -1;
    disk.seekg(128 + nodes * 48);
    disk.read(currNode, 48);
    memcpy(inode, &currNode, 48);
    if(inode->used && strcmp(inode->name, name) == 0) break;
    nodes++;
  }

  // Step 2: Read in the specified block
  // Check that blockNum < inode.size, else flag an error
  // Get the disk address of the specified block
  // That is, addr = inode.blockPointer[blockNum]
  // Move the file pointer to the block location (i.e., to byte #
  //   addr*1024 in the file)
  if(blockNum >= inode->size) return -1;
  disk.seekg(1024 * inode->blockPointers[blockNum]);

  // Read in the block => Read in 1024 bytes from this location
  //   into the buffer "buf"
  disk.read(buf, 1024);
  disk.flush();
  free(inode);
  
  return 0;
} // End read


int myFileSystem::write(char name[8], int blockNum, char buf[1024]){
  // write this block to this file

  // Step 1: locate the inode for this file
  // Move file pointer to the position of the 1st inode (129th byte)
  // Read in a inode
  // If the inode is in use, compare the "name" field with the above file
  // If the file names don't match, repeat
  disk.sync();
  int nodes = 0;
  char currNode[48];
  idxNode* inode = (idxNode* )malloc(sizeof(idxNode));
  while(true) {
    if(nodes > 15) return -1;
    disk.seekg(128 + nodes * 48);
    disk.read(currNode, 48);
    memcpy(inode, &currNode, 48);
    if(inode->used && strcmp(inode->name, name) == 0) break;
    nodes++;
  }

  // Step 2: Write to the specified block
  // Check that blockNum < inode.size, else flag an error
  // Get the disk address of the specified block
  // That is, addr = inode.blockPointer[blockNum]
  // Move the file pointer to the block location (i.e., byte # addr*1024)
  if(blockNum >= inode->size) return -1;
  disk.seekg(1024 * inode->blockPointers[blockNum]);

  // Write the block! => Write 1024 bytes from the buffer "buff" to
  //   this location
  disk.write(buf, 1024);
  disk.flush();
  free(inode);

  return 0;
} // end write

int myFileSystem::close_disk(){
  disk.close();
  return 0;
}