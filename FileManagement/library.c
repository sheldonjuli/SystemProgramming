#include "library.h"

/*
 * Open the disk given diskname.
 */
unsigned char *open_disk(char *diskname) {

    int fd = open(diskname, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "ext2_cp: cannot copy file: Target disk not found\n");
        exit(ENOENT);
    }

    unsigned char *disk = mmap(NULL, NUM_BLOCKS * EXT2_BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED) {
        perror("mmap");
        exit(ENOENT);
    }

    return disk;
}

/*
 * Get super block of the disk.
 */
struct ext2_super_block *get_super_block(unsigned char *disk) {
    return (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
}

/*
 * Get group descriptor of the disk
 */
struct ext2_group_desc *get_group_desc(unsigned char *disk) {
    return (struct ext2_group_desc *)(disk + 2 * EXT2_BLOCK_SIZE);
}

/*
 * Get inode bitmap of the disk
 */
unsigned char *get_inode_bitmap(unsigned char *disk, struct ext2_group_desc *gd) {
    return (unsigned char *)disk + gd->bg_inode_bitmap * EXT2_BLOCK_SIZE;
}

/*
 * Get block bitmap of the disk
 */
unsigned char *get_block_bitmap(unsigned char *disk, struct ext2_group_desc *gd) {
    return (unsigned char *)disk + gd->bg_block_bitmap * EXT2_BLOCK_SIZE;
}

/*
 * Get inode table of the disk
 */
struct ext2_inode *get_inode_tbl(unsigned char *disk, struct ext2_group_desc *gd) {
    return (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gd->bg_inode_table);
}

/*
 * Get inode given inode index
 * Index starts at 1
 */
struct ext2_inode *get_inode_at(struct ext2_inode *inode_tbl, int idx) {
    return (struct ext2_inode *)(inode_tbl + idx - 1);
}

/*
 * Get block given block index
 */
char *get_block_at(unsigned char *disk, int idx) {
    return (char *)(disk + EXT2_BLOCK_SIZE * idx);
}

/*
 * Get inode bit value of inode index
 * Index starts at 1
 */
int get_inode_bit_val_at(unsigned char *disk, struct ext2_group_desc *gd, int idx) {
    idx--;
    unsigned char *inode_bitmap = get_inode_bitmap(disk, gd);
    return inode_bitmap[idx / 8] & (1 << idx % 8);
}

/*
 * Get block bit value of block index
 */
int get_block_bit_val_at(unsigned char *disk, struct ext2_group_desc *gd, int idx) {
    idx--;
    unsigned char *block_bitmap = get_block_bitmap(disk, gd);
    return block_bitmap[idx / 8] & (1 << idx % 8);
}

/*
 * Get the directory entry at given block
 */
struct ext2_dir_entry *get_dir_entry_at(unsigned char *disk, int block_idx) {
    return (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * block_idx);
}

/*
 * Check if the given path is an absolute path.
 * Exit with EINVAL if not
 */
void check_absolute_path(char *method_name, char *dirpath) {
    if(dirpath[0] != '/') {
        fprintf(stderr, "%s: expecting absolute path for target\n", method_name);
        exit(ENOENT);
    }
}

/*
 * Check filename operable
 */
int check_filename_operable(char *file_name) {
    if ((strcmp(file_name, ".") == 0) || (strcmp(file_name, "..") == 0)) {
        return 0;
    }

    return 1;
}

/*
 * Get currect time in ms
 */
unsigned int get_time() {
    
    return (unsigned int) time(NULL);

}

/*
 * Convert i_mode of type unsigned short to file_type of unsigned char
 */
unsigned char i_mode_to_file_type(unsigned short i_mode) {

    if (I_MODE_FILE_TYPE(i_mode) == I_MODE_FILE_TYPE(EXT2_S_IFDIR)) {
        return EXT2_FT_DIR;
    } else if (I_MODE_FILE_TYPE(i_mode) == I_MODE_FILE_TYPE(EXT2_S_IFREG)) {
        return EXT2_FT_REG_FILE;
    } else if (I_MODE_FILE_TYPE(i_mode) == I_MODE_FILE_TYPE(EXT2_S_IFLNK)) {
        return EXT2_FT_SYMLINK;
    } else {
        return EXT2_FT_UNKNOWN;
    }

}

/*
 * Convert i_mode of type unsigned short to a printable char
 */
char i_mode_to_char(unsigned short i_mode) {

    if (I_MODE_FILE_TYPE(i_mode) == I_MODE_FILE_TYPE(EXT2_S_IFDIR)) {
        return 'd';
    } else if (I_MODE_FILE_TYPE(i_mode) == I_MODE_FILE_TYPE(EXT2_S_IFREG)) {
        return 'f';
    } else if (I_MODE_FILE_TYPE(i_mode) == I_MODE_FILE_TYPE(EXT2_S_IFLNK)) {
        return 'l';
    } else {
        return 'u';
    }

}

/*
 * Convert file_type of unsigned char to a printable char
 */
char file_type_to_char(unsigned char file_type) {

    if (FILE_TYPE_LOWER_3(file_type) == FILE_TYPE_LOWER_3(EXT2_FT_DIR)) {
        return 'd';
    } else if (FILE_TYPE_LOWER_3(file_type) == FILE_TYPE_LOWER_3(EXT2_FT_REG_FILE)) {
        return 'f';
    } else if (FILE_TYPE_LOWER_3(file_type) == FILE_TYPE_LOWER_3(EXT2_FT_SYMLINK)) {
        return 'l';
    } else {
        return 'u';
    }
}

/*
 * Return the last file/dir name
 * This method will tokenize the original string, pass in a copy
 */
char *get_file_name(char *path) {

    char *token = strtok(path, "/");
    char *next_token = NULL;
    while (token != NULL) {
        next_token = strtok(NULL, "/");
        if (!next_token) {
            break;
        }

        token = next_token;
    }

    return token;
}

/*
 * Compute the rec_len given name_len
 */
int compute_rec_len(int name_len) {
    return 4 * (((sizeof(struct ext2_dir_entry) + name_len) / 4) + 1);
}

/*
 * Compute the number of blocks needed for a given file size
 */
int get_blocks_needed(int size) {

    int blocks_needed = size / EXT2_BLOCK_SIZE + 1 * ((size % EXT2_BLOCK_SIZE) > 0);
    if (blocks_needed > 12) blocks_needed++;
    return blocks_needed;
}

/*
 * Print out fields in super block for debugging.
 */
void print_super_block(struct ext2_super_block *sb) {
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);
}

/*
 * Print out fields in group descriptor for debugging.
 */
void print_group_desc(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *gd) {
    printf("Block group:\n");
    printf("    Blocks bitmap block: %d\n", gd->bg_block_bitmap);
    printf("    Inodes bitmap block: %d\n", gd->bg_inode_bitmap);
    printf("    Inodes table block: %d\n", gd->bg_inode_table);
    printf("    Free blocks count: %d\n", gd->bg_free_blocks_count);
    printf("    Free inodes count: %d\n", gd->bg_free_inodes_count);
    printf("    Directories count: %d\n", gd->bg_used_dirs_count);

    // print out the block bitmap
    unsigned int i;

    printf("Block bitmap: ");

    char *block_bit = (char *)disk + gd->bg_block_bitmap * 0x400;

    for(; block_bit < (char *)disk + gd->bg_block_bitmap * 0x400 + ((sb -> s_blocks_count) >> 3); ++block_bit) {
        for(i = 1; i <= 0x80; i <<= 1) {
            printf("%d", ((*block_bit) & i) > 0);
        }
        printf(" ");
    }

    printf("\n");

    // print out the inode bitmap
    printf("Inode bitmap: ");

    char *inode_bit = (char *)get_inode_bitmap(disk, gd);

    for(; inode_bit < (char *)disk + gd->bg_inode_bitmap * 0x400 + ((sb -> s_inodes_count) >> 3); ++inode_bit) {
        for(i = 1; i <= 0x80; i <<= 1) {
            printf("%d", ((*inode_bit) & i) > 0);
        }
        printf(" ");
    }

    printf("\n");
}

/*
 * Print out an inode at given index for debugging
 */
void print_inodes(struct ext2_inode *inode_table, unsigned int inode_index) {

    struct ext2_inode *p = inode_table + inode_index - 1;

    char type = i_mode_to_char(p->i_mode);

    // print out the corresponding inode details
    printf("[%d] type: %c size: %d links: %d i_blocks: %d\n", inode_index, type, p -> i_size, p -> i_links_count, p -> i_blocks);
    printf("[%d] Blocks: ", inode_index);

    int i;

    for(i = 0; i < 15; ++i) {       
        printf(" %d", (p -> i_block)[i]);
    }

    printf("\n");

}

/*
 * Print all the directory entries in a given directory
 */
void print_dir_entry(unsigned char *disk, struct ext2_dir_entry *dir_entry) {
    int size = 0;
    while (size < EXT2_BLOCK_SIZE) {
        printf("name %.*s \n", dir_entry->name_len, (char *)dir_entry);
        printf("name_len: %d rec_len: %d\n", dir_entry->name_len, dir_entry->rec_len);
        dir_entry = (void *) dir_entry + dir_entry->rec_len;
        size +=dir_entry->rec_len;
    }
}

/*
 * Counts the number of free inodes in bitmap
 */
unsigned int count_free_inode_in_bitmap(unsigned char *disk, struct ext2_group_desc *gd) {

    unsigned char *inode_bitmap = get_inode_bitmap(disk, gd);
    unsigned int free_count = 0;
    int idx = 0;
    while (idx < 32) {
        if (!(inode_bitmap[idx / 8] & (1 << idx % 8))) {
            free_count++;
        }
        idx++;
    }

    return free_count;
}

/*
 * Counts the number of free block in bitmap
 */
unsigned int count_free_block_in_bitmap(unsigned char *disk, struct ext2_group_desc *gd) {

    unsigned char *block_bitmap = get_block_bitmap(disk, gd);
    unsigned int free_count = 0;
    int idx = 0;
    while (idx < NUM_BLOCKS) {
        if (!(block_bitmap[idx / 8] & (1 << idx % 8))) {
            free_count++;
        }
        idx++;
    }

    return free_count;
}

/*
 * Return the index of the first free inode.
 * Index starts at 1.
 * Assume there is at least 1 free inode.
 */
int find_free_inode_idx(unsigned char *disk, struct ext2_group_desc *gd) {

    unsigned char *inode_bitmap = get_inode_bitmap(disk, gd);
    int idx = 0;
    while (idx < 32) {
        if (!(inode_bitmap[idx / 8] & (1 << idx % 8))) {
            break;
        }
        idx++;
    }

    idx++;
    update_inode_bitmap(disk, gd, idx, 1);
    return idx;
}

/*
 * Update the bit value at given index in inode_bitmap.
 * Index starts at 1.
 */
void update_inode_bitmap(unsigned char *disk, struct ext2_group_desc *gd, int inode_idx, int bit_val) {

    int idx = inode_idx - 1;
    unsigned char *inode_bitmap = get_inode_bitmap(disk, gd);
    struct ext2_super_block *sb = get_super_block(disk);

    if (bit_val) {
        inode_bitmap[idx / 8] |= (1 << idx % 8);
        gd->bg_free_inodes_count--;
        sb->s_free_inodes_count--;
    } else {
        inode_bitmap[idx / 8] &= ~(1 << idx % 8);
        gd->bg_free_inodes_count++;
        sb->s_free_inodes_count++;
    }
}

/*
 * Return the index of the first free block.
 * Index starts at 1.
 * Assume there is at least 1 free block.
 */
int find_free_block_idx(unsigned char *disk, struct ext2_group_desc *gd) {

    unsigned char *block_bitmap = get_block_bitmap(disk, gd);
    int idx = 0;
    while (idx < NUM_BLOCKS) {
        if (!(block_bitmap[idx / 8] & (1 << idx % 8))) {
            break;
        }
        idx++;
    }

    idx++;
    update_block_bitmap(disk, gd, idx, 1);
    return idx;
}

/*
 * Update the bit value at given index in block_bitmap.
 * Index starts at 1.
 */
void update_block_bitmap(unsigned char *disk, struct ext2_group_desc *gd, int block_idx, int bit_val) {

    int idx = block_idx - 1;
    unsigned char *block_bitmap = get_block_bitmap(disk, gd);
    struct ext2_super_block *sb = get_super_block(disk);

    if (bit_val) {
        block_bitmap[idx / 8] |= (1 << idx % 8);
        gd->bg_free_blocks_count--;
        sb->s_free_blocks_count--;
    } else {
        block_bitmap[idx / 8] &= ~(1 << idx % 8);
        gd->bg_free_blocks_count++;
        sb->s_free_blocks_count++;
    }
}

/*
 * Find the inode index of the directory/file with given target_name in the current directory curr_dir
 * Returns -1 if no such directory/file exits.
 */
int get_file_inode_idx(struct ext2_dir_entry *curr_dir, struct ext2_inode *curr_inode, char *target_name, unsigned char target_type) {

    unsigned int size = 0;
    char file_name[EXT2_NAME_LEN + 1];

    while (size < curr_inode->i_size) {

        memcpy(file_name, curr_dir->name, curr_dir->name_len);
        file_name[curr_dir->name_len] = 0;
        if (target_type == EXT2_FT_UNKNOWN) {
            if (strcmp(file_name, target_name) == 0) {
                return curr_dir->inode;
            }    
        } else {
            if ((strcmp(file_name, target_name) == 0) && (curr_dir->file_type == target_type)) {
                return curr_dir->inode;
            }
        }


        size += curr_dir->rec_len;
        curr_dir = (void *) curr_dir + curr_dir->rec_len;
    }

    return -1;

}

/*
 * Initialize a directory block
 */
void init_dir_block(struct ext2_inode *entry_inode, int new_inode_idx, struct ext2_dir_entry *new_dir) {
    new_dir->inode = new_inode_idx;
    new_dir->rec_len = EXT2_BLOCK_SIZE;
    new_dir->name_len = 0;
    new_dir->file_type = EXT2_FT_DIR;
}

/*
 * Create a new inode, initialize all properties
 */
int create_inode(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, unsigned short mode, unsigned int size) {

    int blocks_needed = get_blocks_needed(size);
    int new_inode_idx = find_free_inode_idx(disk, gd);
    struct ext2_inode *new_inode = get_inode_at(inode_tbl, new_inode_idx);
    new_inode->i_mode = (new_inode->i_mode & 0) | mode;
    new_inode->i_uid = 0;
    new_inode->i_size = size;
    new_inode->i_links_count = 0;
    new_inode->i_blocks = 2 * blocks_needed; // 1 blocked used, each disk section is 512
    new_inode->i_dtime = 0;

    int i = 0;
    if (mode & EXT2_S_IFDIR) {

        new_inode->i_block[0] = find_free_block_idx(disk, gd);
        struct ext2_dir_entry *new_dir = get_dir_entry_at(disk, new_inode->i_block[0]);
        init_dir_block(new_inode, new_inode_idx, new_dir);

        for (i = 1; i < 15; i++) {
            new_inode->i_block[i] = 0;
        }

        gd->bg_used_dirs_count++;

    } else if (mode & EXT2_S_IFREG) {

        new_inode->i_links_count++;

        if (blocks_needed < 13) {
            for (i = 0; i < blocks_needed; i++) {
                new_inode->i_block[i] = find_free_block_idx(disk, gd);
            }
            for (i = blocks_needed; i < 15; i++) {
                new_inode->i_block[i] = 0;
            }
        } else {
            for (i = 0; i < 13; i++) { // block 13 is an indirect block
                new_inode->i_block[i] = find_free_block_idx(disk, gd);
            }

            int *indirect_block = (int *)get_block_at(disk, new_inode->i_block[12]);
            for (i = 13; i < blocks_needed; i++) {
                *indirect_block = find_free_block_idx(disk, gd);
                indirect_block++;
            }

            *indirect_block = 0;

            // does not support 2nd or 3rd level indirect
            new_inode->i_block[13] = 0;
            new_inode->i_block[14] = 0;
        }

    }

    return new_inode_idx;

}


/*
 * Insert the given dir/file name into the given parent directory
 */
void insert_into_dir(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int parent_inode_idx, int new_inode_idx, char *new_name, char type) {

    struct ext2_inode *parent_inode = get_inode_at(inode_tbl, parent_inode_idx);

    // if insert a directory, there will be a parent link
    if (type == EXT2_FT_DIR) {
        parent_inode->i_links_count++;
    }

    // find the last block
    int i_block_idx = 0;
    for (i_block_idx= 0; i_block_idx < 15; i_block_idx++) {
        if (parent_inode->i_block[i_block_idx] == 0) {
            break;
        }
    }
    i_block_idx -= 1;

    // find the last entry in the currect block
    unsigned int size = 0;
    unsigned int last_rec_len = 0; // record length of the last entry
    int new_name_len = strlen(new_name);
    int new_rec_len = compute_rec_len(new_name_len);

    struct ext2_dir_entry *curr_dir = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * (parent_inode->i_block[i_block_idx]));

    while (size < EXT2_BLOCK_SIZE) {
        last_rec_len = curr_dir->rec_len;
        size += curr_dir->rec_len;
        curr_dir = (void *) curr_dir + curr_dir->rec_len;
    }

    // jump to the last record
    curr_dir = get_dir_entry_at(disk, parent_inode->i_block[i_block_idx]);
    curr_dir = (void *) curr_dir + (EXT2_BLOCK_SIZE - last_rec_len);

    // not enough space, allocate a new block
    if (!((curr_dir->name_len + new_rec_len) <= curr_dir->rec_len)) {
        i_block_idx++;
        parent_inode->i_block[i_block_idx] = find_free_block_idx(disk, gd);
        curr_dir = get_dir_entry_at(disk, parent_inode->i_block[i_block_idx]);
        init_dir_block(parent_inode, parent_inode_idx, curr_dir);
        parent_inode->i_size += EXT2_BLOCK_SIZE;
        parent_inode->i_blocks += 2;
    }

    int old_rec_len = curr_dir->rec_len;
    int prev_entry_rec_len = 0;
    if (curr_dir->name_len) {
        prev_entry_rec_len = compute_rec_len(curr_dir->name_len);
        curr_dir->rec_len = prev_entry_rec_len;
        curr_dir = (void *) curr_dir + curr_dir->rec_len;
    }

    curr_dir->inode = new_inode_idx;
    curr_dir->name_len = new_name_len;
    curr_dir->rec_len = old_rec_len - prev_entry_rec_len;
    curr_dir->file_type = type;
    strcpy(curr_dir->name, new_name);

}

/*
 * Remove the given dir/file name into the given parent directory
 */
void remove_from_dir(unsigned char *disk, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int parent_inode_idx, char *target_name) {

    struct ext2_inode *parent_inode = get_inode_at(inode_tbl, parent_inode_idx);
    char file_name[EXT2_NAME_LEN + 1];
    struct ext2_dir_entry *curr_dir;
    struct ext2_dir_entry *prev_dir;
    int prev_dir_rec_len = 0;

    // find the last block
    int i_block_idx = 0;
    int block_local = 0;
    for (i_block_idx= 0; i_block_idx < 12; i_block_idx++) {
        block_local = parent_inode->i_block[i_block_idx];
        if (block_local != 0) {
            curr_dir = get_dir_entry_at(disk, block_local);
            int size = 0;
            while (size < EXT2_BLOCK_SIZE) {
                memcpy(file_name, curr_dir->name, curr_dir->name_len);
                file_name[curr_dir->name_len] = 0;
                if (strcmp(file_name, target_name) == 0) {
                    prev_dir= (void *) curr_dir - prev_dir_rec_len;
                    prev_dir->rec_len += curr_dir->rec_len;
                    break;
                }
                size += curr_dir->rec_len;
                prev_dir_rec_len = curr_dir->rec_len;
                curr_dir = (void *) curr_dir + curr_dir->rec_len;
            }
        }
    }

}


/*
 * Create a regular file in the given parent directory with given name
 * Return the new inode index
 */
int create_file(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int parent_inode_idx, char *new_name, unsigned int size) {

    int new_inode_idx = create_inode(disk, sb, gd, inode_tbl, EXT2_S_IFREG, size);
    insert_into_dir(disk, sb, gd, inode_tbl, parent_inode_idx, new_inode_idx, new_name, EXT2_FT_REG_FILE);

    return new_inode_idx;
}

/*
 * Create a new directory in the given parent directory
 * Return the new inode index
 */
int create_dir(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int parent_inode_idx, char *new_name) {

    int new_inode_idx = create_inode(disk, sb, gd, inode_tbl, EXT2_S_IFDIR, EXT2_BLOCK_SIZE);
    insert_into_dir(disk, sb, gd, inode_tbl, parent_inode_idx, new_inode_idx, new_name, EXT2_FT_DIR);
    insert_into_dir(disk, sb, gd, inode_tbl, new_inode_idx, new_inode_idx, ".", EXT2_FT_DIR);
    insert_into_dir(disk, sb, gd, inode_tbl, new_inode_idx, parent_inode_idx, "..", EXT2_FT_DIR);

    return new_inode_idx;
}

/*
 * Create a symbolic(soft) link in the given parent directory with given name
 * Return the new inode index
 */
int create_symbolic_link(unsigned char *disk, struct ext2_super_block *sb, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int parent_inode_idx, char *new_name, char *dirpath_file) {

    int new_inode_idx = create_inode(disk, sb, gd, inode_tbl, EXT2_S_IFLNK, strlen(dirpath_file));
    insert_into_dir(disk, sb, gd, inode_tbl, parent_inode_idx, new_inode_idx, new_name, EXT2_FT_SYMLINK);

    struct ext2_inode *new_inode = get_inode_at(inode_tbl, new_inode_idx);
    char *link_block = get_block_at(disk, new_inode->i_block[0]);
    strcpy(link_block, dirpath_file);

    return new_inode_idx;
}

/*
 * Find the inode index of the last file/directory in the path and its parent
 * Return the [parent_idx, idx] if target found
 * Return [parent_idx, -1] if path found but target not found
 * Return [-1, -1] if path not found
 */
int *get_last_file_and_parent_inode_idx(unsigned char *disk, struct ext2_inode *inode_tbl, struct ext2_dir_entry *start_dir, struct ext2_inode *start_inode, char *path, unsigned char target_type) {

    int *indices = malloc(sizeof(int) * 2);
    struct ext2_inode *curr_inode = start_inode;
    struct ext2_dir_entry *curr_dir = start_dir;
    int parent_idx = EXT2_ROOT_INO;
    int idx = EXT2_ROOT_INO;
    char *path_copy = (char *)malloc(sizeof(char) * strlen(path)); // don't tokenize the actual path
    strcpy(path_copy, path);
    char *token = strtok(path_copy, "/");
    char *next_token = NULL;
    while (token != NULL) {
        next_token = strtok(NULL, "/");

        if (next_token) {
            // a dir along the path
            idx = get_file_inode_idx(curr_dir, curr_inode, token, EXT2_FT_DIR);
            if (idx == -1) {
                parent_idx = -1;
                idx = -1;
                break;
            }
        } else {
            // last file/dir
            idx = get_file_inode_idx(curr_dir, curr_inode, token, target_type);
            break;
        }

        token = next_token;
        curr_inode = inode_tbl + idx - 1;
        curr_dir = get_dir_entry_at(disk, curr_inode->i_block[0]);
        parent_idx = idx;
    }

    indices[0] = parent_idx;
    indices[1] = idx;
    free(path_copy);
    return indices;
}

/*
 * Update free inode/block count in sb
 */
int update_sb_free_count(unsigned int *target, unsigned int val, char *target_data_name, int print) {

    int num_off = 0;
    if (*target != val) {
        
        num_off = *target - val;
        if (num_off < 0) {
            num_off = -num_off;
        }
        if (print) {
            printf("Fixed: superblock's %s counter was off by %d compared to the bitmap\n", target_data_name, num_off);           
        }

        *target = val;
    }

    return num_off;
}

/*
 * Update free inode/block count in group descriptor
 */
int update_gd_free_count(unsigned short *target, unsigned short val, char *target_data_name, int print) {

    int num_off = 0;
    if (*target != val) {

        num_off = *target - val;
        if (num_off < 0) {
            num_off = -num_off;
        }
        if (print) {
            printf("Fixed: block group's %s counter was off by %d compared to the bitmap\n", target_data_name, num_off);           
        }

        *target = val;
    }

    return num_off;
}

/*
 * Recursively remove a directory/file
 */
void recursively_remove(unsigned char *disk, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int parent_inode_idx, int target_inode_idx, char *target_name) {

    struct ext2_inode *curr_inode = get_inode_at(inode_tbl, target_inode_idx);

    if (I_MODE_FILE_TYPE(curr_inode->i_mode) == I_MODE_FILE_TYPE(EXT2_S_IFDIR)) {

        gd->bg_used_dirs_count--;
        struct ext2_inode *parent_inode = get_inode_at(inode_tbl, parent_inode_idx);
        parent_inode->i_links_count--;

        struct ext2_dir_entry *curr_dir = get_dir_entry_at(disk, curr_inode->i_block[0]);
        int size = 0;
        char file_name[EXT2_NAME_LEN+1];
        while (size < curr_inode->i_size) {

            memcpy(file_name, curr_dir->name, curr_dir->name_len);
            file_name[curr_dir->name_len] = 0;
            //skip . ..
            if (check_filename_operable(file_name)) {
                recursively_remove(disk, gd, inode_tbl, parent_inode_idx, curr_dir->inode, file_name);
            }
            size += curr_dir->rec_len;
            curr_dir = (void *) curr_dir + curr_dir->rec_len;

        }
    }

    // remove dir/file name first
    remove_from_dir(disk, gd, inode_tbl, parent_inode_idx, target_name);

    // zero out block index, update bitmap
    // check indirect first
    int indirect_idx = curr_inode->i_block[12];
    if (indirect_idx) {
        int *indirect_block = (int *)get_block_at(disk, indirect_idx);
        while (*indirect_block != 0) {
            update_block_bitmap(disk, gd, *indirect_block, 0);
            indirect_block++;
        }

        update_block_bitmap(disk, gd, indirect_idx, 0);
    }

    int block_idx = 0;
    int block_local = 0;
    for (block_idx = 0; block_idx < 12; block_idx++) {
        block_local = curr_inode->i_block[block_idx];
        if (block_local) {
            update_block_bitmap(disk, gd, block_local, 0);
        }
    }

    // d, fix inode's i_dtime
    curr_inode->i_dtime = get_time();

    // zero out inode, update bitmap
    update_inode_bitmap(disk, gd, target_inode_idx, 0);
}

/*
 * Recursively remove a directory/file
 */
int recursively_restore(unsigned char *disk, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int parent_inode_idx, char *target_name, int r_flag) {
    // search parent directory
    struct ext2_inode *parent_inode = get_inode_at(inode_tbl, parent_inode_idx);
    struct ext2_dir_entry *curr_dir = get_dir_entry_at(disk, parent_inode->i_block[0]);
    int curr_dir_rec_len_copy;

    // if found out target was a directory, but r_flag wasn't used, return ENOENT
    int target_inode_idx;
    int size = 0;
    char file_name[EXT2_NAME_LEN + 1];
    while (size < EXT2_BLOCK_SIZE) {

        // find & check
        int required_rec_len = compute_rec_len(curr_dir->name_len);
        int target_rec_len = compute_rec_len(strlen(target_name));
        if (curr_dir->rec_len >= required_rec_len + target_rec_len) { // no need to check every gap
            curr_dir = (void *) curr_dir + required_rec_len;
            memcpy(file_name, curr_dir->name, curr_dir->name_len);
            file_name[curr_dir->name_len] = 0;
            if (strcmp(file_name, target_name) == 0) {
                target_inode_idx = curr_dir->inode;
                // it is easier to restore it now and remove it later if the file can't be restored
                curr_dir = (void *) curr_dir - required_rec_len; // retreat
                curr_dir_rec_len_copy = curr_dir->rec_len;
                curr_dir->rec_len = required_rec_len;
                break;
            } else {
                curr_dir = (void *) curr_dir - required_rec_len; // retreat
            }

        }
        size += curr_dir->rec_len;
        curr_dir = (void *) curr_dir + curr_dir->rec_len;

    }

    struct ext2_inode *target_inode;
    int i_block_idx;
    // recursively checking all criterias
    if (!(target_inode_idx >= EXT2_GOOD_OLD_FIRST_INO)) { // didn't find the entry
        fprintf(stderr, "ext2_restore: cannot restore: Target not found\n");
        return ENOENT;
    } else {
        if (get_inode_bit_val_at(disk, gd, target_inode_idx)) { // inode already in use
            curr_dir->rec_len = curr_dir_rec_len_copy;
            return ENOENT;
        } else {
            target_inode = get_inode_at(inode_tbl, target_inode_idx);
            for (i_block_idx = 0; i_block_idx < 15; i_block_idx++) {
                if ((target_inode->i_block[i_block_idx] != 0) && get_block_bit_val_at(disk, gd, target_inode->i_block[i_block_idx])) { // some blocks are already in use
                    curr_dir->rec_len = curr_dir_rec_len_copy;
                    return ENOENT;
                } else {
                    if (I_MODE_FILE_TYPE(target_inode->i_mode) == I_MODE_FILE_TYPE(EXT2_S_IFDIR) && !r_flag) {
                        fprintf(stderr, "ext2_restore: cannot restore: Target is a directory, must use [-r]\n");
                        curr_dir->rec_len = curr_dir_rec_len_copy;
                        return EISDIR;
                    }
                }
            }
        }
    }

    // flip all bits
    update_inode_bitmap(disk, gd, target_inode_idx, 1);
    target_inode->i_dtime = 0;
    target_inode->i_links_count++;
    for (i_block_idx = 0; i_block_idx < 15; i_block_idx++) {
        if (target_inode->i_block[i_block_idx] != 0) { // some blocks are already in use
            update_block_bitmap(disk, gd, target_inode->i_block[i_block_idx], 1);
        }
    }

    int indirect_idx = target_inode->i_block[12];
    if (indirect_idx) {
        int *indirect_block = (int *)get_block_at(disk, indirect_idx);
        while (*indirect_block != 0) {
            update_block_bitmap(disk, gd, *indirect_block, 1);
            indirect_block++;
        }
    }

    return 0;
}

/*
 * Recursively fix inconsistencies
 */
int iterate_and_fix_files(unsigned char *disk, struct ext2_group_desc *gd, struct ext2_inode *inode_tbl, int start_inode_idx) {

    int total_fixes = 0;
    struct ext2_inode *curr_inode = get_inode_at(inode_tbl, start_inode_idx);
    struct ext2_dir_entry *entry = get_dir_entry_at(disk, curr_inode->i_block[0]);
    
    // b, check mode matches
    if (i_mode_to_file_type(curr_inode->i_mode) != entry->file_type) {
        entry->file_type = i_mode_to_file_type(curr_inode->i_mode);
        printf("Fixed: Entry type vs inode mismatch: inode [%d]\n", start_inode_idx);
        total_fixes++;
    }

    // c, check inode bitmap marked
    if (!(get_inode_bit_val_at(disk, gd, start_inode_idx))) {
        update_inode_bitmap(disk, gd, start_inode_idx, 1);
        printf("Fixed: inode [%d] not marked as in-use\n", start_inode_idx);
        total_fixes++;
    }

    // d, fix inode's i_dtime
    if (curr_inode->i_dtime != 0) {
        curr_inode->i_dtime = 0;
        printf("Fixed: valid inode marked for deletion: [%d]\n", start_inode_idx);
        total_fixes++;
    }

    if (I_MODE_FILE_TYPE(curr_inode->i_mode) == I_MODE_FILE_TYPE(EXT2_S_IFDIR)) {

        struct ext2_dir_entry *curr_dir = get_dir_entry_at(disk, curr_inode->i_block[0]);
        int size = 0;
        char file_name[EXT2_NAME_LEN + 1];
        while (size < EXT2_BLOCK_SIZE) {

            memcpy(file_name, curr_dir->name, curr_dir->name_len);
            file_name[curr_dir->name_len] = 0;
            //skip . ..
            if (check_filename_operable(file_name) ) {
                total_fixes += iterate_and_fix_files(disk, gd, inode_tbl, curr_dir->inode);
            }
            size += curr_dir->rec_len;
            curr_dir = (void *) curr_dir + curr_dir->rec_len;

        }
    }

    // e, check block bitmap marked
    int block_idx = 0;
    int block_fixes = 0;
    int block_local = 0;
    for (block_idx = 0; block_idx < 13; block_idx++) { // indirect block itself needs to be checked
        block_local = curr_inode->i_block[block_idx];
        if (block_local) {
            if (!(get_block_bit_val_at(disk, gd, block_local))) {
                update_block_bitmap(disk, gd, block_local, 1);
                block_fixes++;
            }
        }
    }

    // check indirect
    int indirect_idx = curr_inode->i_block[12];
    if (indirect_idx) {
        int *indirect_block = (int *)get_block_at(disk, indirect_idx);
        while (*indirect_block != 0) {
            if (!(get_block_bit_val_at(disk, gd, *indirect_block))) {
                update_block_bitmap(disk, gd, *indirect_block, 1);
                block_fixes++;
            }
            indirect_block++;
        }
    }

    if (block_fixes) {
        printf("Fixed: %d in-use data blocks not marked in data bitmap for inode: [%d]\n", block_fixes, start_inode_idx);       
    }

    total_fixes += block_fixes;

    return total_fixes;
}