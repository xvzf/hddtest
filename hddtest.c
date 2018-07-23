#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#define LOG_INTERVAL 2048 // Log every 2048 blocks
#define SECTOR_SIZE 4096 // Sector size in byte
#define BLOCK_SIZE SECTOR_SIZE * 128 // Block size

void get_random(uint8_t* buf, int size);
void print_status(char* status, double progress);

/* Helper for printing the current status */
void print_status(char* status, double progress) {
    printf("\r[+] %s: %.2lf%%", status, progress * 100);
}

/* Generates random output */
void get_random(uint8_t* buf, int size) {
    FILE *fp;
    fp = fopen("/dev/urandom", "r");
    fread(buf, 1, size, fp);
    fclose(fp);
}

int main(int argc, char** argv) {
    if(argc < 2) {
        printf("[!] Missing parameter\n  -> Usage: hddtest /dev/sdX\n");
        return 1;
    }
    char* device = argv[1];
    fputs("\033[?25l", stdout); /* hide the cursor */

    printf("[!] Warning: This utility overwrites all existing data on %s!?\nPress any key to continue", device);
    getchar();

    // Direct I/O access
    int fd = open(device, O_RDWR | O_DIRECT);
    uint64_t size = lseek(fd, 0, SEEK_END);

    // Align a block of memory
    uint8_t* testdata = aligned_alloc(SECTOR_SIZE, BLOCK_SIZE);
    // Same for the read buffer
    uint8_t* read_buffer = aligned_alloc(SECTOR_SIZE, BLOCK_SIZE);

    // Write all sectors
    for(uint64_t current = 0; current < (size - SECTOR_SIZE);) {

        // Write
        if(BLOCK_SIZE != pwrite(fd, testdata, BLOCK_SIZE, current)) {
            printf("\n[!] Could not finish the test, write error at %ld\n", current);
            break;
        }

        // Update the user from time to time
        if((current % BLOCK_SIZE) % LOG_INTERVAL == 0) {
            print_status("Writing testdata to the disk", (double) current / (double) size);
        }

        if(size - current - BLOCK_SIZE >= 0) {
            current += BLOCK_SIZE;
        } else {
            current += SECTOR_SIZE;
        }
    }

    printf("\n");

    // Read and verify all sectors
    uint64_t sector_missmatch_count = 0;
    for(uint64_t current = 0; current < (size - SECTOR_SIZE);) {

        // Read
        if(BLOCK_SIZE != pread(fd, read_buffer, BLOCK_SIZE, current)) {
            printf("\n[!] Could not finish the test, read error at %ld\n", current);
            break;
        }

        // Verify
        for(int i = 0; i < BLOCK_SIZE / SECTOR_SIZE; i++) {
            if(memcmp(testdata + i * SECTOR_SIZE, read_buffer + i * SECTOR_SIZE, SECTOR_SIZE)) {
                printf("[!] Bad sector at %ld\n", current);
                sector_missmatch_count++;
            }
        }

        // Update the user from time to time
        if((current % BLOCK_SIZE) % LOG_INTERVAL == 0) {
            print_status("Testing harddrive integrity", (double) current / (double) size);
        }
        
        if(size - current - BLOCK_SIZE >= 0) {
            // Go for large chunks as long as they are available
            current += BLOCK_SIZE;
        } else {
            // Go by the actual sector size if we are approaching the end of the disk
            current += SECTOR_SIZE;
        }
    }

    printf("\n");
    sector_missmatch_count > 0 ? printf("[!]") : printf("[+]");
    printf(" Final result: %ld possibly bad sectors\n", sector_missmatch_count);
    fputs("\033[?25h", stdout); /* show the cursor */

    close(fd);
    free(testdata);
    free(read_buffer);
    return 0;
}
