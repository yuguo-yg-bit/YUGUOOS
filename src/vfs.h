#pragma once
#include <stdint.h>
#include <stddef.h>

#define VFS_MAX_FD       64
#define VFS_MAX_MOUNTS   8
#define VFS_MAX_PATH     256
#define VFS_MAX_FILENAME 128

#define O_RDONLY  0
#define O_WRONLY  1
#define O_RDWR    2
#define O_CREAT   4

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef enum {
    VFS_FILE,
    VFS_DIR,
    VFS_DEVICE,
    VFS_SYMLINK,
} vfs_node_type_t;

typedef struct vfs_node vfs_node_t;

struct vfs_node {
    char             name[VFS_MAX_FILENAME];
    vfs_node_type_t  type;
    size_t           size;
    vfs_node_t      *parent;
    vfs_node_t      *children;
    vfs_node_t      *next_sibling;
    void            *data;
};

typedef struct {
    int           fd;
    vfs_node_t   *node;
    int           flags;
    size_t        offset;
    int           used;
} vfs_file_t;

typedef struct {
    char         mount_point[VFS_MAX_PATH];
    vfs_node_t  *root;
} vfs_mount_t;

void        vfs_init(void);
int         vfs_open(const char *path, int flags);
int         vfs_close(int fd);
int         vfs_read(int fd, void *buf, size_t count);
int         vfs_write(int fd, const void *buf, size_t count);
int         vfs_seek(int fd, int offset, int whence);
vfs_node_t *vfs_lookup(const char *path);
int         vfs_create(const char *path, vfs_node_type_t type);
int         vfs_mkdir(const char *path);
int         vfs_mount(const char *mount_point, vfs_node_t *root);
void        vfs_dump_tree(void);