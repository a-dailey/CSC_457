#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include "min.h"

int part, subpart;
bool verbose, partition_table_exists;

struct partition_entry partitions[NUM_PARTITIONS];
struct partition_entry subpartitions[NUM_PARTITIONS];



int main(int argc, char *argv[]) {
    part = -1;
    subpart = -1;
    verbose = false;
    partition_table_exists = false;

    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc) {
                print_usage();
                return EXIT_FAILURE;
            }
            //get specified part
            part = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-s") == 0) {
            if (i + 1 >= argc) {
                print_usage();
                return EXIT_FAILURE;
            }
            //get specified subpart
            subpart = atoi(argv[i + 1]);
            i++;
        }
    }

    //get imagefile path and check if it exists
    char *imagefile = argv[argc - 1];
    int fd = open(imagefile, O_RDONLY);
    if (fd < 0) {
        perror("Error opening image file");
        return EXIT_FAILURE;
    }

    //check if partition table exists
    uint32_t partition_offset = 0;

    // Read primary partition table (from MBR)
    read_partition_table(fd, partitions, 0);

    if (!partition_table_exists && (part != -1 || subpart != -1)) {
        fprintf(stderr, "No partition table found but partition or subpartition specified\n");
        close(fd);
        return EXIT_FAILURE;
    }

    // If a partition (`-p`) is specified, use its offset
    if (part != -1) {
        partition_offset = partitions[part].lFirst * SECTOR_SIZE;
        if (verbose) {
            printf("Using partition %d at offset %u bytes\n", part, partition_offset);
        }
        if (partitions[part].type != MINIX_PARTITION_TYPE) {
            fprintf(stderr, "Partition %d is not a Minix partition\n", part);
            close(fd);
            return EXIT_FAILURE;
        }

        // If a subpartition (`-s`) is specified, read the subpartition table inside the partition
        if (subpart != -1) {
            
            // Read the subpartition table at `partition_offset`
            read_partition_table(fd, subpartitions, partition_offset);

            // Add the subpartition's LBA start to get the final offset
            partition_offset += subpartitions[subpart].lFirst * SECTOR_SIZE;
            
        }
    }


    uint32_t subpartition_offset = 0;
    if (subpart != -1) {
        subpartition_offset = partitions[subpart].lFirst * SECTOR_SIZE;
        if (verbose) {
            printf("Using subpartition %d at offset %u bytes\n", subpart, subpartition_offset);
        }
    }
    
    struct superblock sb;
    read_superblock(fd, &sb, partition_offset);
    

    //check if magic number is valid
    if (sb.magic != MINIX_MAGIC_NUM) {
        fprintf(stderr, "Invalid Minix filesystem (magic number: 0x%04X)\n", sb.magic);
        close(fd);
        return EXIT_FAILURE;
    }

    printf("Superblock Contents:\n");
    printf("Number of inodes: %u\n", sb.ninodes);
    printf("Number of zones: %u\n", sb.zones);
    printf("Block size: %u\n", sb.blocksize);
    printf("First data zone: %u\n", sb.firstdata);
    printf("Filesystem magic: 0x%04X\n", sb.magic);

    list_directory(fd, &sb, partition_offset, argv[argc - 1]);

    close(fd);
    return EXIT_SUCCESS;
}

void print_usage() {
    printf("Usage: myminls [ -v ] [ -p part [ -s subpart ] ] imagefile [ path ]\n");
}

int read_partition_table(int fd, struct partition_entry partitions[NUM_PARTITIONS], uint32_t base_offset) {
    uint8_t boot_block[BOOT_BLOCK_SIZE];

    // Read partition table from the given offset (0 for MBR, partition_offset for subpartition tables)
    if (pread(fd, boot_block, BOOT_BLOCK_SIZE, base_offset) != BOOT_BLOCK_SIZE) {
        perror("Error reading partition table");
        return -1;
    }

    // Validate signature
    if (boot_block[510] != VALID_BYTE_510 || boot_block[511] != VALID_BYTE_511) {
        fprintf(stderr, "No valid partition table found.\n");
        return -1;
    }

    // Copy 4 partition entries
    partition_table_exists = true;
    memcpy(partitions, &boot_block[PARTITION_TABLE_OFFSET], NUM_PARTITIONS * sizeof(struct partition_entry));

    return 0;
}

int read_superblock(int fd, struct superblock *sb, uint32_t partition_offset) {
    //get correct superblock offset
    off_t superblock_offset = partition_offset + BOOT_BLOCK_SIZE;

    //Seek to the superblock within the selected partition
    if (lseek(fd, superblock_offset, SEEK_SET) < 0) {
        perror("Error seeking to superblock");
        return -1;
    }

    //Read the superblock
    if (read(fd, sb, sizeof(*sb)) != sizeof(*sb)) {
        perror("Error reading superblock");
        return -1;
    }

    return 0;
}

int list_directory(int fd, struct superblock *sb, uint32_t partition_offset, char *path) {
    // off_t inode_table_offset = partition_offset + BOOT_BLOCK_SIZE + SUPERBLOCK_SIZE + 
    //     (sb->i_blocks * sb->blocksize) + 
    //     (sb->z_blocks * sb->blocksize);
    printf("Using partition offset %u\n", partition_offset);
    off_t inode_table_offset = partition_offset + (2 + sb->i_blocks + sb->z_blocks) * sb->blocksize;
    off_t root_inode_offset = inode_table_offset + (1 - 1) * sizeof(struct inode); // Root inode (inode 1)

    // Seek to the inode
    if (lseek(fd, root_inode_offset, SEEK_SET) < 0) {
        perror("Error seeking to inode");
        return -1;
    }

    struct inode inode;
    if (read(fd, &inode, sizeof(inode)) != sizeof(inode)) {
        perror("Error reading inode");
        return -1;
    }

    // Debugging: Print root inode information
    print_inode(&inode);

    // Ensure the inode is a directory
    if ((inode.mode & MINIX_FILE_TYPE_MASK) != MINIX_DIR_TYPE) {
        fprintf(stderr, "Error: Root inode is not a directory!\n");
        return -1;
    }

    printf("/:\n");
    struct dir_entry entry;

    for (int i = 0; i < DIRECT_ZONES; i++) {
        if (inode.zone[i] == 0) {
            printf("Inode zone %d is empty\n", i);
            continue;
        }

        uint32_t zone_size = (sb->blocksize << sb->log_zone_size);
        uint32_t zone_offset = partition_offset + (inode.zone[i] * zone_size);

        for (uint32_t j = 0; j < sb->blocksize / sizeof(struct dir_entry); j++) {
            lseek(fd, zone_offset + j * sizeof(struct dir_entry), SEEK_SET);
            read(fd, &entry, sizeof(struct dir_entry));

            if (entry.inode == 0) {
                //printf("Skipping empty entry\n");
                continue;
            }
            printf("  %s (inode %u)\n", entry.name, entry.inode);
        }
    }

    return 0;
}




void print_inode(struct inode *inode) {
    printf("Inode info:\n");
    printf("  Links: %u\n", inode->links);
    printf("  UID: %u\n", inode->uid);
    printf("  GID: %u\n", inode->gid);
    printf("  Mode: 0x%X\n", inode->mode);
    printf("  Size: %u bytes\n", inode->size);
    printf("  Zones: [");
    for (int i = 0; i < DIRECT_ZONES; i++) {
        printf(" %u", inode->zone[i]);
    }
    printf(" ]\n");
}