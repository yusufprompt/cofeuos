; GDT (Global Descriptor Table) Ayarları
gdt_start:

gdt_null: ; Zorunlu boş başlangıç segmenti
    dd 0x0
    dd 0x0

gdt_code: ; Kod Segmenti (Code Segment)
    ; base=0x0, limit=0xfffff
    ; 1st flags: (present)1 (privilege)00 (type)1 -> 1001b
    ; type flags: (code)1 (conforming)0 (readable)1 (accessed)0 -> 1010b
    ; 2nd flags: (granularity)1 (32-bit default)1 (64-bit seg)0 (AVL)0 -> 1100b
    dw 0xffff    ; Limit (bits 0-15)
    dw 0x0       ; Base (bits 0-15)
    db 0x0       ; Base (bits 16-23)
    db 10011010b ; 1st flags, type flags
    db 11001111b ; 2nd flags, Limit (bits 16-19)
    db 0x0       ; Base (bits 24-31)

gdt_data: ; Veri Segmenti (Data Segment)
    ; Veri segmenti kod segmentiyle aynıdır, sadece 'type flags' farklıdır.
    ; type flags: (code)0 (expand down)0 (writable)1 (accessed)0 -> 0010b
    dw 0xffff    ; Limit (bits 0-15)
    dw 0x0       ; Base (bits 0-15)
    db 0x0       ; Base (bits 16-23)
    db 10010010b ; 1st flags, type flags
    db 11001111b ; 2nd flags, Limit (bits 16-19)
    db 0x0       ; Base (bits 24-31)

gdt_end:

; GDT Tanımlayıcısı (Descriptor)
gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; GDT Boyutu
    dd gdt_start               ; GDT Başlangıç Adresi

; Segment sabitlerini tanımlayalım (Offset değerleri)
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start
