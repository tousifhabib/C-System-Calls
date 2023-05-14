#include <limits.h>

#include <assert.h>

#include <errno.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <stdbool.h>

#include <sys/uio.h>

#include "assign5.h"


#define ROOT_DIR 1
#define ASSIGN_DIR 2
#define USERNAME_FILE 3
#define FEATURE_FILE 4
#define FILE_COUNT 1000
#define MAX_FILES 1024

typedef struct file_node {
  bool is_directory;
  char name[64];
  fuse_ino_t parent_inode;
}
file_node;

typedef struct file_data {
  char * content;
  size_t size;
}
file_data;

struct stat * file_stats;
file_data * file_contents;
struct file_node * helper_array;
int current_file_count = 4;

static
const int AllRead = S_IRUSR | S_IRGRP | S_IROTH;
static
const int AllExec = S_IXUSR | S_IXGRP | S_IXOTH;
static
const int AllPermissions = S_IRWXU | S_IRWXG | S_IRWXO;

static
const char UsernameContent[] = "thabib\n";
static
const char FeaturesContents[] = "The following features are implemented:\n"
"-Creating and removing directories\n"
"-Listing directories\n"
"-File creation\n"
"-Unlinking files\n"
"-Permission setting\n"
"-Writing to file\n";

const char * get_response_data(const char * content, off_t off, size_t size, size_t * response_len);
void clear_file_entry(int index);

static void assign5_init(void * userdata, struct fuse_conn_info * conn) {
  struct backing_file * backing = userdata;
  fprintf(stderr, "*** %s '%s'\n", __func__, backing -> bf_path);

  file_stats = calloc(FILE_COUNT + 1, sizeof( * file_stats));
  helper_array = calloc(FILE_COUNT + 1, sizeof(struct file_node));
  file_contents = calloc(FILE_COUNT + 1, sizeof(struct file_data));

  struct {
    fuse_ino_t ino;
    mode_t mode;
    int is_directory;
    fuse_ino_t parent_inode;
    const char * name;
  }
  init_files[] = {
    {
      ROOT_DIR,
      S_IFDIR | AllPermissions | AllPermissions,
      1,
      -1,
      "root"
    },
    {
      ASSIGN_DIR,
      S_IFDIR | AllRead | AllExec,
      0,
      ROOT_DIR,
      "assignment"
    },
    {
      USERNAME_FILE,
      S_IFREG | AllRead,
      0,
      ASSIGN_DIR,
      "username"
    },
    {
      FEATURE_FILE,
      S_IFREG | AllRead,
      0,
      ASSIGN_DIR,
      "feature"
    },
  };

  for (int i = 0; i < 4; ++i) {
    struct file_node * node = & helper_array[init_files[i].ino];
    struct stat * stat = & file_stats[init_files[i].ino];

    stat -> st_ino = init_files[i].ino;
    stat -> st_mode = init_files[i].mode;
    stat -> st_nlink = 1;
    node -> is_directory = init_files[i].is_directory;
    node -> parent_inode = init_files[i].parent_inode;
    strcpy(node -> name, init_files[i].name);

    if (init_files[i].ino == USERNAME_FILE) {
      stat -> st_size = sizeof(UsernameContent);
    } else if (init_files[i].ino == FEATURE_FILE) {
      stat -> st_size = sizeof(FeaturesContents);
    }
  }

  current_file_count = 4;
}

static void assign5_destroy(void * userdata) {
  struct backing_file * backing = userdata;
  fprintf(stderr, "*** %s %d\n", __func__, backing -> bf_fd);

  free(file_stats);
  file_stats = NULL;
  free(helper_array);
  helper_array = NULL;
  free(file_contents);
  file_contents = NULL;
}

static void assign5_create(fuse_req_t req, fuse_ino_t parent, const char * name, mode_t mode, struct fuse_file_info * fi) {
  if (parent == ASSIGN_DIR) {
    // Files cannot be created in the assignment directory
    fuse_reply_err(req, EPERM);
    return;
  }

  struct fuse_entry_param dirent;
  current_file_count++;

  file_stats[current_file_count].st_ino = current_file_count;
  file_stats[current_file_count].st_mode = S_IFREG | mode;
  file_stats[current_file_count].st_size = 0;
  file_stats[current_file_count].st_nlink = 1;

  helper_array[current_file_count].is_directory = 0;
  helper_array[current_file_count].parent_inode = parent;
  strcpy(helper_array[current_file_count].name, name);

  dirent.generation = 1;
  dirent.attr_timeout = 1;
  dirent.entry_timeout = 1;
  dirent.ino = current_file_count;
  dirent.attr = file_stats[current_file_count];

  int result = fuse_reply_create(req, & dirent, fi);
  if (result != 0) {
    fprintf(stderr, "Failed to send dirent reply\n");
  }
}

static void
assign5_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info * fip) {
  if (ino > current_file_count) {
    fuse_reply_err(req, ENOENT);
    return;
  }

  int result = fuse_reply_attr(req, file_stats + ino, 1);
  if (result != 0) {
    fprintf(stderr, "Failed to send attr reply\n");
  }
}

static void
assign5_lookup(fuse_req_t req, fuse_ino_t parent, const char * name) {
  struct fuse_entry_param dirent;

  for (int i = 0; i <= current_file_count; i++) {
    if (helper_array[i].parent_inode == parent &&
      strcmp(helper_array[i].name, name) == 0) {

      dirent.generation = 1;
      dirent.attr_timeout = 1;
      dirent.entry_timeout = 1;
      dirent.ino = i;
      dirent.attr = file_stats[i];

      int result = fuse_reply_entry(req, & dirent);
      if (result != 0) {
        fprintf(stderr, "Failed to send dirent reply\n");
      }
      return;
    }
  }

  fuse_reply_err(req, ENOENT);
}

static void assign5_mkdir(fuse_req_t req, fuse_ino_t parent, const char * name, mode_t mode) {
  if (parent == ASSIGN_DIR) {
    // Directories cannot be created in the assignment directory
    fuse_reply_err(req, EPERM);
    return;
  }

  struct fuse_entry_param dirent;
  current_file_count++;

  file_stats[current_file_count].st_ino = current_file_count;
  file_stats[current_file_count].st_mode = S_IFDIR | AllPermissions | AllPermissions;
  file_stats[current_file_count].st_nlink = 1;

  helper_array[current_file_count].is_directory = 1;
  helper_array[current_file_count].parent_inode = parent;
  strcpy(helper_array[current_file_count].name, name);

  dirent.generation = 1;
  dirent.attr_timeout = 1;
  dirent.entry_timeout = 1;
  dirent.ino = current_file_count;
  dirent.attr = file_stats[current_file_count];

  int result = fuse_reply_entry(req, & dirent);
  if (result != 0) {
    fprintf(stderr, "Failed to send dirent reply\n");
  }
}

static void
assign5_mknod(fuse_req_t req, fuse_ino_t parent,
  const char * name,
    mode_t mode, dev_t rdev) {
  if (parent == ASSIGN_DIR) {
    // Nodes cannot be created in the assignment directory
    fuse_reply_err(req, EPERM);
    return;
  }

  struct fuse_entry_param dirent;
  current_file_count++;
  file_stats[current_file_count].st_ino = current_file_count;
  file_stats[current_file_count].st_mode = mode;
  file_stats[current_file_count].st_nlink = 1;
  helper_array[current_file_count].is_directory = 1;
  helper_array[current_file_count].parent_inode = parent;
  strcpy(helper_array[current_file_count].name, name);

  dirent.generation = 1;
  dirent.attr_timeout = 1;
  dirent.entry_timeout = 1;
  dirent.ino = current_file_count;
  dirent.attr = file_stats[current_file_count];

  int result = fuse_reply_entry(req, & dirent);
  if (result != 0) {
    fprintf(stderr, "Failed to send dirent reply\n");
  }
}

static void
assign5_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info * fi) {
  fprintf(stderr, "%s ino=%zu\n", __func__, ino);

  if (ino > current_file_count || ino < USERNAME_FILE) {
    fuse_reply_err(req, ENOENT);
    return;
  }

  if (!S_ISREG(file_stats[ino].st_mode)) {
    fuse_reply_err(req, EISDIR);
    return;
  }

  if ((fi -> flags & 3) != O_RDONLY) {
    fuse_reply_err(req, EACCES);
    return;
  }

  fuse_reply_open(req, fi);
}

static void assign5_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
  off_t off, struct fuse_file_info * fi) {
  if (off > 2) {
    fuse_reply_buf(req, NULL, 0);
    return;
  }

  struct stat * self = & file_stats[ino];
  if (!S_ISDIR(self -> st_mode)) {
    fuse_reply_err(req, ENOTDIR);
    return;
  }

  char buffer[size];
  off_t bytes_accumulated = 0;
  int next_entry = 0;

  struct {
    const char * name;
    struct stat * stbuf;
  }
  entries[] = {
    {
      ".",
      self
    },
    {
      "..",
      & file_stats[ROOT_DIR]
    },
  };

  for (size_t i = off; i < sizeof(entries) / sizeof(entries[0]); ++i) {
    bytes_accumulated += fuse_add_direntry(req, buffer + bytes_accumulated,
      size - bytes_accumulated,
      entries[i].name, entries[i].stbuf, ++next_entry);
  }

  for (int j = 1; j <= current_file_count; j++) {
    if (helper_array[j].parent_inode == ino) {
      bytes_accumulated += fuse_add_direntry(req, buffer + bytes_accumulated,
        size - bytes_accumulated,
        helper_array[j].name, &
        file_stats[file_stats[j].st_ino],
        ++next_entry);
    }
  }

  int result = fuse_reply_buf(req, buffer, bytes_accumulated);
  if (result != 0) {
    fprintf(stderr, "Failed to send readdir reply\n");
  }
}

static void assign5_read(fuse_req_t req, fuse_ino_t ino, size_t size,
  off_t off, struct fuse_file_info * fi) {
  const char * response_data = NULL;
  size_t response_len;
  int err = 0;

  switch (ino) {
  case ROOT_DIR:
  case ASSIGN_DIR:
    err = EISDIR;
    break;

  case USERNAME_FILE:
    response_data = get_response_data(UsernameContent, off, size, & response_len);
    break;

  case FEATURE_FILE:
    response_data = get_response_data(FeaturesContents, off, size, & response_len);
    break;

  default:
    if (ino > FEATURE_FILE) {
      if (file_contents[ino].content) {
        response_data = get_response_data(file_contents[ino].content, off, size, & response_len);
      } else {
        err = EBADF;
      }
    } else {
      err = EBADF;
    }
    break;
  }

  if (err != 0) {
    fuse_reply_err(req, err);
    return;
  }

  int result = fuse_reply_buf(req, response_data, response_len);
  if (result != 0) {
    fprintf(stderr, "Failed to send read reply\n");
  }
}

const char * get_response_data(const char * content, off_t off, size_t size, size_t * response_len) {
  if (off >= strlen(content)) {
    * response_len = 0;
    return NULL;
  }

  * response_len = strlen(content) - off;
  if ( * response_len > size) {
    * response_len = size;
  }

  return content + off;
}

static void
assign5_rmdir(fuse_req_t req, fuse_ino_t parent,
  const char * name) {
  for (int i = 1; i < current_file_count + 1; i++) {
    if (helper_array[i].parent_inode == parent && strcmp(name, helper_array[i].name) == 0) {
      file_stats[i].st_ino = NULL;
      strcpy(helper_array[i].name, "");
      helper_array[i].parent_inode = -1;
      fuse_reply_err(req, 0);
    }
  }
}

static void
assign5_setattr(fuse_req_t req, fuse_ino_t ino, struct stat * attr, int to_set, struct fuse_file_info * fi) {
  if (ino > current_file_count) {
    fuse_reply_err(req, ENOENT);
    return;
  }

  if (to_set & FUSE_SET_ATTR_MODE) {
    file_stats[ino].st_mode = (file_stats[ino].st_mode & S_IFMT) | (attr -> st_mode & 07777);
  }

  int result = fuse_reply_attr(req, file_stats + ino, 1);
  if (result != 0) {
    fprintf(stderr, "Failed to send attr reply\n");
  }
}

static void
assign5_statfs(fuse_req_t req, fuse_ino_t ino) {
  fprintf(stderr, "%s ino=%zu\n", __func__, ino);
  fuse_reply_err(req, ENOSYS);
}

static void assign5_unlink(fuse_req_t req, fuse_ino_t parent,
  const char * name) {
  fprintf(stderr, "%s parent=%zu name='%s'\n", __func__, parent, name);

  for (int i = 1; i <= current_file_count; i++) {
    if (helper_array[i].parent_inode == parent && strcmp(name, helper_array[i].name) == 0) {
      clear_file_entry(i);
      fuse_reply_err(req, 0);
    }
  }
}

void clear_file_entry(int index) {
  file_stats[index].st_ino = NULL;
  strcpy(helper_array[index].name, "");
  helper_array[index].parent_inode = -1;
}

static void assign5_write(fuse_req_t req, fuse_ino_t ino,
  const char * buf, size_t size,
    off_t off, struct fuse_file_info * fi) {
  fprintf(stderr, "%s ino=%zu size=%zu off=%zd\n", __func__,
    ino, size, off);

  if (ino > current_file_count || ino < USERNAME_FILE) {
    fuse_reply_err(req, ENOENT);
    return;
  }

  if (!S_ISREG(file_stats[ino].st_mode)) {
    fuse_reply_err(req, EISDIR);
    return;
  }

  if (off + size > file_contents[ino].size) {
    file_contents[ino].content = realloc(file_contents[ino].content, off + size);
    file_contents[ino].size = off + size;
    file_stats[ino].st_size = off + size;
  }

  memcpy(file_contents[ino].content + off, buf, size);

  fuse_reply_write(req, size);
}

static struct fuse_lowlevel_ops assign5_ops = {
  .init = assign5_init,
  .destroy = assign5_destroy,

  .create = assign5_create,
  .getattr = assign5_getattr,
  .lookup = assign5_lookup,
  .mkdir = assign5_mkdir,
  .mknod = assign5_mknod,
  .open = assign5_open,
  .read = assign5_read,
  .readdir = assign5_readdir,
  .rmdir = assign5_rmdir,
  .setattr = assign5_setattr,
  .statfs = assign5_statfs,
  .unlink = assign5_unlink,
  .write = assign5_write,
};

struct fuse_lowlevel_ops *
  assign5_fuse_ops() {
    return & assign5_ops;
  }