; tsr_dump.asm - TSR that dumps memory when target file is opened
org 0x100
section .text

start:
    jmp init

int21_handler:
    cmp byte [cs:dumped], 1
    je pass_through

    push bp
    mov bp, sp
    ; stack: bp(0), ip(2), cs(4), flags(6)

    cmp ah, 0x3d
    je check_file
    cmp ah, 0x3c
    je check_file
    pop bp
    jmp pass_through

check_file:
    push si
    push cx
    push ax
    mov si, dx
    mov cx, 64
search_loop:
    mov al, [ds:si]
    test al, al
    jz search_done
    cmp al, 'T'
    je check_est
    cmp al, 't'
    je check_est
    inc si
    loop search_loop

search_done:
    pop ax
    pop cx
    pop si
    pop bp
    jmp pass_through

check_est:
    mov al, [ds:si+1]
    and al, 0xDF
    cmp al, 'E'
    jne not_test
    mov al, [ds:si+2]
    and al, 0xDF
    cmp al, 'S'
    jne not_test
    mov al, [ds:si+3]
    and al, 0xDF
    cmp al, 'T'
    je is_target
not_test:
    inc si
    loop search_loop
    jmp search_done

is_target:
    pop ax
    pop cx
    pop si

    mov byte [cs:dumped], 1

    mov ax, [bp+2]
    mov [cs:caller_ip], ax
    mov ax, [bp+4]
    mov [cs:caller_cs], ax
    mov ax, ds
    mov [cs:caller_ds], ax

    pop bp

    ; Unhook INT 21h
    push ds
    push dx
    push ax
    mov ax, 0x2521
    mov dx, [cs:old_int21_off]
    mov ds, [cs:old_int21_seg]
    int 0x21
    pop ax
    pop dx
    pop ds

    ; Dump CS and DS
    pusha
    push ds
    push es

    ; Create DUMP_CS.BIN
    push cs
    pop ds
    mov dx, file_cs
    mov cx, 0
    mov ah, 0x3c
    int 0x21
    jc dump_err
    mov bx, ax

    mov ds, [cs:caller_cs]
    xor dx, dx
    mov cx, 0xFFFF
    mov ah, 0x40
    int 0x21

    mov ah, 0x3e
    int 0x21

    ; Create DUMP_DS.BIN
    push cs
    pop ds
    mov dx, file_ds
    mov cx, 0
    mov ah, 0x3c
    int 0x21
    jc dump_err
    mov bx, ax

    mov ds, [cs:caller_ds]
    xor dx, dx
    mov cx, 0xFFFF
    mov ah, 0x40
    int 0x21

    mov ah, 0x3e
    int 0x21

    push cs
    pop ds
    mov dx, msg_success
    mov ah, 0x09
    int 0x21

    ; Terminate process completely
    mov ax, 0x4c00
    int 0x21

dump_err:
    push cs
    pop ds
    mov dx, msg_fail
    mov ah, 0x09
    int 0x21
    mov ax, 0x4c01
    int 0x21

pass_through:
    jmp far [cs:old_int21_off]

dumped db 0
caller_ip dw 0
caller_cs dw 0
caller_ds dw 0
old_int21_off dw 0
old_int21_seg dw 0

file_cs db "DUMP_CS.BIN", 0
file_ds db "DUMP_DS.BIN", 0
msg_success db 0x0d, 0x0a, "=== MEMORY DUMPED SUCCESSFULLY! ===", 0x0d, 0x0a, "$"
msg_fail db 0x0d, 0x0a, "Dump failed!", 0x0d, 0x0a, "$"
msg_installed db "TSR dumper installed.", 0x0d, 0x0a, "$"

init:
    ; Print installed banner
    mov dx, msg_installed
    mov ah, 0x09
    int 0x21

    ; Save old vector
    mov ax, 0x3521
    int 0x21
    mov [old_int21_off], bx
    mov [old_int21_seg], es

    ; Hook INT 21h
    mov ax, 0x2521
    mov dx, int21_handler
    int 0x21

    ; Stay resident
    mov dx, (init - start + 0x100 + 15) / 16
    mov ax, 0x3100
    int 0x21
