#ifndef EXTSTORE_H
#define EXTSTORE_H

/* Pages can have objects deleted from them at any time. This creates holes
 * that can't be reused until the page is either evicted or all objects are
 * deleted.
 * bytes_fragmented is the total bytes for all of these holes.
 * It is the size of all used pages minus each page's bytes_used value.
 */
struct extstore_stats {
    uint64_t page_allocs;
    uint64_t page_evictions;
    uint64_t pages_free; /* currently unallocated/unused pages */
    uint64_t pages_used;
    uint64_t objects_evicted;
    uint64_t objects_read;
    uint64_t objects_written;
    uint64_t objects_used; /* total number of objects stored */
    uint64_t bytes_evicted;
    uint64_t bytes_written;
    uint64_t bytes_read; /* wbuf - read -> bytes read from storage */
    uint64_t bytes_used; /* total number of bytes stored */
    uint64_t bytes_fragmented; /* see above comment */
};

// TODO: Temporary configuration structure. A "real" library should have an
// extstore_set(enum, void *ptr) which hides the implementation.
// this is plenty for quick development.
struct extstore_conf {
    unsigned int page_size; // ideally 64-256M in size
    unsigned int page_count;
    unsigned int page_buckets; // number of different writeable pages
    unsigned int wbuf_size; // must divide cleanly into page_size
    unsigned int wbuf_count; // this might get locked to "2 per active page"
    unsigned int io_threadcount;
    unsigned int io_depth; // with normal I/O, hits locks less. req'd for AIO
};

enum obj_io_mode {
    OBJ_IO_READ = 0,
    OBJ_IO_WRITE,
};

typedef struct _obj_io obj_io;
typedef void (*obj_io_cb)(void *e, obj_io *io, int ret);

/* An object for both reads and writes to the storage engine.
 * Once an IO is submitted, ->next may be changed by the IO thread. It is not
 * safe to further modify the IO stack until the entire request is completed.
 */
struct _obj_io {
    void *data; /* user supplied data pointer */
    struct _obj_io *next;
    char *buf;  /* buffer of data to read or write to */
    uint64_t page_version;     /* page version for read mode */
    unsigned int len;     /* for both modes */
    unsigned int page_id; /* for read mode */
    unsigned int offset;  /* for read mode */
    enum obj_io_mode mode;
    /* callback pointers? */
    obj_io_cb cb;
};

void *extstore_init(char *fn, struct extstore_conf *cf);
int extstore_write(void *ptr, unsigned int bucket, obj_io *io);
int extstore_submit(void *ptr, obj_io *io);
/* count are the number of objects being removed, bytes are the original
 * length of those objects. Bytes is optional but you can't track
 * fragmentation without it.
 */
int extstore_delete(void *ptr, unsigned int page_id, uint64_t page_version, unsigned int count, unsigned int bytes);
void extstore_get_stats(void *ptr, struct extstore_stats *st);

#endif
