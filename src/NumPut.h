#include <stdio.h>
#include <string.h>

// Function to store a number in binary format at the specified address + offset
void NumPut(void* address, int offset, int value) {
  // Calculate the address to store the value
  char* target = (char*)address + offset;

  // Store the value in binary format (little-endian)
  target[0] = (value >> 0) & 0xFF;
  target[1] = (value >> 8) & 0xFF;
  target[2] = (value >> 16) & 0xFF;
  target[3] = (value >> 24) & 0xFF;
}
