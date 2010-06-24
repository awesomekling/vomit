#ifndef __vga_h__
#define __vga_h__

void vga_init();
void vga_kill();

void mark_palette_dirty();
void clear_palette_dirty();
bool is_palette_dirty();

BYTE vga_read_register(BYTE index);
BYTE vga_read_register2(BYTE index);
BYTE vga_read_sequencer(BYTE index);
void vga_write_register(BYTE index, BYTE value);

typedef struct {
    BYTE r;
    BYTE g;
    BYTE b;
} rgb_t;

extern rgb_t vga_color_register[];
extern BYTE vga_palette_register[];

#endif /* __vga_h__ */
