[bits 32]
[extern kernel_main] ; kernel.c içindeki fonksiyonun adı
call kernel_main     ; C koduna git
jmp $
