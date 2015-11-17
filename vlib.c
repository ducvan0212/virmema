#include "vlib.h"

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
  tlb->records[tlb->tail].page = page;
  tlb->records[tlb->tail].frame = frame;
  tlb->records[tlb->tail].dirty = 1;
  tlb->tail = (tlb->tail + 1) % TLB_SIZE;
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
