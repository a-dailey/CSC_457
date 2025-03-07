#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//partition stuff
#define PARTITION_TABLE_OFFSET 0x1BE
#define MINIX_PARTITION_TYPE 0x81
#define VALID_BYTE_510 0x55
#define VALID_BYTE_511 0xAA
#define NUM_PARTITIONS 4

#define SECTOR_SIZE 512

//inode stuff
#define MINIX_MAGIC_NUM 0x4D5A
#define MINIX_MAGIC_NUM_REV 0x5A4D
#define INODE_SIZE 64
#define SUPERBLOCK_SIZE 1024
#define DIR_ENTRY_SIZE 64
#define DIRECT_ZONES 7
#define MAX_PATH 256
#define ROOT_INODE 1

#define BOOT_BLOCK_SIZE 1024
#define BOOT_BLOCK_OFFSET 0

//directory stuff
#define DIRECTORY_ENTRY_SIZE 64
#define MINIX_FILE_TYPE_MASK  0170000  // Octal (0xF000 in hex)
#define MINIX_DIR_TYPE 0040000  // Octal (0x4000 in hex)



struct superblock { /* Minix Version 3 Superblock
    * this structure found in fs/super.h
    * in minix 3.1.1
    */
    /* on disk. These fields and orientation are non–negotiable */
    uint32_t ninodes; /* number of inodes in this filesystem */
    uint16_t pad1; /* make things line up properly */
    int16_t i_blocks; /* # of blocks used by inode bit map */
    int16_t z_blocks; /* # of blocks used by zone bit map */
    uint16_t firstdata; /* number of first data zone */
    int16_t log_zone_size; /* log2 of blocks per zone */
    int16_t pad2; /* make things line up again */
    uint32_t max_file; /* maximum file size */
    uint32_t zones; /* number of zones on disk */
    int16_t magic; /* magic number */
    int16_t pad3; /* make things line up again */
    uint16_t blocksize; /* block size in bytes */
    uint8_t subversion; /* filesystem sub–version */
    } __attribute__((packed));
    

struct partition_entry {
    uint8_t bootind; /* boot indicator 0/ACTIVE_FLAG */
    uint8_t start_head; /* start of partition in CHS */
    uint8_t start_sec;
    uint8_t start_cyl;
    uint8_t type; /* type of partition */
    uint8_t end_head; /* end of partition in CHS */
    uint8_t end_sec;
    uint8_t end_cyl;
    uint32_t lFirst; /* first sector (LBA) */
    uint32_t size; /* size of partition in sectors */
} __attribute__((packed));

struct inode {
    uint16_t mode; /* mode */
    uint16_t links; /* number or links */
    uint16_t uid;
    uint16_t gid;
    uint32_t size;
    int32_t atime;
    int32_t mtime;
    int32_t ctime;
    uint32_t zone[DIRECT_ZONES];
    uint32_t indirect;
    uint32_t two_indirect;
    uint32_t unused;
    } __attribute__((packed));


struct dir_entry {
    uint32_t inode;       // Inode number
    char name[60];        // File name (null-terminated if <60 chars)
} __attribute__((packed));
    

void print_usage();
void print_inode(struct inode *inode);
int read_partition_table(int fd, struct partition_entry partitions[NUM_PARTITIONS], uint32_t base_offset);
int read_superblock(int fd, struct superblock *sb, uint32_t partition_offset);
int list_directory(int fd, struct superblock *sb, uint32_t partition_offset, char *path);