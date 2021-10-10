#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "ext2.h"

struct disk {
    int         fd;
    uint64_t    length;
};


struct ext2fs {
    struct ext2_superblock      superblock;
    size_t                      bsize;
    uint64_t                    bg_start;
    struct ext2_bg_desc     *   group_descriptors;
    uint8_t                 *   block_cache;
};

#define EXT2_INODE_SIZE     128
#define EXT2_BLOCK_SIZE     1024
#define EXT2_FIRST_BLOCK    1

uint32_t
int_log2(uint32_t i)
{
    uint32_t res = 0;

    while (i) {
        res++;
        i >>= 1;
    }

    return res - 1;
}

extern void lseek64(int, uint64_t, int);

void
disk_read(struct disk *diskp, void *buf, size_t nbyte, uint64_t pos)
{
    lseek64(diskp->fd, pos, SEEK_SET);
    read(diskp->fd, buf, (size_t)nbyte);
}

static void
disk_write(struct disk *diskp, void *buf, size_t nbyte, uint64_t pos)
{
    lseek64(diskp->fd, pos, SEEK_SET);
    write(diskp->fd, buf, (size_t)nbyte);
}

void
read_bg(struct ext2fs *fs, struct disk *diskp, uint64_t index, struct ext2_bg_desc *desc)
{
    memcpy(desc, &fs->group_descriptors[index], sizeof(struct ext2_bg_desc));
}

void
write_bg(struct ext2fs *fs, struct disk *diskp, uint64_t index, struct ext2_bg_desc *desc)
{
    uint64_t bg_addr = fs->bg_start + index*sizeof(struct ext2_bg_desc);

    disk_write(diskp, (char*)desc, sizeof(struct ext2_bg_desc), bg_addr);
}

void
write_inode(struct ext2fs *fs, struct disk *diskp, uint64_t ino, struct ext2_inode *buf)
{
    struct ext2_bg_desc desc;
    uint64_t inode_addr;
    uint64_t bg = INOTOBG(fs->superblock.ipg, ino);
    uint64_t idx = INOIDX(fs->superblock.ipg, ino);
    uint64_t offset = idx*fs->superblock.inode_size;

    read_bg(fs, diskp, bg, &desc);

    inode_addr = BLOCK_ADDR(fs->bsize, desc.i_tables) + offset;

    disk_write(diskp, buf, sizeof(struct ext2_inode), inode_addr);
}

void
block_alloc(struct ext2fs *fs, struct disk *diskp, bool val, uint32_t start, uint32_t amount)
{
    start -= fs->superblock.first_dblock;

    uint64_t bgnum = start / fs->superblock.bpg;
    struct ext2_bg_desc *bg = &fs->group_descriptors[bgnum];

    uint64_t bitmap_addr = BLOCK_ADDR(fs->bsize, bg->b_bitmap);

    disk_read(diskp, (char*)fs->block_cache, fs->bsize, bitmap_addr);

    for (uint32_t i = start; i < start+amount; i++) {
        int index = (i % fs->superblock.bpg) / 8;
        int bit = (i % fs->superblock.bpg) % 8;

        if (val) {
            fs->block_cache[index] |= (1<<bit);
            bg->num_free_blocks--;
            fs->superblock.fbcount--;
        } else {
            fs->block_cache[index] &= ~(1<<bit);
            
        }
    }

    disk_write(diskp, (char*)fs->block_cache, fs->bsize, bitmap_addr);
}

void
inode_alloc(struct ext2fs *fs, struct disk *diskp, bool val, uint32_t start, uint32_t amount)
{
    uint64_t bgnum = INOTOBG(fs->superblock.ipg, start);
    struct ext2_bg_desc *bg = &fs->group_descriptors[bgnum];

    uint64_t bitmap_addr = BLOCK_ADDR(fs->bsize, bg->i_bitmap);

    disk_read(diskp, (char*)fs->block_cache, fs->bsize, bitmap_addr);

    for (uint32_t i = start; i < start+amount; i++) {
        int index = ((i - 1) % fs->superblock.ipg) / 8;
        int bit = ((i - 1) % fs->superblock.ipg) % 8;

        if (val) {
            fs->block_cache[index] |= (1<<bit);
            bg->num_free_inodes--;
            fs->superblock.ficount--;

        } else {
            fs->block_cache[index] &= ~(1<<bit);
        }
    }

    disk_write(diskp, (char*)fs->block_cache, fs->bsize, bitmap_addr);
}

static int
get_random_bytes(void *buf, size_t nbyte)
{
    int fd = open("/dev/random", O_RDONLY);

    if (fd < 0) {
        return -1;
    }

    //read(fd, buf, nbyte);

    close(fd);

    return 0;
}

static void
init_superblock(struct ext2fs *fs, struct disk *diskp)
{
    fs->superblock.magic = 0xef53;
    fs->superblock.inode_size = EXT2_INODE_SIZE;
    fs->superblock.first_dblock = EXT2_FIRST_BLOCK;
    fs->superblock.log_bsize = int_log2(EXT2_BLOCK_SIZE)-10;

    fs->superblock.bcount = diskp->length/EXT2_BLOCK_SIZE;
    fs->superblock.fbcount = fs->superblock.bcount-1;
    fs->superblock.bpg = fs->bsize*8;
    fs->superblock.ipg = fs->superblock.bpg / (fs->bsize / EXT2_INODE_SIZE) / 2;
    fs->superblock.fpg = fs->superblock.bpg;
    fs->superblock.rev = 0;
    fs->superblock.first_ino = 12; 
    fs->superblock.features_incompat = 0x0002;

    if (get_random_bytes(fs->superblock.uuid, 16) != 0) {
        return;
    }

    uint32_t bg_count = (fs->superblock.bcount / fs->superblock.bpg);

    fs->superblock.icount=bg_count*fs->superblock.ipg;
    fs->superblock.ficount = fs->superblock.icount;

    uint64_t bg_start = BLOCK_ADDR(fs->bsize, 1024/fs->bsize+1);

    fs->bg_start = bg_start;

    uint32_t blocks_for_desc = (sizeof(struct ext2_bg_desc)*bg_count+fs->bsize)/fs->bsize;
    uint32_t inode_blocks_per_group = (fs->superblock.ipg * fs->superblock.inode_size + fs->bsize) / fs->bsize;
   
    uint32_t root_dir_table = 0;

    fs->group_descriptors = calloc(sizeof(struct ext2_bg_desc), bg_count);

    printf("block_size=%u block_groups=%u blocks_per_group=%u\n", (uint32_t)fs->bsize, bg_count, fs->superblock.bpg);

    for (uint32_t i = 0; i < bg_count; i++) {
        struct ext2_bg_desc *desc = &fs->group_descriptors[i];

        uint32_t bg_block_start = i * fs->superblock.bpg+1;

        printf(" initializing block group %d\r", i);
        fflush(stdout);

        desc->b_bitmap = bg_block_start + blocks_for_desc;
        desc->i_bitmap = bg_block_start + blocks_for_desc + 1;
        desc->i_tables = bg_block_start + blocks_for_desc + 2;

        desc->num_free_blocks = MIN(fs->superblock.bcount - bg_block_start, fs->superblock.bpg);
        desc->num_free_inodes = MIN(fs->superblock.bcount - bg_block_start, fs->superblock.ipg);
    
        /* first, write all 1s to bitmap. Will pad any excess space */
        memset(fs->block_cache, 0xFF, fs->bsize);

        disk_write(diskp, fs->block_cache, fs->bsize, BLOCK_ADDR(fs->bsize, desc->i_bitmap));
        disk_write(diskp, fs->block_cache, fs->bsize, BLOCK_ADDR(fs->bsize, desc->b_bitmap));

        /* now mark the addressable blocks/indoes as free */
        block_alloc(fs, diskp, false, bg_block_start, desc->num_free_blocks);
        inode_alloc(fs, diskp, false, i*fs->superblock.ipg+1, fs->superblock.ipg);

        block_alloc(fs, diskp, true, bg_block_start, blocks_for_desc+inode_blocks_per_group+1);
    
        if (i == 0) {
            root_dir_table = bg_block_start + blocks_for_desc+inode_blocks_per_group + 1;
            block_alloc(fs, diskp, true, root_dir_table, 1);
        }
    }

    struct ext2_inode root_inode;

    memset(&root_inode, 0, sizeof(root_inode));

    root_inode.mode = 0040755;
    root_inode.atime = time(NULL);
    root_inode.ctime = time(NULL);
    root_inode.mtime = time(NULL);
    root_inode.size = fs->bsize;
    root_inode.isize = 128;
    root_inode.nlink = 2;
    root_inode.nblock = 2;
    root_inode.blocks[0] = root_dir_table;

    write_inode(fs, diskp, 2, &root_inode);

    inode_alloc(fs, diskp, true, 1, 10);
    memset(fs->block_cache, 0, fs->bsize);

	struct ext2_dirent *self_dir = (struct ext2_dirent*)&fs->block_cache[0];
	self_dir->type = 2;
	self_dir->inode = 2;
	self_dir->name_len = 1;
	self_dir->size = 12;
	self_dir->name[0] = '.';

	struct ext2_dirent *parent_dir = (struct ext2_dirent*)&fs->block_cache[12];
	parent_dir->type = 2;
	parent_dir->inode = 2;
	parent_dir->name_len =  2;
	parent_dir->size = fs->bsize - 12;
	parent_dir->name[0] = '.';
	parent_dir->name[1] = '.';

    fs->group_descriptors[0].num_dirs = 1;

    disk_write(diskp, fs->block_cache, fs->bsize, BLOCK_ADDR(fs->bsize, root_dir_table));

    puts("");
    puts("finalizing filesystem");

    disk_write(diskp, fs->group_descriptors, sizeof(struct ext2_bg_desc)*bg_count, bg_start);
    disk_write(diskp, &fs->superblock, sizeof(fs->superblock), 1024);
}

static void
ext2_format(struct disk *diskp)
{
    struct ext2fs fs;
    memset(&fs, 0, sizeof(fs));
    fs.bsize = 1024;
    fs.block_cache = calloc(fs.bsize, 1);
    init_superblock(&fs, diskp);
}

static int
get_length(int fd, uint64_t *lengthp)
{
    struct stat sb;

    if (fstat(fd, &sb) != 0) {
        return -1;
    }


    mode_t fmt = sb.st_mode & S_IFMT;

    if ((fmt & (S_IFCHR | S_IFBLK))) {
        /* elysium ioctl BLKGETSIZE */
        return ioctl(fd, 0x0300, lengthp);
    }


    *lengthp = sb.st_size;

    return 0;
}

int
main(int argc, const char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: newfs <device>\n");
        return -1;
    }
    
    const char *dev_name = argv[1];

    int fd = open(dev_name, O_RDWR);

    if (fd == -1) {
        perror("newfs");
        return -1;
    }

    uint64_t length;

    if (get_length(fd, &length) != 0) {
        perror("newfs");
        return -1;
    }

    struct disk disk;
    disk.fd = fd;
    disk.length = length;
    ext2_format(&disk);

    return 0;
}

