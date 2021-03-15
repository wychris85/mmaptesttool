#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>


#ifdef _MAP_BYTE_
#define MAP_TYPE_T uint8_t
#define FORMAT1 "%c"
#define FORMAT2 "%p: %c"
#endif

#ifdef _MAP_WORD_
#define MAP_TYPE_T uint32_t
#define FORMAT1 "%#x"
#define FORMAT2 "%p: %#x"
#endif

#ifdef _MAP_DWORD_
#define MAP_TYPE_T uint64_t
#define FORMAT1 "%#lx"
#define FORMAT2 "%p: %#lx"
#endif

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while(0)

#define yes_or_no(r) ((r) == 1 ? "YES" : "NO")

static volatile MAP_TYPE_T * base_addr = NULL;
static volatile MAP_TYPE_T * tmp_addr = NULL;
static long unsigned int map_len = 0;

static int incr = 0;
static int rd_only = 0;
static int wr_only = 0;
static int rd_wr = 0;
static char * fname = NULL;
static long long int naddr_count = 0;
static long long unsigned int start_val = 0;
static long long unsigned int loop_count = 1;
static long long unsigned int data_count = 0;
static long long unsigned int diff_count = 0;

void do_read(void);
void do_write(void);
void do_read_write_read(void);
void print_data(MAP_TYPE_T * addr, int disp_addr);
void print_help(void);
void print_stats(void);
void print_options(void);

int main(int argc, char** argv)
{

  /*options and arguments*/
  int help=0;
  int verbose=0;
  
  int opt;
    
  /*file descriptor*/
  int fd;
  
  struct stat finfo;
  long int offset = 0;
  long int pa_offset = 0;
    
  while ((opt = getopt (argc, argv, ":hil:n:rs:vwx")) != -1)
    switch (opt)
      {
      case 'h':
	help = 1;
	break;
      case 'i':
	incr = 1;
	break;
      case 'l':
	loop_count = strtoll(optarg, NULL, 0);
	break;
      case 'n':
	naddr_count = strtoll(optarg, NULL, 0);
	break;
      case 'r':
	rd_only = 1;
	break;
      case 's':
	start_val = strtoll(optarg, NULL, 0);
	break;
      case 'v':
	verbose = 1;
	break;
      case 'w':
	wr_only = 1;
	break;
      case 'x':
	rd_wr = 1;
	break;
      case '?':
	printf("unknown option\n");
	break;
      default:
	help = 1;
	break;
      }

  for (; optind < argc; optind++) {
    fname = argv[optind];
    break;
  }
  
  if ((help==1) || (fname==NULL) || (strlen(fname) <= 1)) {
    print_help();
    return 0;
  }

  if (verbose==1) print_options();

  printf("\n");
  if (verbose==1) printf("\t(0) START\n");

  if (access(fname, F_OK) < 0)
    handle_error("file doesn't exist\n");

  if (access(fname, R_OK | W_OK) == -1)
    handle_error("read and/or write access permission not granted\n");

  if (verbose==1) printf("\t(1) CHECK FILE\n");

  if (verbose==1) printf("\t(2) OPEN file\n");
  fd = open(fname, O_RDWR);
  if (fd == -1) handle_error("open\n");
  
  if (fstat(fd, &finfo) < 0)
    {
      close(fd);
      handle_error("fstat\n");
    }

  pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) -1);
  map_len = finfo.st_size + offset -  pa_offset;
  base_addr = (MAP_TYPE_T *) mmap(NULL, map_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, pa_offset);

  if (verbose==1) printf("\t(3) MAPPED at address=%p, length=%ld, data-size=%ld\n", base_addr, map_len, sizeof(MAP_TYPE_T));
  
  if (base_addr == MAP_FAILED)
    {
      close(fd);
      handle_error("mmap - error\n");
    }

  if (verbose==1) printf("\t(4) READ AND/OR WRITE\n");
  if (rd_only == 1) do_read();
  if (wr_only == 1) do_write();
  if (rd_wr == 1) do_read_write_read();

  printf("\n");
  
  if (verbose==1) printf("\t(5) UNMAP\n");
  munmap((MAP_TYPE_T*)base_addr, map_len);

  if (verbose==1) printf("\t(6) CLOSE file\n");
  close(fd);

  if (verbose==1) printf("\t(7) FINISHED successfully\r\n");
  
  return 0;
}

void do_read(void)
{
  int dsize = sizeof(MAP_TYPE_T);

  printf("\n\t\t\tREAD\n");

  for (long long unsigned int l=0; l<loop_count; l++) {

    tmp_addr = base_addr;
      
    for (long long unsigned int i=0; i<map_len/dsize; i++) {

      if (naddr_count > 0 && i >= naddr_count)
	continue;
      
      printf("\t(Loop=%.8lld, idx=%.8lld)\t", l, i);
      print_data((MAP_TYPE_T*)tmp_addr, 1);
      tmp_addr++;
      printf("\n");
    }
    
    printf("\n");
  }
  return;
}

void do_write(void)
{
  int dsize = sizeof(MAP_TYPE_T);
  
  printf("\n\t\t\tWRITE\n");
  
  MAP_TYPE_T val;

  //initializes random number generator
  srand((unsigned) time(0));

  for (long long unsigned int l=0; l<loop_count; l++) {

    tmp_addr = base_addr;
    val = (MAP_TYPE_T) start_val-1;

    for (long long unsigned int i=0; i<map_len/dsize; i++) {

      if (naddr_count > 0 && i >= naddr_count)
	continue;
      
      if (incr == 0)
	val = (MAP_TYPE_T) rand();
      else
	val++;
      
      *tmp_addr = val;

      printf("\t(Loop=%.8lld, idx=%.8lld)\t", l, i);
      print_data((MAP_TYPE_T*)tmp_addr, 1);
      tmp_addr++;
      printf("\n");
    }
    printf("\n");
  }
  return;
}

void do_read_write_read(void)
{
  int dsize = sizeof(MAP_TYPE_T);

  printf("\n\t\t\tREAD - WRITE - READ\n");
   
  MAP_TYPE_T val = 0;
  MAP_TYPE_T* pval = (MAP_TYPE_T*)&val;

  //initializes random number generator
  srand((unsigned) time(0));

  for (long long unsigned int l=0; l<loop_count; l++) {

    tmp_addr = base_addr;
    val = start_val - 1;

    for (long long unsigned int i=0; i<map_len/dsize; i++) {

      if (naddr_count > 0 && i >= naddr_count)
	continue;

      printf("\t(Loop=%.8lld, idx=%.8lld)\t", l, i);
      printf("%p: ", tmp_addr);
      // read
      print_data((MAP_TYPE_T*) tmp_addr, 0);

      // write
      if (incr == 0) 
	val = (MAP_TYPE_T) (rand());
      else
	val++;
      
      *tmp_addr = *pval;

      if (*pval != *tmp_addr)
	diff_count++;
    
      printf(", new_val=");
      print_data(pval, 0);

      // read
      printf(", ");
      print_data((MAP_TYPE_T*) tmp_addr, 0); 

      tmp_addr++;
      data_count++;
      printf("\n");
    }
    print_stats();
  }
  
  return;
}

void print_data(MAP_TYPE_T* addr, int disp_addr)
{
  if (disp_addr == 0) {
   printf(FORMAT1, *addr);
  } else {
   printf(FORMAT2, addr, *addr);
  }

  return;
}

void print_help(void)
{
  puts("\nmmapttool - mmap test toot\n");
  puts("\nDescription: map a given file node on virtual memory space and\n");
  puts("             perfom read and/or write accesses on that memory region\n");
  puts("\nUsage: mmapttool [OPTIONS] FILENAME\n");
  puts("Options:\n");
  puts("-h\t\t display this help message\n");
  puts("-i\t\t for writing incremented values starting the given start value or 0 if not specified. Default is random\n");
  puts("-l\t\t number of interations to be performed\n");
  puts("-n\t\t number of address location to be accessed starting from the base address. Default is all address locations\n");
  puts("-r\t\t perform read only opterions\n");
  puts("-s\tSTART\t start value used when increment option is selected\n");
  puts("-x\t\t perform read write read operations\n");
  puts("-v\t\t verbose\n");
  puts("-w\t\t perform write only operations\n");

  return;
}

void print_stats(void)
{
  printf("\tdata_count = %lli, diff_count = %lli\n", data_count, diff_count);
  return;
}

void print_options(void)
{
  printf("\noptions status:\n");
  printf("\tincr=%s, ", yes_or_no(incr));
  printf("read=%s, ", yes_or_no(rd_only));
  printf("write=%s, ", yes_or_no(wr_only));
  printf("rd_wr=%s\n", yes_or_no(rd_wr));

  printf("arguments status:\n");
  printf("\tfilename=%s, ", fname);
  printf("start_val=%lli, ", start_val);
  printf("loop_count=%lli, ", loop_count);
  printf("naddr_count=%lli\n", naddr_count);
  return;
}
