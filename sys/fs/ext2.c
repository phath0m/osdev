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
};

void
ext2fs_init_block_buf(struct ext2fs *fs, struct ext2_block_buf *buf)
{
    buf->buf = calloc(1, fs->bsize);
    buf->block_addr = (uint32_t)-1;
}

int
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

int
ext2fs_read_bg(struct ext2fs *fs, uint64_t index, struct ext2_bg_desc *desc)
{
    uint64_t bg_addr = fs->bg_start + index*sizeof(struct ext2_bg_desc);

    if (cdev_read(fs->cdev, (char*)desc, sizeof(struct ext2_bg_desc), bg_addr) != sizeof(struct ext2_bg_desc)) {
        return -1;
    }

    return 0;
}

int
ext2fs_write_bg(struct ext2fs *fs, uint64_t index, struct ext2_bg_desc *desc)
{
    uint64_t bg_addr = fs->bg_start + index*sizeof(struct ext2_bg_desc);

    if (cdev_write(fs->cdev, (char*)desc, sizeof(struct ext2_bg_desc), bg_addr) != sizeof(struct ext2_bg_desc)) {
        return -1;
    }

    return 0;
}

int
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


int
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

int
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

int
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

int
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

int
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

uint32_t
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

    uint32_t ret = 0;

    for (int i = 0; i < fs->bsize && ret != 0; i++) {
        uint8_t byte = fs->block_cache[i];

        for (int b = 0; b < 8; b++) {
            if (((1<<b) & byte) == 0) {
                fs->block_cache[i] = (byte | (1<<b));
                ret = 8 * i + b;
                break;
            }

        }
    }

    if (ret != 0) {
        cdev_write(fs->cdev, (char*)fs->block_cache, fs->bsize, BLOCK_ADDR(fs->bsize, bg.b_bitmap));
        bg.num_free_blocks++;
        ext2fs_write_bg(fs, bgnum, &bg);
        /* rewrite superblock */
        return fs->superblock.first_dblock + bgnum*fs->bpg + ret;
    }

    return 0;
}

uint32_t
ext2fs_inode_alloc(struct ext2fs *fs, int preferred_bg)
{
    struct ext2_bg_desc bg;
	int64_t bgnum;

    for (uint64_t i = 0; i < fs->bg_count; i++) {
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

    uint32_t ret = 0;

    for (int i = 0; i < fs->bsize && ret != 0; i++) {
        uint8_t byte = fs->block_cache[i];

        for (int b = 0; b < 8; b++) {
            if (((1<<b) & byte) == 0) {
                fs->block_cache[i] = (byte | (1<<b));
                ret = 8 * i + b;
                break;
            }

        }
    }

    if (ret != 0) {
        cdev_write(fs->cdev, (char*)fs->block_cache, fs->bsize, BLOCK_ADDR(fs->bsize, bg.b_bitmap));
        bg.num_free_inodes++;
        ext2fs_write_bg(fs, bgnum, &bg);
        /* rewrite superblock */
        return fs->igp*bgnum + ret;
    }

    return 0;
}

static int
ext2_chmod(struct vnode *node, mode_t mode)
{
    return 0;
}

static int
ext2_chown(struct vnode *node, uid_t owner, gid_t group)
{
    return 0;
}

static int
ext2_creat(struct vnode *parent, struct vnode **child, const char *name, mode_t mode)
{
    return 0;
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
    return 0;
}

static int
ext2_mknod(struct vnode *parent, const char *name, mode_t mode, dev_t dev)
{
    return 0;
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
ext2_truncate(struct vnode *node, off_t length)
{
    return 0;
}

static int
ext2_unlink(struct vnode *parent, const char *dirname)
{
    return 0;
}

static int
ext2_utime(struct vnode *node, struct timeval tv[2])
{
    return 0;
}

static int
ext2_write(struct vnode *node, const void *buf, size_t nbyte, uint64_t pos)
{
    return 0;
}

void
ext2_init()
{
    fs_register("ext2", &ext2_ops);
}
