#include "vfs.h"
#include "vmm.h"
#include "serial.h"
#include <stdbool.h>

static vfs_node_t  *vfs_root = NULL;
static vfs_mount_t  vfs_mounts[VFS_MAX_MOUNTS];
static vfs_file_t   vfs_fd_table[VFS_MAX_FD];
static int          vfs_mount_count = 0;

static vfs_node_t *vfs_alloc_node(const char *name, vfs_node_type_t type)
{
    vfs_node_t *node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    if (!node) return NULL;

    int i = 0;
    while (i < VFS_MAX_FILENAME - 1 && name[i]) {
        node->name[i] = name[i];
        i++;
    }
    node->name[i] = '\0';
    node->type = type;
    node->size = 0;
    node->parent = NULL;
    node->children = NULL;
    node->next_sibling = NULL;
    node->data = NULL;

    return node;
}

static vfs_node_t *vfs_find_child(vfs_node_t *parent, const char *name)
{
    if (!parent) return NULL;
    vfs_node_t *child = parent->children;
    while (child) {
        int i = 0;
        bool match = true;
        while (name[i] && child->name[i]) {
            if (name[i] != child->name[i]) {
                match = false;
                break;
            }
            i++;
        }
        if (match && name[i] == '\0' && child->name[i] == '\0') {
            return child;
        }
        child = child->next_sibling;
    }
    return NULL;
}

static void vfs_add_child(vfs_node_t *parent, vfs_node_t *child)
{
    if (!parent || !child) return;
    child->parent = parent;
    child->next_sibling = parent->children;
    parent->children = child;
}

static int vfs_alloc_fd(void)
{
    for (int i = 0; i < VFS_MAX_FD; i++) {
        if (!vfs_fd_table[i].used) {
            vfs_fd_table[i].used = true;
            return i;
        }
    }
    return -1;
}

void vfs_init(void)
{
    vfs_root = vfs_alloc_node("", VFS_DIR);

    for (int i = 0; i < VFS_MAX_MOUNTS; i++) {
        vfs_mounts[i].mount_point[0] = '\0';
        vfs_mounts[i].root = NULL;
    }
    vfs_mount_count = 0;

    for (int i = 0; i < VFS_MAX_FD; i++) {
        vfs_fd_table[i].used = false;
        vfs_fd_table[i].fd = i;
        vfs_fd_table[i].node = NULL;
        vfs_fd_table[i].flags = 0;
        vfs_fd_table[i].offset = 0;
    }

    vfs_create("/dev", VFS_DIR);
    vfs_create("/tmp", VFS_DIR);
    vfs_create("/mnt", VFS_DIR);

    serial_write("[VFS] 虚拟文件系统初始化完成\n");
}

vfs_node_t *vfs_lookup(const char *path)
{
    if (!path || !vfs_root) return NULL;

    if (path[0] == '/' && path[1] == '\0') {
        return vfs_root;
    }

    vfs_node_t *current = vfs_root;
    char component[VFS_MAX_FILENAME];
    int ci = 0;
    int pi = 0;

    while (path[pi]) {
        if (path[pi] == '/') {
            pi++;
            continue;
        }

        ci = 0;
        while (path[pi] && path[pi] != '/' && ci < VFS_MAX_FILENAME - 1) {
            component[ci++] = path[pi++];
        }
        component[ci] = '\0';

        if (ci > 0) {
            current = vfs_find_child(current, component);
            if (!current) return NULL;
        }
    }

    return current;
}

int vfs_open(const char *path, int flags)
{
    vfs_node_t *node = vfs_lookup(path);
    if (!node) {
        if (flags & O_CREAT) {
            vfs_create(path, VFS_FILE);
            node = vfs_lookup(path);
        }
        if (!node) return -1;
    }

    int fd = vfs_alloc_fd();
    if (fd < 0) return -1;

    vfs_fd_table[fd].node = node;
    vfs_fd_table[fd].flags = flags;
    vfs_fd_table[fd].offset = 0;

    return fd;
}

int vfs_close(int fd)
{
    if (fd < 0 || fd >= VFS_MAX_FD || !vfs_fd_table[fd].used) return -1;
    vfs_fd_table[fd].used = false;
    vfs_fd_table[fd].node = NULL;
    return 0;
}

int vfs_read(int fd, void *buf, size_t count)
{
    if (fd < 0 || fd >= VFS_MAX_FD || !vfs_fd_table[fd].used) return -1;

    vfs_node_t *node = vfs_fd_table[fd].node;
    if (!node || !node->data) return 0;

    if (vfs_fd_table[fd].offset >= node->size) return 0;

    size_t remaining = node->size - vfs_fd_table[fd].offset;
    size_t to_read = (count < remaining) ? count : remaining;

    if (to_read == 0) return 0;

    uint8_t *src = (uint8_t *)node->data + vfs_fd_table[fd].offset;
    uint8_t *dst = (uint8_t *)buf;
    for (size_t i = 0; i < to_read; i++) {
        dst[i] = src[i];
    }

    vfs_fd_table[fd].offset += to_read;
    return (int)to_read;
}

int vfs_write(int fd, const void *buf, size_t count)
{
    if (fd < 0 || fd >= VFS_MAX_FD || !vfs_fd_table[fd].used) return -1;

    vfs_node_t *node = vfs_fd_table[fd].node;
    if (!node) return -1;

    size_t new_size = vfs_fd_table[fd].offset + count;

    if (new_size > node->size) {
        uint8_t *new_data = (uint8_t *)kmalloc(new_size);
        if (!new_data) return -1;

        if (node->data) {
            for (size_t i = 0; i < node->size; i++) {
                new_data[i] = ((uint8_t*)node->data)[i];
            }
            kfree(node->data);
        }

        for (size_t i = node->size; i < new_size; i++) {
            new_data[i] = 0;
        }

        node->data = new_data;
        node->size = new_size;
    }

    const uint8_t *src = (const uint8_t *)buf;
    uint8_t *dst = (uint8_t *)node->data + vfs_fd_table[fd].offset;
    for (size_t i = 0; i < count; i++) {
        dst[i] = src[i];
    }

    vfs_fd_table[fd].offset += count;
    return (int)count;
}

int vfs_seek(int fd, int offset, int whence)
{
    if (fd < 0 || fd >= VFS_MAX_FD || !vfs_fd_table[fd].used) return -1;

    vfs_node_t *node = vfs_fd_table[fd].node;
    if (!node) return -1;

    switch (whence) {
    case SEEK_SET:
        if (offset < 0) return -1;
        vfs_fd_table[fd].offset = (size_t)offset;
        break;
    case SEEK_CUR:
        if ((int)vfs_fd_table[fd].offset + offset < 0) return -1;
        vfs_fd_table[fd].offset = (size_t)((int)vfs_fd_table[fd].offset + offset);
        break;
    case SEEK_END:
        if ((int)node->size + offset < 0) return -1;
        vfs_fd_table[fd].offset = (size_t)((int)node->size + offset);
        break;
    default:
        return -1;
    }

    return 0;
}

int vfs_create(const char *path, vfs_node_type_t type)
{
    if (!path || !vfs_root) return -1;

    char parent_path[VFS_MAX_PATH];
    char filename[VFS_MAX_FILENAME];

    int pi = 0;
    int last_slash = -1;

    while (path[pi]) {
        if (path[pi] == '/') last_slash = pi;
        pi++;
    }

    if (last_slash < 0) return -1;

    for (int i = 0; i <= last_slash && i < VFS_MAX_PATH - 1; i++) {
        parent_path[i] = path[i];
    }
    parent_path[last_slash] = '\0';
    if (last_slash == 0) parent_path[0] = '/';
    parent_path[last_slash + 1] = '\0';

    int fi = 0;
    for (int i = last_slash + 1; path[i] && fi < VFS_MAX_FILENAME - 1; i++) {
        filename[fi++] = path[i];
    }
    filename[fi] = '\0';

    vfs_node_t *parent = vfs_lookup(parent_path);
    if (!parent) {
        if (parent_path[0] == '/' && last_slash == 0) {
            parent = vfs_root;
        } else {
            return -1;
        }
    }

    if (vfs_find_child(parent, filename)) return -1;

    vfs_node_t *node = vfs_alloc_node(filename, type);
    if (!node) return -1;

    vfs_add_child(parent, node);
    return 0;
}

int vfs_mkdir(const char *path)
{
    return vfs_create(path, VFS_DIR);
}

int vfs_mount(const char *mount_point, vfs_node_t *root)
{
    if (vfs_mount_count >= VFS_MAX_MOUNTS) return -1;

    vfs_node_t *mp = vfs_lookup(mount_point);
    if (!mp) return -1;

    int i = 0;
    while (mount_point[i] && i < VFS_MAX_PATH - 1) {
        vfs_mounts[vfs_mount_count].mount_point[i] = mount_point[i];
        i++;
    }
    vfs_mounts[vfs_mount_count].mount_point[i] = '\0';
    vfs_mounts[vfs_mount_count].root = root;
    vfs_mount_count++;

    return 0;
}

void vfs_dump_tree(void)
{
    serial_write("[VFS] 文件系统树:\n");

    if (!vfs_root) return;

    vfs_node_t *stack[64];
    int depths[64];
    int sp = 0;

    stack[0] = vfs_root;
    depths[0] = 0;
    sp = 1;

    while (sp > 0) {
        sp--;
        vfs_node_t *node = stack[sp];
        int depth = depths[sp];

        for (int i = 0; i < depth; i++) {
            serial_write("  ");
        }

        if (node == vfs_root) {
            serial_write("/ (root)\n");
        } else {
            serial_write(node->name);
            if (node->type == VFS_DIR) {
                serial_write("/");
            }
            serial_write(" (");
            switch (node->type) {
            case VFS_FILE: serial_write("file"); break;
            case VFS_DIR:  serial_write("dir"); break;
            case VFS_DEVICE: serial_write("dev"); break;
            case VFS_SYMLINK: serial_write("sym"); break;
            }
            serial_write(")\n");
        }

        vfs_node_t *child = node->children;
        while (child) {
            if (sp < 64) {
                stack[sp] = child;
                depths[sp] = depth + 1;
                sp++;
            }
            child = child->next_sibling;
        }
    }
}