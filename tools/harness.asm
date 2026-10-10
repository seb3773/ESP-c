; harness.asm - Standalone runner for esp_engine.bin
org 0x100
section .text

start:
    ; Shrink memory for COM program
    push cs
    pop es
    mov bx, 0x1000 ; 64KB
    mov ah, 0x4a
    int 0x21

    ; Open IN.DAT
    mov dx, file_in
    mov ax, 0x3d00
    int 0x21
    jc fail_open_in
    mov [in_handle], ax

    ; Create OUT.BIN
    mov dx, file_out
    mov cx, 0
    mov ah, 0x3c
    int 0x21
    jc fail_open_out
    mov [out_handle], ax

    ; Open ENGINE.BIN
    mov dx, file_engine
    mov ax, 0x3d00
    int 0x21
    jc fail_engine
    mov [engine_h], ax

    ; Allocate memory for engine code (0x10f paragraphs = ~4.3KB)
    mov bx, 0x0110
    mov ah, 0x48
    int 0x21
    jc fail_mem
    mov [engine_seg], ax

    ; Read engine
    mov ds, ax
    xor dx, dx
    mov cx, 4286
    mov bx, [cs:engine_h]
    mov ah, 0x3f
    int 0x21

    ; Close engine file
    mov bx, [cs:engine_h]
    mov ah, 0x3e
    int 0x21

    ; Now setup call to engine:
    ; engine(read_cb, write_cb, method, param4)
    push cs
    pop ds

    ; Push parameters (C calling convention, right-to-left):
    ; param 4: dummy far pointer (CS:dummy_buf)
    push cs
    push word dummy_buf

    ; param 3: method (word) -> [method_num]
    push word [method_num]

    ; param 2: write_cb (far pointer CS:write_cb)
    push cs
    push word write_cb

    ; param 1: read_cb (far pointer CS:read_cb)
    push cs
    push word read_cb

    ; AX = 3 (min_match = 3)
    mov ax, 3

    ; Far call to engine_seg:0000
    call far [engine_entry]
    add sp, 14

    push cs
    pop ds

    ; Close files
    mov bx, [in_handle]
    mov ah, 0x3e
    int 0x21

    mov bx, [out_handle]
    mov ah, 0x3e
    int 0x21

    
    ; Write CALLS.BIN
    push cs
    pop ds
    mov dx, file_calls
    mov cx, 0
    mov ah, 0x3c
    int 0x21
    mov bx, ax
    mov dx, call_count
    mov cx, 130
    mov ah, 0x40
    int 0x21
    mov ah, 0x3e
    int 0x21

    mov dx, msg_ok
    mov ah, 0x09
    int 0x21

    mov ax, 0x4c00
    int 0x21

fail_open_in:
    mov dx, msg_in_err
    jmp fail_exit
fail_open_out:
    mov dx, msg_out_err
    jmp fail_exit
fail_engine:
    mov dx, msg_eng_err
    jmp fail_exit
fail_mem:
    mov dx, msg_mem_err
fail_exit:
    mov ah, 0x09
    int 0x21
    mov ax, 0x4c01
    int 0x21

; Read callback: read_cb(void far *buf, uint16_t size)
read_cb:
    push bp
    mov bp, sp
    push ds
    push bx
    push cx
    push dx

    mov bx, [cs:in_handle]
    mov cx, [bp+10] ; size
    mov dx, [bp+6]  ; offset
    mov ds, [bp+8]  ; seg
    mov ah, 0x3f    ; read
    int 0x21
    jnc read_ok
    xor ax, ax      ; on error return 0

read_ok:
    ; log read call: ax bytes read
    push bx
    push si
    mov si, [cs:read_call_count]
    shl si, 1
    mov [cs:read_call_sizes + si], ax
    inc word [cs:read_call_count]
    pop si
    pop bx

    pop dx
    pop cx
    pop bx
    pop ds
    pop bp
    retf

; Write callback: write_cb(void far *buf, uint16_t size)
write_cb:
    push bp
    mov bp, sp
    push ds
    push bx
    push cx
    push dx

    ; save size to call log
    push ax
    push si
    mov ax, [bp+10] ; size
    mov si, [cs:call_count]
    shl si, 1
    mov [cs:call_sizes + si], ax
    inc word [cs:call_count]
    pop si
    pop ax

    mov bx, [cs:out_handle]
    mov cx, [bp+10] ; size
    mov dx, [bp+6]  ; offset
    mov ds, [bp+8]  ; seg
    mov ah, 0x40    ; write
    int 0x21

    xor ax, ax      ; return 0 for success

    pop dx
    pop cx
    pop bx
    pop ds
    pop bp
    retf

in_handle dw 0
out_handle dw 0
engine_h dw 0
method_num dw 0 ; 0 = Method 0 (/M0 Best)

engine_entry:
engine_off dw 0
engine_seg dw 0

dummy_buf times 16 db 0

file_in db "IN.DAT", 0
file_out db "OUT.BIN", 0
file_engine db "ENGINE.BIN", 0

msg_ok db "Compression finished successfully!", 0x0d, 0x0a, "$"
msg_in_err db "Error opening IN.DAT!", 0x0d, 0x0a, "$"
msg_out_err db "Error creating OUT.BIN!", 0x0d, 0x0a, "$"
msg_eng_err db "Error opening ENGINE.BIN!", 0x0d, 0x0a, "$"
msg_mem_err db "Error allocating memory!", 0x0d, 0x0a, "$"

call_count dw 0
call_sizes times 32 dw 0

file_calls db "CALLS.BIN", 0

read_call_count dw 0
read_call_sizes times 32 dw 0
