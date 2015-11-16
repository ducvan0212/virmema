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
    if (page_table[i].dirty == 1 && page_table[i].page == page) {
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

void print_page_table(Record *page_table, int size) {
  int i;
  for (i=0; i<size; i++) {
    if (page_table[i].dirty == 0) {
      return;
    }
    printf("Page table #%-3d: page %-15d frame %-15d dirty %d\n", i, page_table[i].page, page_table[i].frame, page_table[i].dirty);
  }
}

void print_frame(Frame frame) {
  int i;
  for (i=0; i<256; i++) {
    printf("%c", frame.content[i]);
  }
}

void print_tlb(TLB tlb) {
  int i;
  
  printf("TLB tail: %d\n", tlb.tail);
  for (i=0; i<TLB_SIZE; i++) {
    printf("Page: %-10dFrame: %-10dDirty: %d\n", tlb.records[i].page, tlb.records[i].frame, tlb.records[i].dirty);
  }
}

void update_tlb(TLB *tlb, int page, int frame) {
  tlb->tail = (tlb->tail + 1) % TLB_SIZE;
  tlb->records[tlb->tail].page = page;
  tlb->records[tlb->tail].frame = frame;
  tlb->records[tlb->tail].dirty = 1;
  printf("Update in TLB#%d: page %d, frame %d\n", tlb->tail, tlb->records[tlb->tail].page, tlb->records[tlb->tail].frame);
}

int search_in_tlb(TLB tlb, int page) {
  int i;
  
  for (i=0; i<TLB_SIZE; i++) {
    if (tlb.records[i].dirty == 1) {
      if (tlb.records[i].page == page) {
        return tlb.records[i].frame;
      }
    }
  }
  return -1;
}

int translate_to_physical_address(int frame, int offset) {
  int address = frame;
  // printf("%d\n", address);
  address = address << 8;
  // printf("%d\n", address);
  address = address | offset;
  // printf("%d\n", address);
  return address;
}

int main(void)
{
  FILE *fbs, *fa;
  int logical_address, physical_address, page_number, page_offset;
  // page table has 2^8=256 entries
  Record page_table[PAGE_TABLE_SIZE];
  // physical memory has 256*256 bytes
  Frame physical_memory[PHY_SIZE];
  int i = 0, a, corresponding_frame;
  int page_fault_counter = 0, translation_counter = 0, tlb_hit_counter = 0;
  char *buffer;
  TLB tlb;
  
  // init page table
  for (i=0; i<PAGE_TABLE_SIZE; i++) {
    page_table[i].dirty = 0;
  }
  
  // init physical memory
  for (i=0; i<PHY_SIZE; i++) {
    physical_memory[i].dirty = 0;
  }
  
  // init TLB
  tlb.tail = 0;
  for (i=0; i<TLB_SIZE; i++) {
    tlb.records[i].dirty = 0;
  }
  
  fbs = fopen("BACKING_STORE.bin", "rb"); // open backing store
  if (fbs == NULL) {
    printf("File can not opened, errno = %d\n", errno);
    return 1;
  }
  
  fa = fopen("addresses.txt", "rb"); // open backing store
  if (fa == NULL) {
    printf("File can not opened, errno = %d\n", errno);
    return 1;
  }
  
  buffer = (char*) malloc (sizeof(char)*(FRAME_SIZE));
  
  char *s;
	while ((s = get_line(stdin))) {
    translation_counter++;
    logical_address = atoi(s);
    if (logical_address > 65535) {
      printf("Virtual address space is 2^16 = 65536. You entered %d. So, out of bound!\n", logical_address);
      free(s);
      continue;
    }
    // extract page offset by get 8 lsb from logical address.
    page_offset = logical_address & 0xff;
    // extract page number by shift 8 bits to the right. 8 msb are remaining.
    page_number = logical_address >> 8;
    printf("Page number: %-10d\t\tPage offset: %d\n", page_number, page_offset);
    
    corresponding_frame = search_in_tlb(tlb, page_number);
    if (corresponding_frame >= 0) {
      printf("TLB hit.\n");
      tlb_hit_counter++;
    } else {
      printf("TLB fault.\n");
      corresponding_frame = search_in_page_table(page_table, PAGE_TABLE_SIZE, page_number);
      // PAGE TABLE HIT
      if (corresponding_frame >= 0) {
        printf("Page table hit.\n");
        update_tlb(&tlb, page_number, corresponding_frame);
      } 
      // PAGE FAULT. page is not in page table
      else {
        printf("Page table fault.\n");
        page_fault_counter++;
        // move indicator to the page in backing store
        fseek(fbs, sizeof(char)*page_number*(PAGE_SIZE), SEEK_SET);
        // read a page
        a=fread(buffer, sizeof(char)*(PAGE_SIZE), 1, fbs);
    		// fetch to physical memory
        corresponding_frame = copy_to_physical_memory(physical_memory, PHY_SIZE, buffer);
        printf("Write to physical memory #%d\n", corresponding_frame);
    		print_frame(physical_memory[corresponding_frame]);
        printf("\n");
        // update page table
        update_page_table(page_table, PAGE_TABLE_SIZE, page_number, corresponding_frame);
        update_tlb(&tlb, page_number, corresponding_frame);
      }
    }
      
    print_page_table(page_table, PAGE_TABLE_SIZE);
    print_tlb(tlb);
    physical_address = translate_to_physical_address(corresponding_frame, page_offset);
    printf("Physical address: %-15dValue: %c\n", physical_address, physical_memory[corresponding_frame].content[page_offset]);
    free(s);
    printf("\n");
	}
  
  printf("Translation: %d\n", translation_counter);
  printf("Page fault: %d\n", page_fault_counter);
  printf("Page fault rate: %f\n", (float) page_fault_counter/translation_counter);
  printf("TLB hit: %d\n", tlb_hit_counter);
  printf("TLB hit rate: %f\n", (float) tlb_hit_counter/translation_counter);
  
  free(buffer);
  free(s);
  fclose(fbs); /* close history file */
	return 0;
}
