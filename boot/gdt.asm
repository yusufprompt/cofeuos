; GDT tanımları kernel_entry.asm'de yapıldığı için bu dosya şu anda kullanılmıyor
; Ama ayakta tutuyoruz uyumluluğu için

; Global Descriptor Table (GDT)
; 0x00: Null descriptor
; 0x08: Code segment selector (kernel mode)
; 0x10: Data segment selector (kernel mode)
