#include "../../include/common.h"
#include <cpu/decode.h>

InstBuffer inst_buf;

void record_inst(InstEntry *s) {
    inst_buf.current = (inst_buf.current + 1) % IRINGBUF_SIZE;
    inst_buf.buf[inst_buf.current] = *s;
}

void recent_inst_display() {
    printf("Recently executed instructions:\n");
    for (int i = 0; i < IRINGBUF_SIZE; i++) {
        char *prefix = i == inst_buf.current ? "=> " : "   ";
        printf("%s0x%08x: 0x%08x\n", prefix, inst_buf.buf[i].addr, inst_buf.buf[i].inst);
    }
}

void init_iringbuf() {
    inst_buf.current = -1;
    for (int i = 0; i < IRINGBUF_SIZE; i++) {
        inst_buf.buf[i].addr = 0;
        inst_buf.buf[i].inst = 0;
    }
}

void display_mem_read(paddr_t addr, int len) {
    printf("reading at " FMT_PADDR " len=%d\n", addr, len);
}

void display_mem_write(paddr_t addr, int len, word_t data) {
    printf("writing at " FMT_PADDR " len=%d, data=" FMT_WORD "\n", addr, len, data);
}