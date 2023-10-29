/**
 * This is a simple program that takes a file and reads a random block of data
 * from it, putting it in some memory mapped user space, using io uring. This
 * is simply meant to be a proof of concept.
 */

#include <liburing.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>

#define QUEUE_DEPTH 2
#define BLOCK_SZ 1024
#define FILE_NAME "data/random.txt"

typedef struct page_s {
  struct iovec vec;
  off_t offset;
  int fd;
} page_t;

int open_file(char *fname) {
  int fd = open(fname, O_RDONLY);
  if (fd < 0) {
    perror("unable to open a file.");
    return fd;
  }
  return fd;
}

void submit_get_page_request(struct io_uring *ring, page_t *page) {
  /* Get an SQE */
  struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
  /* Setup a readv operation */
  io_uring_prep_readv(sqe, page->fd, &page->vec, 1, page->offset);
  /* Set user data */
  io_uring_sqe_set_data(sqe, page);
  /* Finally, submit the request */
  io_uring_submit(ring);
}

int get_completion_event(page_t **dst, struct io_uring *ring) {
  struct io_uring_cqe *cqe;
  // blocking wait for completion queue
  int ret = io_uring_wait_cqe(ring, &cqe);
  if (ret < 0) {
    perror("io_uring_wait_cqe");
    return 1;
  }
  if (cqe->res < 0) {
    fprintf(stderr, "Async readv failed.\n");
    return 1;
  }

  *dst = io_uring_cqe_get_data(cqe);
  return 0;
}

void print_page(page_t *page) {
  char *buf = page->vec.iov_base;
  int len = page->vec.iov_len;

  while (len--) {
    fputc(*buf++, stdout);  
  }
  fputc('\n', stdout);
}

/**
 * Allocates BLOCK_SZ amount of data in memory and returns a pointer. All
 * allocated memory should likely be allocated at the begining of the program's
 * execution. From there on, this memory should be managed using some form of
 * memory pools/arenas. Memory should likely not be allocated dynamically during
 * execution.
 */
void *get_aligned_memory() {
  void *buf;
  if (posix_memalign(&buf, BLOCK_SZ, BLOCK_SZ)) {
    perror("posix_memalign");
    return NULL;
  }
  return buf;
}

int main(int argc, char **argv) {
  struct io_uring ring;
  int block_number;
  int fd;
  int rc;

  if (argc != 2) {
    printf("Usage: %s [block_number]\n", argv[0]);
    return -1;
  }

  block_number = atoi(argv[1]);
  io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
  fd = open_file(FILE_NAME);

  // setup page for DMA
  page_t *page = malloc(sizeof(page_t));
  page->vec.iov_base = get_aligned_memory();
  page->vec.iov_len = BLOCK_SZ;
  page->fd = fd;
  page->offset = block_number * BLOCK_SZ;

  // submit job to submission queue
  submit_get_page_request(&ring, page);
  rc = get_completion_event(&page, &ring);
  if (rc) {
    perror("get completion queue event");
    return rc;
  }

  printf("retrieved the following page from disk:\n");
  print_page(page);

  io_uring_queue_exit(&ring);
  close(fd);
  return 0;
}
