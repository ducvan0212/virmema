#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "vlib.h"

#define FRAME_SIZE 256
#define PAGE_SIZE 256
#define PHY_SIZE 256
#define PAGE_TABLE_SIZE 256

// record of page table
typedef struct page_record
{
  int dirty;
  int page;
  int frame;
} Record;

// record of page table
typedef struct v_frame
{
  int dirty;
  char content[FRAME_SIZE];
} Frame;

char *get_line(FILE* fp)
{
	int len = 0, got = 0, c;
	char *buf = 0;
  
	while ((c = fgetc(fp)) != EOF) {
		if (got + 1 >= len) {
			len *= 2;
			if (len < 4) len = 4;
			buf = realloc(buf, len);
		}
		if (c == '\n') break;
    if (c - '0' < 0 || c - '0' > 9) {
      printf("Only numbers are allowed!\n");
      return 0;
    }
		buf[got++] = c;
	}
	if (c == EOF && !got) return 0;
 
	buf[got++] = '\0';
	return buf;
}

int search_in_page_table(Record *page_table, int size, int page) {
  int i;
  for (i=0; i<size; i++) {
    printf("Page table #%-3d: page %-15d frame %-15d dirty %d\n", i, page_table[i].page, page_table[i].frame, page_table[i].dirty);
    if (page_table[i].dirty == 1 && page_table[i].page == page) {
      printf("Found!\n");
      return page_table[i].frame;
    }
  }
  return -1;
}

int get_available_frame(Frame *physical_memory, int size) {
  int i;
  for (i=0; i<size; i++) {
    if (physical_memory[i].dirty == 0) {
      return i;
    }
  }
  return -1;
}

int copy_to_physical_memory(Frame *physical_memory, int size, char *buffer) {
  int i = get_available_frame(physical_memory, size);
  if (i >= 0) {
    memcpy(physical_memory[i].content, buffer, FRAME_SIZE);
    physical_memory[i].dirty = 1;
    return i;
  }
  return -1;
}

int get_available_record(Record *page_table, int size) {
  int i;
  for (i=0; i<size; i++) {
    if (page_table[i].dirty == 0) {
      return i;
    }
  }
  return -1;
}

void update_page_table(Record *page_table, int size, int page_number, int frame) {
  int a = get_available_record(page_table, size);
  if (a >= 0) {
    printf("Update in page table: entry #%d, page %d, frame %d\n", a, page_number, frame);
    page_table[a].page = page_number;
    page_table[a].frame = frame;
    page_table[a].dirty = 1;
  } else {
    printf("No available record in page table found!\n");
  }
}

void print_frame(Frame frame) {
  int i;
  for (i=0; i<256; i++) {
    printf("%c", frame.content[i]);
  }
}

int main(void)
{
  FILE *fbs;
  int logical_address, page_number, page_offset;
  // page table has 2^8=256 entries
  Record page_table[PAGE_TABLE_SIZE];
  // physical memory has 256*256 bytes
  Frame physical_memory[PHY_SIZE];
  int i = 0, a, frame;
  char *buffer;
  
  // init page table
  for (i=0; i<PAGE_TABLE_SIZE; i++) {
    page_table[i].dirty = 0;
  }
  
  // init physical memory
  for (i=0; i<PHY_SIZE; i++) {
    physical_memory[i].dirty = 0;
  }
  
  fbs = fopen("BACKING_STORE.bin", "rb"); // open backing store
  if (fbs == NULL) {
    printf("File can not opened, errno = %d\n", errno);
    return 1;
  }
  
  fseek (fbs , 0 , SEEK_END);
  int lSize = ftell (fbs);
  rewind (fbs);
  // printf("%d", lSize);
  buffer = (char*) malloc (sizeof(char)*(FRAME_SIZE));
  
  char *s;
	while ((s = get_line(stdin))) {
    logical_address = atoi(s);
    if (logical_address >= 65535) {
      printf("Virtual address space is 2^16 = 65536. Out of bound!\n");
      free(s);
      continue;
    }
    // extract page offset by get 8 lsb from logical address.
    page_offset = logical_address & 0xff;
    // extract page number by shift 8 bits to the right. 8 msb are remaining.
    page_number = logical_address >> 8;
    printf("Page number: %-10d\t\tPage offset: %d\n", page_number, page_offset);
    
    // page is not in page table
    if (search_in_page_table(page_table, PAGE_TABLE_SIZE, page_number) == -1) {
      // move indicator to the page in backing store
      fseek(fbs, sizeof(char)*page_number*(PAGE_SIZE), SEEK_SET);
      // read a page
      a=fread(buffer, sizeof(char)*(PAGE_SIZE), 1, fbs);
  		printf("Read from backing store\n");
      print_frame(physical_memory[frame]);
      printf("\n");
      // fetch to physical memory
      frame = copy_to_physical_memory(physical_memory, PHY_SIZE, buffer);
      printf("Write to physical memory #%d\n", frame);
      // update page table
      update_page_table(page_table, PAGE_TABLE_SIZE, page_number, frame);
      // TODO: update TLB
    }
    
    free(s);
    printf("\n");
	}
  
  free(buffer);
  free(s);
  fclose(fbs); /* close history file */
	return 0;
}
