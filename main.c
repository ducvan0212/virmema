#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "vlib.h"


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
	while ((s = get_line(fa))) {
    printf("------------------------\n");
    translation_counter++;
    logical_address = atoi(s);
    if (logical_address > 65535) {
      printf("Virtual address space is 2^16 = 65536. You entered %d. So, out of bound!\n", logical_address);
      free(s);
      continue;
    }
    
    printf("Logical address: %d\n", logical_address);
    
    // extract page offset by get 8 lsb from logical address.
    page_offset = logical_address & 0xff;
    // extract page number by shift 8 bits to the right. 8 msb are remaining.
    page_number = logical_address >> 8;
    printf("\tPage number: %-10d\n\tPage offset: %d\n", page_number, page_offset);
    
    corresponding_frame = search_in_tlb(tlb, page_number);
    if (corresponding_frame >= 0) {
      printf("\tTLB hit.\n");
      tlb_hit_counter++;
    } else {
      printf("\tTLB fault.\n");
      corresponding_frame = search_in_page_table(page_table, PAGE_TABLE_SIZE, page_number);
      // PAGE TABLE HIT
      if (corresponding_frame >= 0) {
        printf("\tPage table hit.\n");
        update_tlb(&tlb, page_number, corresponding_frame);
      } 
      // PAGE FAULT. page is not in page table
      else {
        printf("\tPage table fault.\n");
        page_fault_counter++;
        // move indicator to the page in backing store
        fseek(fbs, sizeof(char)*page_number*(PAGE_SIZE), SEEK_SET);
        // read a page
        a=fread(buffer, sizeof(char)*(PAGE_SIZE), 1, fbs);
    		// fetch to physical memory
        corresponding_frame = copy_to_physical_memory(physical_memory, PHY_SIZE, buffer);
        // printf("Write to physical memory #%d\n", corresponding_frame);
        // print_frame(physical_memory[corresponding_frame]);
        // printf("\n");
        // update page table
        update_page_table(page_table, PAGE_TABLE_SIZE, page_number, corresponding_frame);
        update_tlb(&tlb, page_number, corresponding_frame);
      }
    }
      
    // print_page_table(page_table, PAGE_TABLE_SIZE);
    // print_tlb(tlb);
    physical_address = translate_to_physical_address(corresponding_frame, page_offset);
    printf("Physical address: %-15d\nValue: %c\n", physical_address, physical_memory[corresponding_frame].content[page_offset]);
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
