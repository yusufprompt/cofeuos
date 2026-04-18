/*
 * ============================================================================
 * IO.C - Giriş/Çıkış İşlemleri Uygulamaları
 * ============================================================================
 */

#include "../include/io.h"

/* Donanım I/O port işlemleri */

void outb(u16 port, u8 data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

void outw(u16 port, u16 data) {
    __asm__ volatile("outw %0, %1" : : "a"(data), "Nd"(port));
}

void outl(u16 port, u32 data) {
    __asm__ volatile("outl %0, %1" : : "a"(data), "Nd"(port));
}

u8 inb(u16 port) {
    u8 res;
    __asm__ volatile("inb %1, %0" : "=a"(res) : "Nd"(port));
    return res;
}

u16 inw(u16 port) {
    u16 res;
    __asm__ volatile("inw %1, %0" : "=a"(res) : "Nd"(port));
    return res;
}

u32 inl(u16 port) {
    u32 res;
    __asm__ volatile("inl %1, %0" : "=a"(res) : "Nd"(port));
    return res;
}

/* Gecikme fonksiyonları */
void io_wait(void) {
    /* Boş bir port'a yazarak CPU'yu yavaşlat */
    outb(0x80, 0);
}

/* Basit yazdırma fonksiyonları */
void putchar(char c) {
    /* Basit bir karakter yazdırma fonksiyonu */
    /* Not: Gerçek bir sistemde bu fonksiyon ekran belleğine yazar */
}

void puts(const char* s) {
    while (*s) {
        putchar(*s++);
    }
}

void printf(const char* format, ...) {
    /* Basit bir printf fonksiyonu */
    /* Not: Gerçek bir sistemde bu fonksiyon format string'ini işler */
    puts(format);
}
