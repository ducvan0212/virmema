#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#define FRAME_SIZE 256
#define PAGE_SIZE 256
#define PHY_SIZE 256
#define PAGE_TABLE_SIZE 256
#define TLB_SIZE 16

// record of page table
typedef struct page_record
{
  int dirty;
  // index can be used to infer the page number instead of a explicitly declared page field. Added for clarity purpose.
  int page;
  int frame;
} Record;

// record of page table
typedef struct v_frame
{
  int dirty;
  char content[FRAME_SIZE];
} Frame;

typedef struct v_tlb_record
{
  int dirty;
  int page;
  int frame;
} TLB_Record;

// circular array structure
typedef struct v_tlb
{
  TLB_Record records[TLB_SIZE];
  int tail;
} TLB;

char *get_line(FILE* fp);

int search_in_page_table(Record *page_table, int size, int page);

int get_available_frame(Frame *physical_memory, int size);

int copy_to_physical_memory(Frame *physical_memory, int size, char *buffer);

int get_available_record(Record *page_table, int size);

void update_page_table(Record *page_table, int size, int page_number, int frame);

void print_page_table(Record *page_table, int size);

void print_frame(Frame frame);

void print_tlb(TLB tlb);

void update_tlb(TLB *tlb, int page, int frame);

int search_in_tlb(TLB tlb, int page);

int translate_to_physical_address(int frame, int offset);