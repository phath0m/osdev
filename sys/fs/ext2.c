/*
 * ext2.c - shitty 2nd Extended File System Implementation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <ds/dict.h>
#include <ds/list.h>
#include <sys/cdev.h>
#include <sys/errno.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/string.h>
#include <sys/types.h>
#include <sys/vnode.h>
// delete me
#include <sys/systm.h>


#define EXT2_WORD_ALIGN(n) (((n) + 4) & 0xFFFFFFFC)

#define MIN(X, Y) ((X < Y) ? X : Y)

#define BLOCK_ADDR(bsize, bnum) ((uint64_t)(bsize)*(uint64_t)(bnum))
#define BLOCK_NUM(bsize, offset) ((offset)/(bsize))

/* inode number to block group */
#define INOTOBG(igp, i) (((i) - 1) / (igp))

/* extract inode index within inode table*/
#define INOIDX(igp, i) (((i) - 1) % (igp))

static int ext2_lookup(struct vnode *parent, struct vnode **result, const char *name);
static int ext2_mount(struct vnode *parent, struct cdev *dev, struct vnode **root);
static int ext2_chmod(struct vnode *vn, mode_t mode);
static int ext2_chown(struct vnode *vn, uid_t owner, gid_t group);
static int ext2_creat(struct vnode *parent, struct vnode **child, const char *name, mode_t mode);
static int ext2_read(struct vnode *vn, void *buf, size_t nbyte, uint64_t pos);
static int ext2_readdirent(struct vnode *vn, struct dirent *dirent, uint64_t entry);
static int ext2_rmdir(struct vnode *parent, const char *dirname); 
static int ext2_mkdir(struct vnode *parent, const char *name, mode_t mode);
static int ext2_mknod(struct vnode *parent, const char *name, mode_t mode, dev_t dev);
static int ext2_seek(struct vnode *vn, off_t *pos, off_t off, int whence);
static int ext2_stat(struct vnode *vne, struct stat *stat);
static int ext2_truncate(struct vnode *vn, off_t length);
static int ext2_unlink(struct vnode *parent, const char *dirname);
static int ext2_utime(struct vnode *vn, struct timeval tv[2]);
static int ext2_write(struct vnode *vn, const void *buf, size_t nbyte, uint64_t pos);

struct vops ext2_file_ops = {
    .chmod      = ext2_chmod,
    .chown      = ext2_chown,
    .creat      = ext2_creat,
    .lookup     = ext2_lookup,
    .read       = ext2_read,
    .readdirent = ext2_readdirent,
    .rmdir      = ext2_rmdir,
    .mkdir      = ext2_mkdir,
    .mknod      = ext2_mknod,
    .seek       = ext2_seek,
    .stat       = ext2_stat,
    .truncate   = ext2_truncate,
    .unlink     = ext2_unlink,
    .utimes     = ext2_utime,
    .write      = ext2_write
};

struct fs_ops ext2_ops = {
    .mount = ext2_mount,
};

struct ext2_superblock {
    uint32_t    icount;         /* inode count */
    uint32_t    bcount;         /* block count */
    uint32_t    rbcount;        /* reserved block count */
    uint32_t    fbcount;        /* free block count*/
    uint32_t    ficount;        /* free inode count */
    uint32_t    first_dblock;   /* first data block */
    uint32_t    log_bsize;        /* block size */
    uint32_t    log_fsize;      /* fragment size */
    uint32_t    bpg;            /* blocks per group */
    uint32_t    fpg;            /* fragements per group */
    uint32_t    ipg;            /* inodes per group */
    uint32_t    mtime;          /* mount mtime */
    uint32_t    wtime;          /* write time */
    uint16_t    mnt_count;      /* mount count */
    uint16_t    max_mnt_count;  /* max mount count */
    uint16_t    magic;          /* magic number */
    uint16_t    state;          /* filesystem state */
    uint16_t    beh;            /* error behavior */
    uint16_t    minrev;         /* minor revision level */
    uint32_t    lastfsck;       /* last fsck */
    uint32_t    fsckintv;       /* fsck interval */
    uint32_t    creator;        /* creator os*/
    uint32_t    rev;            /* revision */
    uint16_t    ruid;           /* default UID for reserved blocks */
    uint16_t    rgid;           /* default GID for reserved blocks */
    uint32_t    first_ino;      /* first non-reserved inode */
    uint16_t    inode_size;     /* size of inode structure */
    uint16_t    block_group_nr; /* block grp number of super block */
    uint32_t    features_compat; /* compatible feature set */
    uint32_t    features_incompat;
    uint32_t    features_rocompat;
    uint8_t     uuid[16];
    char        vname[16];
    char        fsmnt[64];
    uint32_t    algo;       /* compression algorithm */
    uint8_t     prealloc;
    uint8_t     dir_prealloc;
    uint16_t    reserved_ngbd;
    
} __attribute__((packed));

struct ext2_bg_desc {
    uint32_t    b_bitmap; /* block bitmap */
    uint32_t    i_bitmap; /* inode bitmap */
    uint32_t    i_tables; /* inode table block*/
    uint16_t    num_free_blocks;
    uint16_t    num_free_inodes;
    uint16_t    num_dirs;
    uint16_t    reserved;
    uint32_t    reserved2[3];
} __attribute__((packed));

struct ext2_inode {
    uint16_t    mode;
    uint16_t    uid;
    uint32_t    size;
    uint32_t    atime;
    uint32_t    ctime;
    uint32_t    mtime;
    uint32_t    dtime;
    uint16_t    gid;
    uint16_t    nlink;
    uint32_t    nblock;
    uint32_t    flags;
    uint32_t    version;
    uint32_t    blocks[12+3];
    uint32_t    gen;
    uint32_t    facl;
    uint32_t    size_hi;
    uint32_t    faddr;
    uint16_t    nblock_hi;
    uint16_t    facl_hi;
    uint16_t    uid_high;
    uint16_t    gid_high;
    uint16_t    chksum_lo;
    uint16_t    reserved;
    uint16_t    isize;
    uint16_t    chksum_hi;
    uint32_t    x_ctime;
    uint32_t    x_mtime;
    uint32_t    x_atime;
    uint32_t    crtime;
    uint32_t    x_crtime;
    uint32_t    version_hi;
} __attribute__((packed));

struct ext2_dirent {
    uint32_t    inode;
    uint16_t    size;
    uint8_t     name_len;
    uint8_t     type;
    char        name[];
} __attribute__((packed));

/* block sized buffer to avoid repeative reads from the underlying device */
struct ext2_block_buf {
    uint64_t    block_addr;
    uint8_t *   buf;
};

struct ext2fs {
    struct ext2_superblock  superblock;
    uint32_t                bsize;
    uint32_t                igp;
    uint32_t                bpg;
    uint32_t                inode_size;
    uint64_t                bg_start;
    uint64_t                bg_count;
    uint8_t *               block_cache;
    struct ext2_block_buf   superblock_cache;
    struct ext2_block_buf   block_ptr_buf;
    struct ext2_block_buf   single_indirect_buf;
    struct ext2_block_buf   double_indirect_buf;
    struct cdev *           cdev;
    spinlock_t              lock;
};

__attribute__((always_inline))
static inline void
ext2fs_lock(struct ext2fs *fs)
{
    spinlock_lock(&fs->lock);
}

__attribute__((always_inline))
static inline void
ext2fs_unlock(struct ext2fs *fs)
{
    spinlock_unlock(&fs->lock);
}

static void
ext2fs_init_block_buf(struct ext2fs *fs, struct ext2_block_buf *buf)
{
    buf->buf = calloc(1, fs->bsize);
    buf->block_addr = (uint32_t)-1;
}

static void
ext2fs_rewrite_superblock(struct ext2fs *fs)
{
    cdev_write(fs->cdev, (char*)&fs->superblock, sizeof(fs->superblock), 1024);
}

static int
ext2fs_read_block(struct ext2fs *fs, uint64_t block_addr, struct ext2_block_buf *buf)
{
    if (buf->block_addr == block_addr) {
        return 0;
    }

    if (cdev_read(fs->cdev, (char*)buf->buf, fs->bsize, BLOCK_ADDR(fs->bsize, block_addr)) != fs->bsize) {
        return -1;
    }

    buf->block_addr = block_addr;

    return 0;
}

static int
ext2fs_read_bg(struct ext2fs *fs, uint64_t index, struct ext2_bg_desc *desc)
{
    uint64_t bg_addr = fs->bg_start + index*sizeof(struct ext2_bg_desc);

    if (cdev_read(fs->cdev, (char*)desc, sizeof(struct ext2_bg_desc), bg_addr) != sizeof(struct ext2_bg_desc)) {
        return -1;
    }

    return 0;
}

static int
ext2fs_write_bg(struct ext2fs *fs, uint64_t index, struct ext2_bg_desc *desc)
{
    uint64_t bg_addr = fs->bg_start + index*sizeof(struct ext2_bg_desc);

    if (cdev_write(fs->cdev, (char*)desc, sizeof(struct ext2_bg_desc), bg_addr) != sizeof(struct ext2_bg_desc)) {
        return -1;
    }

    return 0;
}

static int
ext2fs_read_inode(struct ext2fs *fs, uint64_t ino, struct ext2_inode *buf)
{
    uint64_t bg = INOTOBG(fs->igp, ino);
    uint64_t idx = INOIDX(fs->igp, ino);
    uint64_t offset = idx*fs->inode_size;
    struct ext2_bg_desc desc;

    if (ext2fs_read_bg(fs, bg, &desc) != 0) {
        return -1;
    }

    uint64_t inode_addr = BLOCK_ADDR(fs->bsize, desc.i_tables) + offset;
    
    if (cdev_read(fs->cdev, (char*)buf, sizeof(struct ext2_inode), inode_addr) != sizeof(struct ext2_inode)) {
        return -1;
    }
    return 0;
}

static int
ext2fs_write_inode(struct ext2fs *fs, uint64_t ino, struct ext2_inode *buf)
{
    uint64_t bg = INOTOBG(fs->igp, ino);
    uint64_t idx = INOIDX(fs->igp, ino);
    uint64_t offset = idx*fs->inode_size;
    struct ext2_bg_desc desc;

    if (ext2fs_read_bg(fs, bg, &desc) != 0) {
        return -1;
    }

    uint64_t inode_addr = BLOCK_ADDR(fs->bsize, desc.i_tables) + offset;

    if (cdev_write(fs->cdev, (char*)buf, sizeof(struct ext2_inode), inode_addr) != sizeof(struct ext2_inode)) {
        return -1;
    }

    return 0;
}

static int
ext2fs_read_dblock(struct ext2fs *fs, struct ext2_inode *inode, uint64_t block_addr, void *buf)
{
    if (block_addr < 12) {
        return cdev_read(fs->cdev, (char*)buf, fs->bsize, BLOCK_ADDR(fs->bsize, inode->blocks[block_addr]));
    }
    
    uint64_t ptrs_per_block = (fs->bsize / 4);

    if (block_addr < ((ptrs_per_block + 12)) && inode->blocks[12]) {
        if (ext2fs_read_block(fs, inode->blocks[12], &fs->single_indirect_buf) != 0) {
            return -1;
        }
        uint32_t *indirect_blocks = (uint32_t*)fs->single_indirect_buf.buf;
        uint32_t ptr_idx = block_addr - 12;
        uint64_t block_num = BLOCK_ADDR(fs->bsize, indirect_blocks[ptr_idx]);
        return cdev_read(fs->cdev, (char*)buf, fs->bsize, block_num);
    } else {
        uint32_t table_idx = (block_addr - (ptrs_per_block + 12)) / ptrs_per_block;
        uint32_t ptr_idx = (block_addr - (ptrs_per_block + 12)) % ptrs_per_block;
        if (ext2fs_read_block(fs, inode->blocks[13], &fs->single_indirect_buf) != 0) {
            return -1;
        }
        
        uint32_t *indirect_table = (uint32_t*)fs->single_indirect_buf.buf;
        if (ext2fs_read_block(fs, indirect_table[table_idx], &fs->block_ptr_buf) != 0) {
            return -1;
        }

        uint32_t *block_ptrs = (uint32_t*)fs->block_ptr_buf.buf;
        return cdev_read(fs->cdev, (char*)buf, fs->bsize, BLOCK_ADDR(fs->bsize, block_ptrs[ptr_idx]));
    }

    return -1;
}

static int
ext2fs_write_dblock(struct ext2fs *fs, struct ext2_inode *inode, uint64_t block_addr, void *buf)
{
    if (block_addr < 12) {
        return cdev_write(fs->cdev, (char*)buf, fs->bsize, BLOCK_ADDR(fs->bsize, inode->blocks[block_addr]));
    }

    uint64_t ptrs_per_block = (fs->bsize / 4);

    if (block_addr < ((ptrs_per_block + 12)) && inode->blocks[12]) {
        if (ext2fs_read_block(fs, inode->blocks[12], &fs->single_indirect_buf) != 0) {
            return -1;
        }
        uint32_t *indirect_blocks = (uint32_t*)fs->single_indirect_buf.buf;
        uint32_t ptr_idx = block_addr - 12;
        uint64_t block_num = BLOCK_ADDR(fs->bsize, indirect_blocks[ptr_idx]);
        return cdev_write(fs->cdev, (char*)buf, fs->bsize, block_num);
    } else {
        uint32_t table_idx = (block_addr - (ptrs_per_block + 12)) / ptrs_per_block;
        uint32_t ptr_idx = (block_addr - (ptrs_per_block + 12)) % ptrs_per_block;
        if (ext2fs_read_block(fs, inode->blocks[13], &fs->single_indirect_buf) != 0) {
            return -1;
        }

        uint32_t *indirect_table = (uint32_t*)fs->single_indirect_buf.buf;
        if (ext2fs_read_block(fs, indirect_table[table_idx], &fs->block_ptr_buf) != 0) {
            return -1;
        }

        uint32_t *block_ptrs = (uint32_t*)fs->block_ptr_buf.buf;
        return cdev_write(fs->cdev, (char*)buf, fs->bsize, BLOCK_ADDR(fs->bsize, block_ptrs[ptr_idx]));
    }

    return -1;
}

static int
ext2fs_fill_dirent(struct ext2fs *fs, int ino, int dir, struct dirent *buf)
{
    uint8_t *block = fs->block_cache;
    struct ext2_inode inode;

    if (ext2fs_read_inode(fs, ino, &inode) != 0) {
        return -1;
    }

    int i = 0;
    int blocks_required = inode.size / fs->bsize;

    for (int b = 0; b < blocks_required; b++) {
        uint64_t offset = 0;
        ext2fs_read_dblock(fs, &inode, b, block);

        while (offset + b*fs->bsize < fs->bsize) {
            struct ext2_dirent *dirent = (struct ext2_dirent*)&block[offset];
            
            if (dirent->inode) {
                if (i == dir) {
                    memcpy(buf->name, dirent->name, dirent->name_len);
                    buf->name[dirent->name_len] = 0;
                    buf->inode = dirent->inode;
                    switch (dirent->type) {
                        case 1:
                            buf->type = DT_REG;
                            break;
                        case 2:
                            buf->type = DT_DIR;
                            break;
                        case 3:
                            buf->type = DT_CHR;
                            break;
                        case 4:
                            buf->type = DT_BLK;
                            break;
                    }
                    return 0;
                }
                offset += dirent->size;
                i++;
            } else {
                return -(ENOENT);
            }
        }
    }

    return -(ENOENT);
}

static int
ext2fs_fill_vnode(struct ext2fs *fs, int ino, struct vnode *vn)
{
    struct ext2_inode inode_buf;
    ext2fs_read_inode(fs, ino, &inode_buf);

    vn->uid = inode_buf.uid;
    vn->gid = inode_buf.gid;
    vn->mode = inode_buf.mode;
    vn->inode = ino;
    vn->state = fs;

    return 0;
}

static uint32_t
ext2fs_block_alloc(struct ext2fs *fs, int preferred_bg)
{
    struct ext2_bg_desc bg;
    int64_t bgnum = -1;

    for (uint64_t i = 0; i < fs->bg_count; i++) {
        int64_t bg_low = preferred_bg - i;
        int64_t bg_high = preferred_bg + i;

        if (bg_low >= 0) {
            ext2fs_read_bg(fs, bg_low, &bg);
            
            if (bg.num_free_blocks !=  0) {
                bgnum = bg_low;
                break;
            }
        }

        if (bg_low != bg_high && bg_high < fs->bg_count) {
            ext2fs_read_bg(fs, bg_high, &bg);

            if (bg.num_free_blocks != 0) {
                bgnum = bg_high;
                break;
            }
        }

    }

    if (bgnum == -1) {
        return (uint32_t)-1;
    }

    if (cdev_read(fs->cdev, (char*)fs->block_cache, fs->bsize, BLOCK_ADDR(fs->bsize, bg.b_bitmap)) != fs->bsize) {
        return (uint32_t)-1;
    }

    int32_t ret = -1;

    for (int i = 0; i < fs->bsize && ret == -1; i++) {
        uint8_t byte = fs->block_cache[i];

        for (int b = 0; b < 8; b++) {
            if (((1<<b) & byte) == 0) {
                fs->block_cache[i] = (byte | (1<<b));
                ret = 8 * i + b;
                break;
            }

        }
    }

    if (ret != -1) {
        cdev_write(fs->cdev, (char*)fs->block_cache, fs->bsize, BLOCK_ADDR(fs->bsize, bg.b_bitmap));

        bg.num_free_blocks++;
        ext2fs_write_bg(fs, bgnum, &bg);

        fs->superblock.fbcount--;
        ext2fs_rewrite_superblock(fs);

        uint32_t block_addr = fs->superblock.first_dblock + bgnum*fs->bpg + ret;

        memset(fs->block_cache, 0, fs->bsize);
        cdev_write(fs->cdev, (char*)fs->block_cache, fs->bsize, BLOCK_ADDR(fs->bsize, block_addr));

        return block_addr;
    }

    return 0;
}

static uint32_t
ext2fs_block_free(struct ext2fs *fs, int blockno)
{
    blockno -= fs->superblock.first_dblock;

    struct ext2_bg_desc bg;
    int64_t bgnum = blockno / fs->bpg;
    
    ext2fs_read_bg(fs, bgnum, &bg);

    int64_t bitmap_addr = BLOCK_ADDR(fs->bsize, bg.b_bitmap);

    if (cdev_read(fs->cdev, (char*)fs->block_cache, fs->bsize, bitmap_addr) != fs->bsize) {
        return (uint32_t)-1;
    }

    int index = (blockno % fs->bpg) / 8;
    int bit = (blockno % fs->bpg) % 8;

    KASSERT((fs->block_cache[index] & (1<<bit)) != 0, "block being freed should be allocated");

    fs->block_cache[index] &= ~(1<<bit);
    cdev_write(fs->cdev, (char*)fs->block_cache, fs->bsize, bitmap_addr);

    fs->superblock.fbcount++;
    ext2fs_rewrite_superblock(fs);

    return 0;
}

static uint32_t
ext2fs_inode_alloc(struct ext2fs *fs, int64_t preferred_bg)
{
    struct ext2_bg_desc bg;
    int64_t bgnum = -1;

    for (int64_t i = 0; i < (int64_t)fs->bg_count; i++) {
        int64_t bg_low = preferred_bg - i;
        int64_t bg_high = preferred_bg + i;

        if (bg_low >= 0) {
            ext2fs_read_bg(fs, bg_low, &bg);

            if (bg.num_free_inodes != 0) {
                bgnum = bg_low;
                break;
            }
        }

        if (bg_low != bg_high && bg_high < fs->bg_count) {
            ext2fs_read_bg(fs, bg_high, &bg);

            if (bg.num_free_inodes != 0) {
                bgnum = bg_high;
                break;
            }
        }
        
    }

    if (bgnum == -1) {
        return (uint32_t)-1;
    }

    if (cdev_read(fs->cdev, (char*)fs->block_cache, fs->bsize, BLOCK_ADDR(fs->bsize, bg.i_bitmap)) != fs->bsize) {
        return (uint32_t)-1;
    }

    int32_t ret = -1;

    for (int i = 0; i < fs->bsize && ret == -1; i++) {
        uint8_t byte = fs->block_cache[i];

        for (int b = 0; b < 8; b++) {
            if (((1<<b) & byte) == 0) {
                fs->block_cache[i] |= 1<<b;
                ret = 8 * i + b;
                break;
            }

        }
    }

    if (ret != -1) {
        cdev_write(fs->cdev, (char*)fs->block_cache, fs->bsize, BLOCK_ADDR(fs->bsize, bg.i_bitmap));
        bg.num_free_inodes++;
        ext2fs_write_bg(fs, bgnum, &bg);
        
        fs->superblock.ficount--;
        ext2fs_rewrite_superblock(fs);

        return fs->superblock.first_dblock + fs->igp*bgnum + ret;
    }

    return -1;
}


uint32_t
ext2fs_inode_free(struct ext2fs *fs, int inum)
{
    struct ext2_bg_desc bg;

    int64_t bgnum = INOTOBG(fs->igp, inum);

    ext2fs_read_bg(fs, bgnum, &bg);

    int64_t bitmap_addr = BLOCK_ADDR(fs->bsize, bg.i_bitmap);

    if (cdev_read(fs->cdev, (char*)fs->block_cache, fs->bsize, bitmap_addr) != fs->bsize) {
        return (uint32_t)-1;
    }

    int index = ((inum-1) % fs->igp) / 8;
    int bit = ((inum-1) % fs->igp) % 8;

    KASSERT((fs->block_cache[index] & (1<<bit)) != 0, "inode being freed should be allocated");

    fs->block_cache[index] &= ~(1<<bit);
    cdev_write(fs->cdev, (char*)fs->block_cache, fs->bsize, bitmap_addr);

    fs->superblock.ficount++;
    ext2fs_rewrite_superblock(fs);

    return 0;
}

static int
ext2fs_grow_inode(struct ext2fs *fs, int ino, struct ext2_inode *inode, size_t newsize)
{
    int current_blocks = (inode->size + fs->bsize - 1) / fs->bsize;
    int blocks_needed = (inode->size + newsize + fs->bsize - 1) / fs->bsize;

    for (int block = current_blocks; block < blocks_needed; block++) {
        uint32_t new_block = ext2fs_block_alloc(fs, 0);

        if (block < 12) {
            inode->blocks[block] = new_block;/* alloc */
            continue;
        }

        uint64_t ptrs_per_block = (fs->bsize / 4);

        if (block < ((ptrs_per_block + 12))) {

            if (!inode->blocks[12]) {
                inode->blocks[12] = ext2fs_block_alloc(fs, 0);
            }

            if (ext2fs_read_block(fs, inode->blocks[12], &fs->single_indirect_buf) != 0) {
                /* FAIL!*/
                return -1;
            }
            uint32_t *indirect_blocks = (uint32_t*)fs->single_indirect_buf.buf;

            indirect_blocks[block - 12] = new_block;
        } else {
            uint32_t table_idx = (block - (ptrs_per_block + 12)) / ptrs_per_block;
            uint32_t ptr_idx = (block - (ptrs_per_block + 12)) % ptrs_per_block;

            if (!inode->blocks[13]) {
                inode->blocks[13] = ext2fs_block_alloc(fs, 0);
            }

            if (ext2fs_read_block(fs, inode->blocks[13], &fs->single_indirect_buf) != 0) {
                /* fail */
                return -1;
            }

            uint32_t *indirect_table = (uint32_t*)fs->single_indirect_buf.buf;

            if (ext2fs_read_block(fs, indirect_table[table_idx], &fs->block_ptr_buf) != 0) {
                return -1;
            }

            uint32_t *block_ptrs = (uint32_t*)fs->block_ptr_buf.buf;
            
            block_ptrs[ptr_idx] = new_block;
        }
    }

    inode->size = newsize;
    ext2fs_write_inode(fs, ino, inode);
    return 0;
}

static int
ext2fs_shrink_inode(struct ext2fs *fs, int ino, struct ext2_inode *inode, size_t newsize)
{
    int current_blocks = (inode->size + fs->bsize - 1) / fs->bsize;
    int blocks_needed = (inode->size + newsize + fs->bsize - 1) / fs->bsize;

    for (int block = current_blocks; blocks_needed < block; block--) {
        if (block < 12) {
            ext2fs_block_free(fs, inode->blocks[block]);
            continue;
        }

        uint64_t ptrs_per_block = (fs->bsize / 4);

        if (block < ((ptrs_per_block + 12))) {
            if (ext2fs_read_block(fs, inode->blocks[12], &fs->single_indirect_buf) != 0) {
                /* FAIL!*/
                return -1;
            }

            uint32_t *indirect_blocks = (uint32_t*)fs->single_indirect_buf.buf;

            ext2fs_block_free(fs, indirect_blocks[block - 12]);

            if (block == 12) {
                ext2fs_block_free(fs, inode->blocks[12]);
            }

        } else {
            uint32_t table_idx = (block - (ptrs_per_block + 12)) / ptrs_per_block;
            uint32_t ptr_idx = (block - (ptrs_per_block + 12)) % ptrs_per_block;

            if (ext2fs_read_block(fs, inode->blocks[13], &fs->single_indirect_buf) != 0) {
                /* fail */
                return -1;
            }

            uint32_t *indirect_table = (uint32_t*)fs->single_indirect_buf.buf;

            if (ext2fs_read_block(fs, indirect_table[table_idx], &fs->block_ptr_buf) != 0) {
                return -1;
            }

            uint32_t *block_ptrs = (uint32_t*)fs->block_ptr_buf.buf;

            ext2fs_block_free(fs, block_ptrs[ptr_idx]);

            if (ptr_idx == 0) {
                ext2fs_block_free(fs, indirect_table[table_idx]);
            }

            if (table_idx == 0 && ptr_idx == 0) {
                ext2fs_block_free(fs, inode->blocks[13]);
            }
        }
    }

    inode->size = newsize;
    ext2fs_write_inode(fs, ino, inode);

    return 0;
}

static int
ext2fs_mknod(struct vnode *parent, const char *name, mode_t mode, uint32_t *inump, struct ext2_inode *child_inode)
{
    struct ext2fs *fs = parent->state;
    struct ext2_inode parent_inode;

    if (ext2fs_read_inode(fs, parent->inode, &parent_inode) != 0) {
        return -(EIO);
    }

    ssize_t name_len = strlen(name);
    ssize_t dirent_size = EXT2_WORD_ALIGN(sizeof(struct ext2_dirent) + name_len + 1);

    uint32_t inum = ext2fs_inode_alloc(fs,  INOTOBG(fs->igp, parent->inode));
    uint8_t *block_buf = fs->block_cache;

    int block_num;
    int blocks_required = parent_inode.size / fs->bsize;

    uint64_t offset = 0;
    bool space_found = false;

    for (block_num = 0; block_num < blocks_required; block_num++) {
        ext2fs_read_dblock(fs, &parent_inode, block_num, block_buf);

        offset = 0;

        while (offset < fs->bsize) {
            struct ext2_dirent *dirent = (struct ext2_dirent*)&block_buf[offset];
            ssize_t min_size = EXT2_WORD_ALIGN(dirent->name_len + sizeof(struct ext2_dirent));
            ssize_t slackspace = dirent->size - min_size;

            if (strncmp(dirent->name, name, dirent->name_len) == 0) {
                return -(EEXIST);
            }

            if (slackspace > dirent_size) {
                dirent->size = min_size;
                offset += dirent->size;
                space_found = true;
                break;
            }

            offset += dirent->size;
        }

        if (space_found) {
            break;
        }
    }

    if (!space_found) {
        offset = 0;
        ext2fs_grow_inode(fs, parent->inode, &parent_inode, parent_inode.size + fs->bsize);
    }

    struct ext2_dirent *dirent = (struct ext2_dirent*)&block_buf[offset];
    dirent->name_len = name_len;
    dirent->size = fs->bsize - offset;
    dirent->inode = inum;
    
    switch (mode & S_IFMT) {
        case S_IFREG:
            dirent->type = 1;
            break;
        case S_IFDIR:
            dirent->type = 2;
            break;
        case S_IFCHR:
            dirent->type = 3;
            break;
        case S_IFBLK:
            dirent->type = 4;
            break;
        default:
            break;
    }

    memcpy(dirent->name, name, name_len);

    ext2fs_write_dblock(fs, &parent_inode, block_num, block_buf);

    memset(child_inode, 0, sizeof(struct ext2_inode));

    child_inode->gid = 0;
    child_inode->mode = mode;
    child_inode->uid = 0;
    child_inode->mtime = time(NULL);
    child_inode->atime = time(NULL);
    child_inode->ctime = time(NULL);

    ext2fs_write_inode(fs, inum, child_inode);

    *inump = inum;

    return 0;
}

static int
ext2fs_remove_dirent(struct vnode *parent, const char *name)
{
    struct ext2fs *fs = parent->state;
    struct ext2_inode parent_inode;

    if (ext2fs_read_inode(fs, parent->inode, &parent_inode) != 0) {
        return -(EIO);
    }

    size_t name_len = strlen(name);
    uint8_t *block_buf = fs->block_cache;

    int block_num;
    int blocks_required = parent_inode.size / fs->bsize;

    for (block_num = 0; block_num < blocks_required; block_num++) {
        ext2fs_read_dblock(fs, &parent_inode, block_num, block_buf);

        struct ext2_dirent *last_dirent = NULL;
        size_t shift_amount = 0;
        uint32_t shift_offset = 0;
        uint32_t offset = 0;

        while (offset < fs->bsize) {
            struct ext2_dirent *dirent = (struct ext2_dirent*)&block_buf[offset];

            if (dirent->name_len == name_len && memcmp(dirent->name, name, name_len) == 0) {
                shift_amount = dirent->size;
                shift_offset = offset;
            } else {
                last_dirent = dirent;
            }

            offset += dirent->size;
        }

        if (shift_amount) {
            last_dirent->size += shift_amount;
            memcpy(&block_buf[shift_offset], &block_buf[shift_offset + shift_amount], fs->bsize - (shift_offset + shift_amount));
            ext2fs_write_dblock(fs, &parent_inode, block_num, block_buf);
            break;
        }
    }

    return 0;
}

static int
ext2_chmod(struct vnode *vn, mode_t mode)
{
    struct ext2_inode inode;

    if (ext2fs_read_inode((struct ext2fs*)vn->state, vn->inode, &inode) != 0) {
        return -(EIO);
    }

    mode_t fmt = mode & S_IFMT;
    mode &= ~S_IFMT;

    inode.mode = mode | fmt;

    if (ext2fs_write_inode(vn->state, vn->inode, &inode) != 0) {
        return -(EIO);
    }

    return 0;
}

static int
ext2_chown(struct vnode *vn, uid_t owner, gid_t group)
{
    struct ext2_inode inode;

    if (ext2fs_read_inode((struct ext2fs*)vn->state, vn->inode, &inode) != 0) {
        return -(EIO);
    }

    inode.uid = owner;
    inode.gid = group;

    if (ext2fs_write_inode(vn->state, vn->inode, &inode) != 0) {
        return -(EIO);
    }

    return 0;
}

static int
ext2_creat(struct vnode *parent, struct vnode **child, const char *name, mode_t mode)
{
    uint32_t inum;
    struct ext2_inode child_inode;
   
    ext2fs_lock(parent->state);

    int res = ext2fs_mknod(parent, name, mode | S_IFREG, &inum, &child_inode);
    
    if (res == 0) {
        res = ext2_lookup(parent, child, name);
    }

    ext2fs_unlock(parent->state);

    return res;
}

static int
ext2_lookup(struct vnode *parent, struct vnode **result, const char *name)
{
    struct ext2fs *fs = parent->state;
    struct dirent dirent;

    for (int i = 0; ext2fs_fill_dirent(fs, parent->inode, i, &dirent) == 0; i++) {
        if (strcmp(name, dirent.name) == 0) {
            struct vnode *vn = vn_new(parent, fs->cdev, &ext2_file_ops);
            ext2fs_fill_vnode(fs, dirent.inode, vn);
            *result = vn;
            return 0;
        }
    }

    return -(ENOENT);
}

static int
ext2_mount(struct vnode *parent, struct cdev *cdev, struct vnode **root)
{
    struct ext2_superblock superblock;
    cdev_read(cdev, (char*)&superblock, sizeof(superblock), 1024);

    if (superblock.magic != 0xef53) {
        return -(EINVAL);
    }

    struct ext2fs *fs = calloc(1, sizeof(struct ext2fs));
    
    memcpy(&fs->superblock, &superblock, sizeof(superblock));

    fs->bsize = 1024 << superblock.log_bsize;
    fs->cdev = cdev;
    fs->inode_size = superblock.inode_size;
    fs->igp = superblock.ipg;
    fs->bpg = superblock.bpg;
    fs->block_cache = calloc(1, fs->bsize);
    fs->bg_start = BLOCK_ADDR(fs->bsize, 1024/fs->bsize+1);
    fs->bg_count = (superblock.bcount / superblock.bpg) - superblock.first_dblock;

    ext2fs_init_block_buf(fs, &fs->block_ptr_buf);
    ext2fs_init_block_buf(fs, &fs->single_indirect_buf);
    ext2fs_init_block_buf(fs, &fs->double_indirect_buf);

    struct vnode *vn = vn_new(parent, cdev, &ext2_file_ops);
    ext2fs_fill_vnode(fs, 2, vn);
    *root = vn;

    return 0;
}

static int
ext2_read(struct vnode *vn, void *buf, size_t nbyte, uint64_t pos)
{
    struct ext2fs *fs = vn->state;
    uint8_t *block = fs->block_cache;
    struct ext2_inode inode;
    uint64_t start = pos;
    uint64_t end = start + nbyte;

    if (ext2fs_read_inode(fs, vn->inode, &inode) != 0) {
        return -(EIO);
    }

    if (!S_ISREG(inode.mode)) {
        return -(EISDIR);
    }

    if (start >= inode.size) {
        return 0;
    }

    if (end > inode.size) {
        nbyte = inode.size - start;
    }

    int bytes_read = 0;
    uint64_t start_block = pos / fs->bsize;
    uint64_t start_offset = pos % fs->bsize;
    char *im_not_crazy = (char*)buf;
    for (uint64_t i = 0; bytes_read < nbyte; i++) {
        uint32_t bytes_this_block = MIN(fs->bsize-start_offset, nbyte - bytes_read);
        ext2fs_read_dblock(fs, &inode, start_block + i, block);
        memcpy(&im_not_crazy[bytes_read], &block[start_offset], bytes_this_block);
        start_offset = 0;
        bytes_read += bytes_this_block;
    }
    return bytes_read;
}

static int
ext2_readdirent(struct vnode *vn, struct dirent *dirent, uint64_t entry)
{
    if (ext2fs_fill_dirent((struct ext2fs*)vn->state, vn->inode, entry, dirent) == 0) {
        return 0;
    }

    return -(ENOENT);
}

static int
ext2_rmdir(struct vnode *parent, const char *dirname)
{
    return 0;
}

static int
ext2_mkdir(struct vnode *parent, const char *name, mode_t mode)
{
    struct ext2fs *fs = parent->state;
    uint8_t *block_buf = fs->block_cache; 
    uint32_t inum;
    struct ext2_inode child_inode;
    int res = ext2fs_mknod(parent, name, mode | S_IFDIR, &inum, &child_inode);

    if (res == 0) {
        ext2fs_grow_inode(fs, inum, &child_inode, fs->bsize);

        memset(block_buf, 0, fs->bsize);

        struct ext2_dirent *self_dir = (struct ext2_dirent*)&block_buf[0];
        self_dir->type = 2;
        self_dir->inode = inum;
        self_dir->name_len = 1;
        self_dir->size = 12;
        self_dir->name[0] = '.'; 

        struct ext2_dirent *parent_dir = (struct ext2_dirent*)&block_buf[12];
        parent_dir->type = 2;
        parent_dir->inode = parent->inode;
        parent_dir->name_len =  2;
        parent_dir->size = fs->bsize - 24;
        parent_dir->name[0] = '.';
        parent_dir->name[1] = '.';

        ext2fs_write_dblock(fs, &child_inode, 0, block_buf);
    }

    return res;
}

static int
ext2_mknod(struct vnode *parent, const char *name, mode_t mode, dev_t dev)
{
    uint32_t inum;
    struct ext2_inode child_inode;

    ext2fs_lock(parent->state);

    int res = ext2fs_mknod(parent, name, mode, &inum, &child_inode);

    ext2fs_unlock(parent->state);

    return res;
}

static int
ext2_seek(struct vnode *vn, off_t *cur_pos, off_t off, int whence)
{
    struct ext2_inode inode;
    off_t new_pos;

    switch (whence) {
        case SEEK_CUR:
            new_pos = *cur_pos + off;
            break;
        case SEEK_END:
            if (ext2fs_read_inode((struct ext2fs*)vn->state, vn->inode, &inode) != 0) {
                return -(EIO);
            }
            new_pos = inode.size - off;
            break;
        case SEEK_SET:
            new_pos = off;
            break;
    }

    if (new_pos < 0) {
        return -(ESPIPE);
    }

    *cur_pos = new_pos;

    return new_pos;
}

static int
ext2_stat(struct vnode *vn, struct stat *stat)
{
    struct ext2_inode inode;

    if (ext2fs_read_inode((struct ext2fs*)vn->state, vn->inode, &inode) != 0) {
        return -(EIO);
    }

    stat->st_ino = vn->inode;
    stat->st_mtime = inode.mtime;
    stat->st_ctime = inode.ctime;
    stat->st_atime = inode.atime;
    stat->st_uid = inode.uid;
    stat->st_gid = inode.gid;
    stat->st_size = inode.size;
    stat->st_mode = inode.mode;

    return 0;
}

static int
ext2_truncate(struct vnode *vn, off_t length)
{
    int res = 0;
    struct ext2_inode inode;

    ext2fs_lock(vn->state);

    if (ext2fs_read_inode((struct ext2fs*)vn->state, vn->inode, &inode) != 0) {
        res = -(EIO);
        goto cleanup;
    }

    if (ext2fs_shrink_inode(vn->state, vn->inode, &inode, length) != 0) {
        res = -(EIO);
    }

cleanup:
    ext2fs_unlock(vn->state);

    return res;
}

static int
ext2_unlink(struct vnode *parent, const char *name)
{
    struct ext2_inode inode;

    int inum = -1;
    int res = 0;
    struct ext2fs *fs = parent->state;
    struct dirent dirent;

    ext2fs_lock(parent->state);

    for (int i = 0; ext2fs_fill_dirent(fs, parent->inode, i, &dirent) == 0; i++) {
        if (strcmp(name, dirent.name) == 0) {
            inum = dirent.inode;
            break;
        }
    }

    if (ext2fs_read_inode(fs, inum, &inode) != 0) {
        res = -(EIO);
        goto cleanup;
    }

    ext2fs_remove_dirent(parent, name);
    ext2fs_shrink_inode(fs, inum, &inode, 0);
    ext2fs_inode_free(fs, inum);
    
cleanup:
    ext2fs_unlock(parent->state);
    return res;
}

static int
ext2_utime(struct vnode *vn, struct timeval tv[2])
{
    struct ext2_inode inode;

    if (ext2fs_read_inode((struct ext2fs*)vn->state, vn->inode, &inode) != 0) {
        return -(EIO);
    }

    inode.atime = tv[0].tv_sec;
    inode.mtime = tv[1].tv_sec;

    if (ext2fs_write_inode(vn->state, vn->inode, &inode) != 0) {
        return -(EIO);
    }

    return 0;
}

static int
ext2_write(struct vnode *vn, const void *buf, size_t nbyte, uint64_t pos)
{
    int res = 0;
    struct ext2fs *fs = vn->state;
    uint8_t *block = fs->block_cache;
    struct ext2_inode inode;
    size_t start = pos;
    size_t end = start + nbyte;

    if (ext2fs_read_inode(fs, vn->inode, &inode) != 0) {
        res = -(EIO);
        goto cleanup;
    }

    if (!S_ISREG(inode.mode)) {
        res = -(EISDIR);
        goto cleanup;
    }

    if (end > inode.size) {
        ext2fs_grow_inode(fs, vn->inode, &inode, end);
    }

    int bytes_read = 0;
    uint64_t start_block = pos / fs->bsize;
    uint64_t start_offset = pos % fs->bsize;
    char *im_not_crazy = (char*)buf;

    for (uint64_t i = 0; bytes_read < nbyte; i++) {
        uint32_t bytes_this_block = MIN(fs->bsize-start_offset, nbyte - bytes_read);

        ext2fs_read_dblock(fs, &inode, start_block + i, block);
        memcpy(&block[start_offset], &im_not_crazy[bytes_read], bytes_this_block);
        ext2fs_write_dblock(fs, &inode, start_block + i, block);

        start_offset = 0;
        bytes_read += bytes_this_block;
    }

    inode.mtime = time(NULL);
    inode.atime = inode.mtime;

    if (ext2fs_write_inode(vn->state, vn->inode, &inode) != 0) {
        res = -(EIO);
    } else {
        res = bytes_read;
    }

cleanup:
    ext2fs_unlock(vn->state);

    return res;
}

void
ext2_init()
{
    fs_register("ext2", &ext2_ops);
}
