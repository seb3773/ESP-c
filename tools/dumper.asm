; dumper.asm - Dump unpacked DOS program memory on opening *TEST*
org 0x100
section .text

start:
    ; Shrink our memory allocation first (AH=4Ah)
    push cs
    pop es
    mov bx, 0x0100 ; 4KB (256 paragraphs)
    mov ah, 0x4a
    int 0x21

    ; Print banner
    mov dx, msg_start
    mov ah, 0x09
    int 0x21

    ; Save old INT 21h vector
    mov ax, 0x3521
    int 0x21
    mov [old_int21_off], bx
    mov [old_int21_seg], es

    ; Hook INT 21h
    mov ax, 0x2521
    mov dx, int21_handler
    int 0x21

    ; Execute target program via INT 21h AH=4B00h
    ; Prepare EXEC parameter block
    push cs
    pop ds
    mov word [param_env], 0
    mov word [param_cmd_off], cmd_line
    mov word [param_cmd_seg], cs
    mov word [param_fcb1_off], fcb1
    mov word [param_fcb1_seg], cs
    mov word [param_fcb2_off], fcb2
    mov word [param_fcb2_seg], cs

    ; Exec ESP.EXE
    mov dx, prog_name
    mov bx, exec_params
    mov ax, 0x4b00
    int 0x21
    jnc exec_ok

    ; Exec failed, print error
    push cs
    pop ds
    mov dx, msg_exec_fail
    mov ah, 0x09
    int 0x21
    mov ax, 0x4c01
    int 0x21

exec_ok:
    mov dx, msg_done
    mov ah, 0x09
    int 0x21

    mov ax, 0x4c00
    int 0x21

int21_handler:
    ; Check if dumped already
    cmp byte [cs:dumped], 1
    je pass_through

    push bp
    mov bp, sp
    ; Stack: bp (0), ip_ret (2), cs_ret (4), flags (6)

    ; Only trigger on AH=3Dh (Open file) or AH=3Ch (Create file)
    cmp ah, 0x3d
    je check_name
    cmp ah, 0x3c
    je check_name
    pop bp
    jmp pass_through

check_name:
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

    ; WE FOUND THE TARGET CALL FROM ESP.EXE!
    mov byte [cs:dumped], 1

    mov ax, [bp+2]
    mov [cs:caller_ip], ax
    mov ax, [bp+4]
    mov [cs:caller_cs], ax
    mov ax, ds
    mov [cs:caller_ds], ax

    pop bp

    ; Unhook INT 21h immediately
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

    ; Now write dump files using restored INT 21h!
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
    jc fail_dump
    mov bx, ax ; handle

    ; Write 64KB from caller CS:0000
    mov ds, [cs:caller_cs]
    xor dx, dx
    mov cx, 0xFFFF
    mov ah, 0x40
    int 0x21

    ; Close file
    mov ah, 0x3e
    int 0x21

    ; Create DUMP_DS.BIN
    push cs
    pop ds
    mov dx, file_ds
    mov cx, 0
    mov ah, 0x3c
    int 0x21
    jc fail_dump
    mov bx, ax

    ; Write 64KB from caller DS:0000
    mov ds, [cs:caller_ds]
    xor dx, dx
    mov cx, 0xFFFF
    mov ah, 0x40
    int 0x21

    ; Close file
    mov ah, 0x3e
    int 0x21

    push cs
    pop ds
    mov dx, msg_success
    mov ah, 0x09
    int 0x21

    ; Done, exit
    mov ax, 0x4c00
    int 0x21

fail_dump:
    push cs
    pop ds
    mov dx, msg_fail
    mov ah, 0x09
    int 0x21
    mov ax, 0x4c01
    int 0x21

pass_through:
    ; Jump to original INT 21h
    jmp far [cs:old_int21_off]

dumped db 0
caller_ip dw 0
caller_cs dw 0
caller_ds dw 0
old_int21_off dw 0
old_int21_seg dw 0

prog_name db "ESP.EXE", 0
cmd_line db 18, " TEST.ESP TEST.DAT", 0x0d
fcb1 times 16 db 0
fcb2 times 16 db 0

exec_params:
param_env dw 0
param_cmd_off dw 0
param_cmd_seg dw 0
param_fcb1_off dw 0
param_fcb1_seg dw 0
param_fcb2_off dw 0
param_fcb2_seg dw 0

file_cs db "DUMP_CS.BIN", 0
file_ds db "DUMP_DS.BIN", 0
msg_start db "Starting Dumper...", 0x0d, 0x0a, "$"
msg_success db 0x0d, 0x0a, "=== MEMORY DUMPED SUCCESSFULLY! ===", 0x0d, 0x0a, "$"
msg_fail db "Dump failed!", 0x0d, 0x0a, "$"
msg_exec_fail db "EXEC failed!", 0x0d, 0x0a, "$"
msg_done db "Done.", 0x0d, 0x0a, "$"
