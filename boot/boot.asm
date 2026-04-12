[org 0x7c00]
KERNEL_OFFSET equ 0x1000

mov [BOOT_DRIVE], dl

; Stack Ayarla
mov bp, 0x9000
mov sp, bp

; --- HATA AYIKLAMA 1: Başladık ---
mov ah, 0x0e
mov al, 'B' ; 'B'ooting...
int 0x10

; Kernel'ı yükle
call load_kernel

; --- HATA AYIKLAMA 2: Kernel Yüklendi ---
mov al, 'K' ; 'K'ernel Loaded...
int 0x10

; Grafik Moduna Geç
mov ah, 0x00
mov al, 0x13
int 0x10

call switch_to_pm
jmp $

; ... (load_kernel, switch_to_pm ve GDT kısımları bir önceki mesajdakiyle aynı kalsın) ...


[bits 16]
load_kernel:
    mov bx, KERNEL_OFFSET ; Hedef adres: 0x1000
    mov ah, 0x02          ; BIOS "Sektör Oku" fonksiyonu
    mov al, 50           ; 64 çok büyük olabilir, şimdilik 30 sektör oku (15KB)
    mov ch, 0x00          ; Silindir 0
    mov dh, 0x00          ; Kafa (Head) 0
    mov cl, 0x02          ; 2. sektörden başla
    mov dl, [BOOT_DRIVE]  ; Boot sürücüsü (BIOS'un verdiği DL)
    
    int 0x13              ; BIOS Kesmesi
    jc disk_error         ; Eğer hata varsa (Carry Flag) 'E' bas

    ret

disk_error:
    mov ah, 0x0e
    mov al, 'E'           ; Hata durumunda ekranda 'BE' göreceksin
    int 0x10
    jmp $

[bits 16]
switch_to_pm:
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:init_pm

[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebp, 0x90000
    mov esp, ebp
    call BEGIN_PM

[bits 32]
BEGIN_PM:
    call KERNEL_OFFSET
    jmp $

; --- GDT (Global Descriptor Table) ---
gdt_start:
gdt_null: 
    dd 0x0, 0x0
gdt_code: 
    dw 0xffff, 0x0
    db 0x0, 10011010b, 11001111b, 0x0
gdt_data: 
    dw 0xffff, 0x0
    db 0x0, 10010010b, 11001111b, 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

BOOT_DRIVE db 0
times 510-($-$$) db 0
dw 0xaa55
