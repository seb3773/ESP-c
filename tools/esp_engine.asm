00000000  2EA3BC10          mov [cs:0x10bc],ax
00000004  BB63CD            mov bx,0xcd63
00000007  C1EB04            shr bx,byte 0x4
0000000A  B448              mov ah,0x48
0000000C  CD21              int 0x21
0000000E  7304              jnc 0x14
00000010  B80100            mov ax,0x1
00000013  CB                retf
00000014  55                push bp
00000015  8BEC              mov bp,sp
00000017  56                push si
00000018  57                push di
00000019  8CDB              mov bx,ds
0000001B  8ED8              mov ds,ax
0000001D  8EC0              mov es,ax
0000001F  891E28C8          mov [0xc828],bx
00000023  8C162AC8          mov [0xc82a],ss
00000027  89262CC8          mov [0xc82c],sp
0000002B  668B4606          mov eax,[bp+0x6]
0000002F  66A342C8          mov [0xc842],eax
00000033  668B460A          mov eax,[bp+0xa]
00000037  66A33EC8          mov [0xc83e],eax
0000003B  8B5E0E            mov bx,[bp+0xe]
0000003E  891E46C8          mov [0xc846],bx
00000042  03DB              add bx,bx
00000044  2E8B873410        mov ax,[cs:bx+0x1034]
00000049  A334C8            mov [0xc834],ax
0000004C  C7060AC400A4      mov word [0xc40a],0xa400
00000052  33C0              xor ax,ax
00000054  A32EC8            mov [0xc82e],ax
00000057  A330C8            mov [0xc830],ax
0000005A  A249C8            mov [0xc849],al
0000005D  C60648C801        mov byte [0xc848],0x1
00000062  FC                cld
00000063  BF0CC4            mov di,0xc40c
00000066  B90E02            mov cx,0x20e
00000069  F3AB              rep stosw
0000006B  833E46C803        cmp word [0xc846],byte +0x3
00000070  7417              jz 0x89
00000072  BF00A4            mov di,0xa400
00000075  B80081            mov ax,0x8100
00000078  B90010            mov cx,0x1000
0000007B  F3AB              rep stosw
0000007D  33C0              xor ax,ax
0000007F  33FF              xor di,di
00000081  B98000            mov cx,0x80
00000084  F3AB              rep stosw
00000086  A300C4            mov [0xc400],ax
00000089  BB0010            mov bx,0x1000
0000008C  B448              mov ah,0x48
0000008E  CD21              int 0x21
00000090  7329              jnc 0xbb
00000092  8B0E2EC8          mov cx,[0xc82e]
00000096  E306              jcxz 0x9e
00000098  8EC1              mov es,cx
0000009A  B449              mov ah,0x49
0000009C  CD21              int 0x21
0000009E  8B0E30C8          mov cx,[0xc830]
000000A2  E306              jcxz 0xaa
000000A4  8EC1              mov es,cx
000000A6  B449              mov ah,0x49
000000A8  CD21              int 0x21
000000AA  1E                push ds
000000AB  07                pop es
000000AC  8E1E28C8          mov ds,[0xc828]
000000B0  B449              mov ah,0x49
000000B2  CD21              int 0x21
000000B4  5F                pop di
000000B5  5E                pop si
000000B6  5D                pop bp
000000B7  B80100            mov ax,0x1
000000BA  CB                retf
000000BB  A32EC8            mov [0xc82e],ax
000000BE  BBFA06            mov bx,0x6fa
000000C1  B448              mov ah,0x48
000000C3  CD21              int 0x21
000000C5  BA0010            mov dx,0x1000
000000C8  731C              jnc 0xe6
000000CA  81EB4001          sub bx,0x140
000000CE  72C2              jc 0x92
000000D0  81FBFA06          cmp bx,0x6fa
000000D4  72BC              jc 0x92
000000D6  8BC3              mov ax,bx
000000D8  C1E004            shl ax,byte 0x4
000000DB  2D925F            sub ax,0x5f92
000000DE  50                push ax
000000DF  B448              mov ah,0x48
000000E1  CD21              int 0x21
000000E3  5A                pop dx
000000E4  72AC              jc 0x92
000000E6  A330C8            mov [0xc830],ax
000000E9  8EC0              mov es,ax
000000EB  81C27A5E          add dx,0x5e7a
000000EF  2689167F5B        mov [es:0x5b7f],dx
000000F4  26C706844F8413    mov word [es:0x4f84],0x1384
000000FB  26C706884F8413    mov word [es:0x4f88],0x1384
00000102  26C606874F80      mov byte [es:0x4f87],0x80
00000108  8B1E28C8          mov bx,[0xc828]
0000010C  668B0E3EC8        mov ecx,[0xc83e]
00000111  268C1E8A4F        mov [es:0x4f8a],ds
00000116  8ED8              mov ds,ax
00000118  891E904F          mov [0x4f90],bx
0000011C  66890E8C4F        mov [0x4f8c],ecx
00000121  6633C0            xor eax,eax
00000124  A27E5B            mov [0x5b7e],al
00000127  A3925F            mov [0x5f92],ax
0000012A  662EA32C10        mov [cs:0x102c],eax
0000012F  662EA33010        mov [cs:0x1030],eax
00000134  C7067C5B925F      mov word [0x5b7c],0x5f92
0000013A  C706825BDB59      mov word [0x5b82],0x59db
00000140  C606D85901        mov byte [0x59d8],0x1
00000145  A3D959            mov [0x59d9],ax
00000148  8E1E8A4F          mov ds,[0x4f8a]
0000014C  8CD8              mov ax,ds
0000014E  FA                cli
0000014F  8ED0              mov ss,ax
00000151  BC54CD            mov sp,0xcd54
00000154  FB                sti
00000155  8E062EC8          mov es,[0xc82e]
00000159  89264CC8          mov [0xc84c],sp
0000015D  833E46C803        cmp word [0xc846],byte +0x3
00000162  750C              jnz 0x170
00000164  81060AC40020      add word [0xc40a],0x2000
0000016A  E8FF01            call 0x36c
0000016D  E97901            jmp 0x2e9
00000170  C70608C40081      mov word [0xc408],0x8100
00000176  BD0001            mov bp,0x100
00000179  C70606C40000      mov word [0xc406],0x0
0000017F  892E02C4          mov [0xc402],bp
00000183  E8E603            call 0x56c
00000186  A102C4            mov ax,[0xc402]
00000189  2D0301            sub ax,0x103
0000018C  A304C4            mov [0xc404],ax
0000018F  8B3E06C4          mov di,[0xc406]
00000193  BA0301            mov dx,0x103
00000196  26C706FE01FF80    mov word [es:0x1fe],0x80ff
0000019D  83ED03            sub bp,byte +0x3
000001A0  E98600            jmp 0x229
000001A3  E8AF06            call 0x855
000001A6  06                push es
000001A7  8E0630C8          mov es,[0xc830]
000001AB  26813E844FF44D    cmp word [es:0x4f84],0x4df4
000001B2  7203              jc 0x1b7
000001B4  E85807            call 0x90f
000001B7  07                pop es
000001B8  81FCD4CB          cmp sp,0xcbd4
000001BC  72E5              jc 0x1a3
000001BE  8B1E00C4          mov bx,[0xc400]
000001C2  B9030F            mov cx,0xf03
000001C5  D3E3              shl bx,cl
000001C7  025B02            add bl,[bp+di+0x2]
000001CA  22FD              and bh,ch
000001CC  03DB              add bx,bx
000001CE  8BC5              mov ax,bp
000001D0  878700A4          xchg ax,[bx-0x5c00]
000001D4  891E00C4          mov [0xc400],bx
000001D8  3BE8              cmp bp,ax
000001DA  7904              jns 0x1e0
000001DC  8D860080          lea ax,[bp-0x8000]
000001E0  8BF5              mov si,bp
000001E2  268902            mov [es:bp+si],ax
000001E5  A138C8            mov ax,[0xc838]
000001E8  A33CC8            mov [0xc83c],ax
000001EB  A136C8            mov ax,[0xc836]
000001EE  A33AC8            mov [0xc83a],ax
000001F1  E80004            call 0x5f4
000001F4  A13AC8            mov ax,[0xc83a]
000001F7  2E3B06BC10        cmp ax,[cs:0x10bc]
000001FC  7206              jc 0x204
000001FE  3B0636C8          cmp ax,[0xc836]
00000202  7316              jnc 0x21a
00000204  8A9B00FF          mov bl,[bp+di-0x100]
00000208  32FF              xor bh,bh
0000020A  53                push bx
0000020B  03DB              add bx,bx
0000020D  FF870CC4          inc word [bx-0x3bf4]
00000211  45                inc bp
00000212  392E04C4          cmp [0xc404],bp
00000216  79A0              jns 0x1b8
00000218  EB59              jmp short 0x273
0000021A  8BD0              mov dx,ax
0000021C  05FD00            add ax,0xfd
0000021F  50                push ax
00000220  8D86FFFE          lea ax,[bp-0x101]
00000224  2B063CC8          sub ax,[0xc83c]
00000228  50                push ax
00000229  8B1E00C4          mov bx,[0xc400]
0000022D  B9030F            mov cx,0xf03
00000230  4A                dec dx
00000231  7426              jz 0x259
00000233  45                inc bp
00000234  D3E3              shl bx,cl
00000236  025B02            add bl,[bp+di+0x2]
00000239  22FD              and bh,ch
0000023B  03DB              add bx,bx
0000023D  8BC5              mov ax,bp
0000023F  878700A4          xchg ax,[bx-0x5c00]
00000243  8BF5              mov si,bp
00000245  3BE8              cmp bp,ax
00000247  7807              js 0x250
00000249  268902            mov [es:bp+si],ax
0000024C  EBE2              jmp short 0x230
0000024E  8BC0              mov ax,ax
00000250  8D860080          lea ax,[bp-0x8000]
00000254  268902            mov [es:bp+si],ax
00000257  EBD7              jmp short 0x230
00000259  891E00C4          mov [0xc400],bx
0000025D  E8EC03            call 0x64c
00000260  45                inc bp
00000261  392E04C4          cmp [0xc404],bp
00000265  0F894FFF          jns near 0x1b8
00000269  A102C4            mov ax,[0xc402]
0000026C  2BC5              sub ax,bp
0000026E  05FF00            add ax,0xff
00000271  7870              js 0x2e3
00000273  A102C4            mov ax,[0xc402]
00000276  2BC5              sub ax,bp
00000278  3D0301            cmp ax,0x103
0000027B  7707              ja 0x284
0000027D  E8EC02            call 0x56c
00000280  8B3E06C4          mov di,[0xc406]
00000284  A108C4            mov ax,[0xc408]
00000287  2BC5              sub ax,bp
00000289  3D0001            cmp ax,0x100
0000028C  7724              ja 0x2b2
0000028E  8D860080          lea ax,[bp-0x8000]
00000292  A308C4            mov [0xc408],ax
00000295  BB00A4            mov bx,0xa400
00000298  B90400            mov cx,0x4
0000029B  90                nop
0000029C  3907              cmp [bx],ax
0000029E  7902              jns 0x2a2
000002A0  8907              mov [bx],ax
000002A2  394702            cmp [bx+0x2],ax
000002A5  7903              jns 0x2aa
000002A7  894702            mov [bx+0x2],ax
000002AA  03D9              add bx,cx
000002AC  81FB00C4          cmp bx,0xc400
000002B0  72EA              jc 0x29c
000002B2  A102C4            mov ax,[0xc402]
000002B5  2D0301            sub ax,0x103
000002B8  2BC5              sub ax,bp
000002BA  790F              jns 0x2cb
000002BC  3DFEFD            cmp ax,0xfdfe
000002BF  7C28              jl 0x2e9
000002C1  3D00FE            cmp ax,0xfe00
000002C4  0F8C3CFF          jl near 0x204
000002C8  E9EDFE            jmp 0x1b8
000002CB  8B1E08C4          mov bx,[0xc408]
000002CF  81EB0001          sub bx,0x100
000002D3  2BDD              sub bx,bp
000002D5  3BC3              cmp ax,bx
000002D7  7602              jna 0x2db
000002D9  8BC3              mov ax,bx
000002DB  03C5              add ax,bp
000002DD  A304C4            mov [0xc404],ax
000002E0  E9D5FE            jmp 0x1b8
000002E3  8BF4              mov si,sp
000002E5  40                inc ax
000002E6  014402            add [si+0x2],ax
000002E9  E86905            call 0x855
000002EC  E82006            call 0x90f
000002EF  1E                push ds
000002F0  8E1E30C8          mov ds,[0xc830]
000002F4  8B1E825B          mov bx,[0x5b82]
000002F8  C60701            mov byte [bx],0x1
000002FB  C747010100        mov word [bx+0x1],0x1
00000300  8306825B03        add word [0x5b82],byte +0x3
00000305  E89407            call 0xa9c
00000308  8B0E7C5B          mov cx,[0x5b7c]
0000030C  B8925F            mov ax,0x5f92
0000030F  2BC8              sub cx,ax
00000311  803E7E5B00        cmp byte [0x5b7e],0x0
00000316  7409              jz 0x321
00000318  41                inc cx
00000319  803E7E5B08        cmp byte [0x5b7e],0x8
0000031E  7601              jna 0x321
00000320  41                inc cx
00000321  E310              jcxz 0x333
00000323  51                push cx
00000324  1E                push ds
00000325  50                push ax
00000326  368E1E28C8        mov ds,[ss:0xc828]
0000032B  36FF1E3EC8        call far [ss:0xc83e]
00000330  83C406            add sp,byte +0x6
00000333  1F                pop ds
00000334  FA                cli
00000335  8E162AC8          mov ss,[0xc82a]
00000339  8B262CC8          mov sp,[0xc82c]
0000033D  FB                sti
0000033E  8E0630C8          mov es,[0xc830]
00000342  B449              mov ah,0x49
00000344  CD21              int 0x21
00000346  8E062EC8          mov es,[0xc82e]
0000034A  B449              mov ah,0x49
0000034C  CD21              int 0x21
0000034E  8CD8              mov ax,ds
00000350  8E1E28C8          mov ds,[0xc828]
00000354  8EC0              mov es,ax
00000356  B449              mov ah,0x49
00000358  CD21              int 0x21
0000035A  33C0              xor ax,ax
0000035C  5F                pop di
0000035D  5E                pop si
0000035E  5D                pop bp
0000035F  662E8B1E3010      mov ebx,[cs:0x1030]
00000365  662E8B0E2C10      mov ecx,[cs:0x102c]
0000036B  CB                retf
0000036C  B80080            mov ax,0x8000
0000036F  33FF              xor di,di
00000371  B90080            mov cx,0x8000
00000374  F3AB              rep stosw
00000376  C70608C40080      mov word [0xc408],0x8000
0000037C  33ED              xor bp,bp
0000037E  892E06C4          mov [0xc406],bp
00000382  892E02C4          mov [0xc402],bp
00000386  E8E301            call 0x56c
00000389  A102C4            mov ax,[0xc402]
0000038C  2D0101            sub ax,0x101
0000038F  A304C4            mov [0xc404],ax
00000392  8B3E06C4          mov di,[0xc406]
00000396  FF364CC8          push word [0xc84c]
0000039A  89264CC8          mov [0xc84c],sp
0000039E  392E04C4          cmp [0xc404],bp
000003A2  793A              jns 0x3de
000003A4  392E02C4          cmp [0xc402],bp
000003A8  0F844801          jz near 0x4f4
000003AC  E84D01            call 0x4fc
000003AF  A102C4            mov ax,[0xc402]
000003B2  2BC5              sub ax,bp
000003B4  3D0400            cmp ax,0x4
000003B7  720B              jc 0x3c4
000003B9  81FCCCCB          cmp sp,0xcbcc
000003BD  0F821B01          jc near 0x4dc
000003C1  EB1B              jmp short 0x3de
000003C3  90                nop
000003C4  8A1B              mov bl,[bp+di]
000003C6  32FF              xor bh,bh
000003C8  53                push bx
000003C9  03DB              add bx,bx
000003CB  FF870CC4          inc word [bx-0x3bf4]
000003CF  45                inc bp
000003D0  392E04C4          cmp [0xc404],bp
000003D4  78CE              js 0x3a4
000003D6  81FCCCCB          cmp sp,0xcbcc
000003DA  0F82FE00          jc near 0x4dc
000003DE  8B5B02            mov bx,[bp+di+0x2]
000003E1  8B0B              mov cx,[bp+di]
000003E3  8BF3              mov si,bx
000003E5  D1CB              ror bx,1
000003E7  2BD9              sub bx,cx
000003E9  1ADF              sbb bl,bh
000003EB  C1E302            shl bx,byte 0x2
000003EE  8BC5              mov ax,bp
000003F0  268707            xchg ax,[es:bx]
000003F3  8BD0              mov dx,ax
000003F5  26875702          xchg dx,[es:bx+0x2]
000003F9  3BE8              cmp bp,ax
000003FB  78C7              js 0x3c4
000003FD  8BD8              mov bx,ax
000003FF  3909              cmp [bx+di],cx
00000401  8D8D0001          lea cx,[di+0x100]
00000405  753A              jnz 0x441
00000407  397102            cmp [bx+di+0x2],si
0000040A  7535              jnz 0x441
0000040C  83C704            add di,byte +0x4
0000040F  90                nop
00000410  8B03              mov ax,[bp+di]
00000412  2B01              sub ax,[bx+di]
00000414  7516              jnz 0x42c
00000416  83C702            add di,byte +0x2
00000419  3BF9              cmp di,cx
0000041B  7314              jnc 0x431
0000041D  8B03              mov ax,[bp+di]
0000041F  2B01              sub ax,[bx+di]
00000421  7509              jnz 0x42c
00000423  83C702            add di,byte +0x2
00000426  3BF9              cmp di,cx
00000428  72E6              jc 0x410
0000042A  EB05              jmp short 0x431
0000042C  2C01              sub al,0x1
0000042E  83D700            adc di,byte +0x0
00000431  87DA              xchg bx,dx
00000433  3BEB              cmp bp,bx
00000435  7813              js 0x44a
00000437  8B43FF            mov ax,[bp+di-0x1]
0000043A  3943FF            cmp [bp+di-0x1],ax
0000043D  750B              jnz 0x44a
0000043F  EB57              jmp short 0x498
00000441  8BDA              mov bx,dx
00000443  3BEB              cmp bp,bx
00000445  7951              jns 0x498
00000447  E97AFF            jmp 0x3c4
0000044A  FECD              dec ch
0000044C  87CF              xchg cx,di
0000044E  2BCF              sub cx,di
00000450  83F904            cmp cx,byte +0x4
00000453  72F2              jc 0x447
00000455  8BC5              mov ax,bp
00000457  03E9              add bp,cx
00000459  81C1FD00          add cx,0xfd
0000045D  51                push cx
0000045E  2BD0              sub dx,ax
00000460  42                inc dx
00000461  F7DA              neg dx
00000463  52                push dx
00000464  392E04C4          cmp [0xc404],bp
00000468  7809              js 0x473
0000046A  81FCC4CB          cmp sp,0xcbc4
0000046E  726C              jc 0x4dc
00000470  E96BFF            jmp 0x3de
00000473  A102C4            mov ax,[0xc402]
00000476  2BC5              sub ax,bp
00000478  7817              js 0x491
0000047A  7415              jz 0x491
0000047C  50                push ax
0000047D  E87C00            call 0x4fc
00000480  58                pop ax
00000481  3D0400            cmp ax,0x4
00000484  0F823CFF          jc near 0x3c4
00000488  81FCC4CB          cmp sp,0xcbc4
0000048C  724E              jc 0x4dc
0000048E  E94DFF            jmp 0x3de
00000491  8BF4              mov si,sp
00000493  014402            add [si+0x2],ax
00000496  EB5C              jmp short 0x4f4
00000498  8BF1              mov si,cx
0000049A  81EE0001          sub si,0x100
0000049E  8B02              mov ax,[bp+si]
000004A0  3900              cmp [bx+si],ax
000004A2  752D              jnz 0x4d1
000004A4  8B4202            mov ax,[bp+si+0x2]
000004A7  394002            cmp [bx+si+0x2],ax
000004AA  7525              jnz 0x4d1
000004AC  83C604            add si,byte +0x4
000004AF  90                nop
000004B0  8B02              mov ax,[bp+si]
000004B2  2B00              sub ax,[bx+si]
000004B4  7516              jnz 0x4cc
000004B6  83C602            add si,byte +0x2
000004B9  3BF1              cmp si,cx
000004BB  7314              jnc 0x4d1
000004BD  8B02              mov ax,[bp+si]
000004BF  2B00              sub ax,[bx+si]
000004C1  7509              jnz 0x4cc
000004C3  83C602            add si,byte +0x2
000004C6  3BF1              cmp si,cx
000004C8  72E6              jc 0x4b0
000004CA  EB05              jmp short 0x4d1
000004CC  2C01              sub al,0x1
000004CE  83D600            adc si,byte +0x0
000004D1  3BF7              cmp si,di
000004D3  7604              jna 0x4d9
000004D5  8BFE              mov di,si
000004D7  8BD3              mov dx,bx
000004D9  E96EFF            jmp 0x44a
000004DC  E87603            call 0x855
000004DF  06                push es
000004E0  8E0630C8          mov es,[0xc830]
000004E4  26813E844FF44D    cmp word [es:0x4f84],0x4df4
000004EB  7203              jc 0x4f0
000004ED  E81F04            call 0x90f
000004F0  07                pop es
000004F1  E9EAFE            jmp 0x3de
000004F4  E85E03            call 0x855
000004F7  8F064CC8          pop word [0xc84c]
000004FB  C3                ret
000004FC  A102C4            mov ax,[0xc402]
000004FF  2BC5              sub ax,bp
00000501  3D0001            cmp ax,0x100
00000504  7703              ja 0x509
00000506  E86300            call 0x56c
00000509  3B2E08C4          cmp bp,[0xc408]
0000050D  7921              jns 0x530
0000050F  A102C4            mov ax,[0xc402]
00000512  2D0101            sub ax,0x101
00000515  2BC5              sub ax,bp
00000517  780D              js 0x526
00000519  8B1E08C4          mov bx,[0xc408]
0000051D  4B                dec bx
0000051E  2BDD              sub bx,bp
00000520  3BC3              cmp ax,bx
00000522  7202              jc 0x526
00000524  8BC3              mov ax,bx
00000526  03C5              add ax,bp
00000528  A304C4            mov [0xc404],ax
0000052B  8B3E06C4          mov di,[0xc406]
0000052F  C3                ret
00000530  A108C4            mov ax,[0xc408]
00000533  050080            add ax,0x8000
00000536  A308C4            mov [0xc408],ax
00000539  06                push es
0000053A  1F                pop ds
0000053B  33DB              xor bx,bx
0000053D  B90800            mov cx,0x8
00000540  3907              cmp [bx],ax
00000542  7904              jns 0x548
00000544  8907              mov [bx],ax
00000546  EB05              jmp short 0x54d
00000548  394702            cmp [bx+0x2],ax
0000054B  7903              jns 0x550
0000054D  894702            mov [bx+0x2],ax
00000550  394704            cmp [bx+0x4],ax
00000553  7905              jns 0x55a
00000555  894704            mov [bx+0x4],ax
00000558  EB05              jmp short 0x55f
0000055A  394706            cmp [bx+0x6],ax
0000055D  7903              jns 0x562
0000055F  894706            mov [bx+0x6],ax
00000562  03D9              add bx,cx
00000564  75DA              jnz 0x540
00000566  16                push ss
00000567  1F                pop ds
00000568  EBA5              jmp short 0x50f
0000056A  8BC0              mov ax,ax
0000056C  56                push si
0000056D  57                push di
0000056E  A102C4            mov ax,[0xc402]
00000571  8B1E06C4          mov bx,[0xc406]
00000575  03C3              add ax,bx
00000577  3B060AC4          cmp ax,[0xc40a]
0000057B  752C              jnz 0x5a9
0000057D  BF0001            mov di,0x100
00000580  8BF0              mov si,ax
00000582  B90083            mov cx,0x8300
00000585  2BF1              sub si,cx
00000587  D1E9              shr cx,1
00000589  1E                push ds
0000058A  07                pop es
0000058B  2BDE              sub bx,si
0000058D  03DF              add bx,di
0000058F  81FB0080          cmp bx,0x8000
00000593  750B              jnz 0x5a0
00000595  833E46C803        cmp word [0xc846],byte +0x3
0000059A  7404              jz 0x5a0
0000059C  2BDF              sub bx,di
0000059E  2BFF              sub di,di
000005A0  891E06C4          mov [0xc406],bx
000005A4  FC                cld
000005A5  F3A5              rep movsw
000005A7  8BC7              mov ax,di
000005A9  8B360AC4          mov si,[0xc40a]
000005AD  2BF0              sub si,ax
000005AF  7903              jns 0x5b4
000005B1  BE0080            mov si,0x8000
000005B4  803E49C801        cmp byte [0xc849],0x1
000005B9  742F              jz 0x5ea
000005BB  0BF6              or si,si
000005BD  742B              jz 0x5ea
000005BF  A102C4            mov ax,[0xc402]
000005C2  030606C4          add ax,[0xc406]
000005C6  56                push si
000005C7  1E                push ds
000005C8  50                push ax
000005C9  8E1E28C8          mov ds,[0xc828]
000005CD  36FF1E42C8        call far [ss:0xc842]
000005D2  83C406            add sp,byte +0x6
000005D5  8CD2              mov dx,ss
000005D7  8EDA              mov ds,dx
000005D9  0BC0              or ax,ax
000005DB  7505              jnz 0x5e2
000005DD  C60649C801        mov byte [0xc849],0x1
000005E2  010602C4          add [0xc402],ax
000005E6  2BF0              sub si,ax
000005E8  EBCA              jmp short 0x5b4
000005EA  8E062EC8          mov es,[0xc82e]
000005EE  5F                pop di
000005EF  5E                pop si
000005F0  C3                ret
000005F1  C3                ret
000005F2  8BC0              mov ax,ax
000005F4  3D0300            cmp ax,0x3
000005F7  7253              jc 0x64c
000005F9  8DB6FFFE          lea si,[bp-0x101]
000005FD  03F0              add si,ax
000005FF  03F6              add si,si
00000601  8BDD              mov bx,bp
00000603  262B1C            sub bx,[es:si]
00000606  78E9              js 0x5f1
00000608  8BD5              mov dx,bp
0000060A  262B54FE          sub dx,[es:si-0x2]
0000060E  78E1              js 0x5f1
00000610  2D0200            sub ax,0x2
00000613  F7D8              neg ax
00000615  8BC8              mov cx,ax
00000617  3BD3              cmp dx,bx
00000619  7C03              jl 0x61e
0000061B  8BDA              mov bx,dx
0000061D  41                inc cx
0000061E  83EE04            sub si,byte +0x4
00000621  40                inc ax
00000622  40                inc ax
00000623  7F12              jg 0x637
00000625  8BD5              mov dx,bp
00000627  262B14            sub dx,[es:si]
0000062A  83EE02            sub si,byte +0x2
0000062D  3BDA              cmp bx,dx
0000062F  7FF1              jg 0x622
00000631  8BDA              mov bx,dx
00000633  8BC8              mov cx,ax
00000635  EBEB              jmp short 0x622
00000637  8B1634C8          mov dx,[0xc834]
0000063B  8B8301FF          mov ax,[bp+di-0xff]
0000063F  2BDD              sub bx,bp
00000641  F7DB              neg bx
00000643  8BF1              mov si,cx
00000645  03F7              add si,di
00000647  E31F              jcxz 0x668
00000649  E90C01            jmp 0x758
0000064C  8BF7              mov si,di
0000064E  8B1634C8          mov dx,[0xc834]
00000652  8D9E01FF          lea bx,[bp-0xff]
00000656  03DB              add bx,bx
00000658  268B1F            mov bx,[es:bx]
0000065B  3BEB              cmp bp,bx
0000065D  0F887F00          js near 0x6e0
00000661  8B8301FF          mov ax,[bp+di-0xff]
00000665  668BC0            mov eax,eax
00000668  4A                dec dx
00000669  7C75              jl 0x6e0
0000066B  3900              cmp [bx+si],ax
0000066D  7429              jz 0x698
0000066F  03DB              add bx,bx
00000671  268B1F            mov bx,[es:bx]
00000674  3BEB              cmp bp,bx
00000676  7868              js 0x6e0
00000678  4A                dec dx
00000679  3900              cmp [bx+si],ax
0000067B  741B              jz 0x698
0000067D  03DB              add bx,bx
0000067F  268B1F            mov bx,[es:bx]
00000682  3BEB              cmp bp,bx
00000684  785A              js 0x6e0
00000686  4A                dec dx
00000687  3900              cmp [bx+si],ax
00000689  740D              jz 0x698
0000068B  03DB              add bx,bx
0000068D  268B1F            mov bx,[es:bx]
00000690  3BEB              cmp bp,bx
00000692  79D4              jns 0x668
00000694  EB4A              jmp short 0x6e0
00000696  8BC0              mov ax,ax
00000698  8CD8              mov ax,ds
0000069A  8EC0              mov es,ax
0000069C  8BC6              mov ax,si
0000069E  8D31              lea si,[bx+di]
000006A0  8DBB01FF          lea di,[bp+di-0xff]
000006A4  B98000            mov cx,0x80
000006A7  F3A7              repe cmpsw
000006A9  743D              jz 0x6e8
000006AB  8A4CFE            mov cl,[si-0x2]
000006AE  2A4DFE            sub cl,[di-0x2]
000006B1  80C1FF            add cl,0xff
000006B4  83DE02            sbb si,byte +0x2
000006B7  2BF3              sub si,bx
000006B9  8E062EC8          mov es,[0xc82e]
000006BD  8B3E06C4          mov di,[0xc406]
000006C1  8D4D02            lea cx,[di+0x2]
000006C4  3BF1              cmp si,cx
000006C6  7C04              jl 0x6cc
000006C8  3BF0              cmp si,ax
000006CA  7F38              jg 0x704
000006CC  8BF0              mov si,ax
000006CE  03DB              add bx,bx
000006D0  268B1F            mov bx,[es:bx]
000006D3  3BEB              cmp bp,bx
000006D5  7809              js 0x6e0
000006D7  8B8201FF          mov ax,[bp+si-0xff]
000006DB  EB8B              jmp short 0x668
000006DD  668BC0            mov eax,eax
000006E0  2BF7              sub si,di
000006E2  46                inc si
000006E3  893636C8          mov [0xc836],si
000006E7  C3                ret
000006E8  8B3E06C4          mov di,[0xc406]
000006EC  8DB5FF00          lea si,[di+0xff]
000006F0  891E38C8          mov [0xc838],bx
000006F4  8E062EC8          mov es,[0xc82e]
000006F8  EBE6              jmp short 0x6e0
000006FA  8BC0              mov ax,ax
000006FC  D1EE              shr si,1
000006FE  46                inc si
000006FF  893636C8          mov [0xc836],si
00000703  C3                ret
00000704  891E38C8          mov [0xc838],bx
00000708  2BF7              sub si,di
0000070A  03F6              add si,si
0000070C  03ED              add bp,bp
0000070E  268B8A00FE        mov cx,[es:bp+si-0x200]
00000713  D1DD              rcr bp,1
00000715  3BE9              cmp bp,cx
00000717  78E3              js 0x6fc
00000719  B80200            mov ax,0x2
0000071C  3BCB              cmp cx,bx
0000071E  7824              js 0x744
00000720  40                inc ax
00000721  03DB              add bx,bx
00000723  268B48FC          mov cx,[es:bx+si-0x4]
00000727  3BE9              cmp bp,cx
00000729  78D1              js 0x6fc
0000072B  268B1F            mov bx,[es:bx]
0000072E  3BEB              cmp bp,bx
00000730  78CA              js 0x6fc
00000732  3BCB              cmp cx,bx
00000734  780E              js 0x744
00000736  D1EE              shr si,1
00000738  03F7              add si,di
0000073A  8B8201FF          mov ax,[bp+si-0xff]
0000073E  E927FF            jmp 0x668
00000741  668BC0            mov eax,eax
00000744  D1EE              shr si,1
00000746  46                inc si
00000747  893636C8          mov [0xc836],si
0000074B  2BF0              sub si,ax
0000074D  F7DE              neg si
0000074F  03F7              add si,di
00000751  8BD9              mov bx,cx
00000753  8B8301FF          mov ax,[bp+di-0xff]
00000757  90                nop
00000758  4A                dec dx
00000759  7C29              jl 0x784
0000075B  3900              cmp [bx+si],ax
0000075D  7441              jz 0x7a0
0000075F  03DB              add bx,bx
00000761  268B1F            mov bx,[es:bx]
00000764  3BEB              cmp bp,bx
00000766  781C              js 0x784
00000768  4A                dec dx
00000769  3900              cmp [bx+si],ax
0000076B  7433              jz 0x7a0
0000076D  03DB              add bx,bx
0000076F  268B1F            mov bx,[es:bx]
00000772  3BEB              cmp bp,bx
00000774  780E              js 0x784
00000776  4A                dec dx
00000777  3900              cmp [bx+si],ax
00000779  7425              jz 0x7a0
0000077B  03DB              add bx,bx
0000077D  268B1F            mov bx,[es:bx]
00000780  3BEB              cmp bp,bx
00000782  79D4              jns 0x758
00000784  C3                ret
00000785  8E062EC8          mov es,[0xc82e]
00000789  81EE0001          sub si,0x100
0000078D  8B3E06C4          mov di,[0xc406]
00000791  2BF7              sub si,di
00000793  893638C8          mov [0xc838],si
00000797  C70636C80001      mov word [0xc836],0x100
0000079D  C3                ret
0000079E  8BC0              mov ax,ax
000007A0  8CD8              mov ax,ds
000007A2  8EC0              mov es,ax
000007A4  8BC6              mov ax,si
000007A6  8D7002            lea si,[bx+si+0x2]
000007A9  8DBB03FF          lea di,[bp+di-0xfd]
000007AD  B97F00            mov cx,0x7f
000007B0  F3A7              repe cmpsw
000007B2  74D1              jz 0x785
000007B4  8A4CFE            mov cl,[si-0x2]
000007B7  2A4DFE            sub cl,[di-0x2]
000007BA  80C1FF            add cl,0xff
000007BD  1BFD              sbb di,bp
000007BF  8D8DFE00          lea cx,[di+0xfe]
000007C3  8B3E06C4          mov di,[0xc406]
000007C7  8E062EC8          mov es,[0xc82e]
000007CB  8BF0              mov si,ax
000007CD  2BCF              sub cx,di
000007CF  3B0E36C8          cmp cx,[0xc836]
000007D3  7E47              jng 0x81c
000007D5  890E36C8          mov [0xc836],cx
000007D9  2BF7              sub si,di
000007DB  8D00              lea ax,[bx+si]
000007DD  A338C8            mov [0xc838],ax
000007E0  03E9              add bp,cx
000007E2  03ED              add bp,bp
000007E4  268B86FEFD        mov ax,[es:bp-0x202]
000007E9  D1DD              rcr bp,1
000007EB  2BE9              sub bp,cx
000007ED  3BE8              cmp bp,ax
000007EF  7863              js 0x854
000007F1  3BC3              cmp ax,bx
000007F3  7837              js 0x82c
000007F5  03DB              add bx,bx
000007F7  03F1              add si,cx
000007F9  03F6              add si,si
000007FB  268B40FA          mov ax,[es:bx+si-0x6]
000007FF  3BE8              cmp bp,ax
00000801  7851              js 0x854
00000803  268B1F            mov bx,[es:bx]
00000806  3BEB              cmp bp,bx
00000808  784A              js 0x854
0000080A  3BC3              cmp ax,bx
0000080C  7832              js 0x840
0000080E  D1EE              shr si,1
00000810  2BF1              sub si,cx
00000812  03F7              add si,di
00000814  8B8301FF          mov ax,[bp+di-0xff]
00000818  E93DFF            jmp 0x758
0000081B  90                nop
0000081C  03DB              add bx,bx
0000081E  268B1F            mov bx,[es:bx]
00000821  3BEB              cmp bp,bx
00000823  782F              js 0x854
00000825  8B8301FF          mov ax,[bp+di-0xff]
00000829  E92CFF            jmp 0x758
0000082C  8BD8              mov bx,ax
0000082E  8BF1              mov si,cx
00000830  83EE02            sub si,byte +0x2
00000833  F7DE              neg si
00000835  03F7              add si,di
00000837  8B8301FF          mov ax,[bp+di-0xff]
0000083B  E91AFF            jmp 0x758
0000083E  8BC0              mov ax,ax
00000840  8BD8              mov bx,ax
00000842  8BF1              mov si,cx
00000844  83EE03            sub si,byte +0x3
00000847  F7DE              neg si
00000849  03F7              add si,di
0000084B  8B8301FF          mov ax,[bp+di-0xff]
0000084F  E906FF            jmp 0x758
00000852  8BC0              mov ax,ax
00000854  C3                ret
00000855  8BC4              mov ax,sp
00000857  050200            add ax,0x2
0000085A  3B064CC8          cmp ax,[0xc84c]
0000085E  7501              jnz 0x861
00000860  C3                ret
00000861  A34EC8            mov [0xc84e],ax
00000864  55                push bp
00000865  57                push di
00000866  06                push es
00000867  8E0630C8          mov es,[0xc830]
0000086B  8B364CC8          mov si,[0xc84c]
0000086F  83EE02            sub si,byte +0x2
00000872  268B3E844F        mov di,[es:0x4f84]
00000877  268B2E884F        mov bp,[es:0x4f88]
0000087C  268B16864F        mov dx,[es:0x4f86]
00000881  EB0D              jmp short 0x890
00000883  90                nop
00000884  26885600          mov [es:bp+0x0],dl
00000888  32D2              xor dl,dl
0000088A  8BEF              mov bp,di
0000088C  47                inc di
0000088D  EB05              jmp short 0x894
0000088F  90                nop
00000890  D0C6              rol dh,1
00000892  72F0              jc 0x884
00000894  8B04              mov ax,[si]
00000896  80C4FF            add ah,0xff
00000899  720D              jc 0x8a8
0000089B  AA                stosb
0000089C  83EE02            sub si,byte +0x2
0000089F  39364EC8          cmp [0xc84e],si
000008A3  76EB              jna 0x890
000008A5  EB4B              jmp short 0x8f2
000008A7  90                nop
000008A8  41                inc cx
000008A9  0AD6              or dl,dh
000008AB  AA                stosb
000008AC  8BD8              mov bx,ax
000008AE  03DB              add bx,bx
000008B0  FF872CC6          inc word [bx-0x39d4]
000008B4  8B44FE            mov ax,[si-0x2]
000008B7  AB                stosw
000008B8  32FF              xor bh,bh
000008BA  84E4              test ah,ah
000008BC  751A              jnz 0x8d8
000008BE  84C0              test al,al
000008C0  780E              js 0x8d0
000008C2  8AD8              mov bl,al
000008C4  2E8A9F3C10        mov bl,[cs:bx+0x103c]
000008C9  FEC7              inc bh
000008CB  EB16              jmp short 0x8e3
000008CD  668BC0            mov eax,eax
000008D0  BB0801            mov bx,0x108
000008D3  EB0E              jmp short 0x8e3
000008D5  668BC0            mov eax,eax
000008D8  8ADC              mov bl,ah
000008DA  2E8A9F3C10        mov bl,[cs:bx+0x103c]
000008DF  81C30801          add bx,0x108
000008E3  03DB              add bx,bx
000008E5  FF870CC4          inc word [bx-0x3bf4]
000008E9  83EE04            sub si,byte +0x4
000008EC  39364EC8          cmp [0xc84e],si
000008F0  769E              jna 0x890
000008F2  26893E844F        mov [es:0x4f84],di
000008F7  26885600          mov [es:bp+0x0],dl
000008FB  268916864F        mov [es:0x4f86],dx
00000900  26892E884F        mov [es:0x4f88],bp
00000905  07                pop es
00000906  5F                pop di
00000907  5D                pop bp
00000908  58                pop ax
00000909  8B264CC8          mov sp,[0xc84c]
0000090D  50                push ax
0000090E  C3                ret
0000090F  06                push es
00000910  55                push bp
00000911  56                push si
00000912  57                push di
00000913  803E48C801        cmp byte [0xc848],0x1
00000918  7417              jz 0x931
0000091A  8E1E30C8          mov ds,[0xc830]
0000091E  8B1E825B          mov bx,[0x5b82]
00000922  C60701            mov byte [bx],0x1
00000925  C747010000        mov word [bx+0x1],0x0
0000092A  8306825B03        add word [0x5b82],byte +0x3
0000092F  16                push ss
00000930  1F                pop ds
00000931  C60648C800        mov byte [0xc848],0x0
00000936  FF364CC8          push word [0xc84c]
0000093A  89264CC8          mov [0xc84c],sp
0000093E  680001            push word 0x100
00000941  68FF7F            push word 0x7fff
00000944  E80EFF            call 0x855
00000947  8F064CC8          pop word [0xc84c]
0000094B  8E0630C8          mov es,[0xc830]
0000094F  BE0CC4            mov si,0xc40c
00000952  BF924F            mov di,0x4f92
00000955  B90E02            mov cx,0x20e
00000958  FC                cld
00000959  F3A5              rep movsw
0000095B  8CC2              mov dx,es
0000095D  8CD8              mov ax,ds
0000095F  8EC0              mov es,ax
00000961  33C0              xor ax,ax
00000963  BF0CC4            mov di,0xc40c
00000966  FC                cld
00000967  B90E02            mov cx,0x20e
0000096A  F3AB              rep stosw
0000096C  892632C8          mov [0xc832],sp
00000970  FA                cli
00000971  8ED2              mov ss,dx
00000973  BC905F            mov sp,0x5f90
00000976  FB                sti
00000977  8EDA              mov ds,dx
00000979  68AE53            push word 0x53ae
0000097C  68CA57            push word 0x57ca
0000097F  68924F            push word 0x4f92
00000982  681001            push word 0x110
00000985  E88C02            call 0xc14
00000988  83C408            add sp,byte +0x8
0000098B  68CE55            push word 0x55ce
0000098E  68DA58            push word 0x58da
00000991  68B251            push word 0x51b2
00000994  68FE00            push word 0xfe
00000997  E87A02            call 0xc14
0000099A  83C408            add sp,byte +0x8
0000099D  68CA57            push word 0x57ca
000009A0  681001            push word 0x110
000009A3  E8D704            call 0xe7d
000009A6  83C404            add sp,byte +0x4
000009A9  68DA58            push word 0x58da
000009AC  68FE00            push word 0xfe
000009AF  E8CB04            call 0xe7d
000009B2  83C404            add sp,byte +0x4
000009B5  1E                push ds
000009B6  07                pop es
000009B7  BE8413            mov si,0x1384
000009BA  8B3E825B          mov di,[0x5b82]
000009BE  893E825B          mov [0x5b82],di
000009C2  E8D700            call 0xa9c
000009C5  8B3E825B          mov di,[0x5b82]
000009C9  EB15              jmp short 0x9e0
000009CB  90                nop
000009CC  84DB              test bl,bl
000009CE  7809              js 0x9d9
000009D0  2E8A9F3C10        mov bl,[cs:bx+0x103c]
000009D5  FEC7              inc bh
000009D7  EB45              jmp short 0xa1e
000009D9  BB0801            mov bx,0x108
000009DC  EB40              jmp short 0xa1e
000009DE  8BC0              mov ax,ax
000009E0  81FF045B          cmp di,0x5b04
000009E4  73D8              jnc 0x9be
000009E6  AC                lodsb
000009E7  B401              mov ah,0x1
000009E9  8BE8              mov bp,ax
000009EB  90                nop
000009EC  D1ED              shr bp,1
000009EE  74F0              jz 0x9e0
000009F0  7216              jc 0xa08
000009F2  AC                lodsb
000009F3  32E4              xor ah,ah
000009F5  8BD8              mov bx,ax
000009F7  8A87CA57          mov al,[bx+0x57ca]
000009FB  AA                stosb
000009FC  03DB              add bx,bx
000009FE  8B87AE53          mov ax,[bx+0x53ae]
00000A02  AB                stosw
00000A03  EBE7              jmp short 0x9ec
00000A05  668BC0            mov eax,eax
00000A08  8B5C01            mov bx,[si+0x1]
00000A0B  8BD3              mov dx,bx
00000A0D  84FF              test bh,bh
00000A0F  74BB              jz 0x9cc
00000A11  8ADF              mov bl,bh
00000A13  32FF              xor bh,bh
00000A15  2E8A9F3C10        mov bl,[cs:bx+0x103c]
00000A1A  81C30801          add bx,0x108
00000A1E  8A87CA57          mov al,[bx+0x57ca]
00000A22  AA                stosb
00000A23  03DB              add bx,bx
00000A25  8B87AE53          mov ax,[bx+0x53ae]
00000A29  AB                stosw
00000A2A  8BC3              mov ax,bx
00000A2C  D1E8              shr ax,1
00000A2E  32E4              xor ah,ah
00000A30  8AC8              mov cl,al
00000A32  BBFF7F            mov bx,0x7fff
00000A35  D3C3              rol bx,cl
00000A37  23D3              and dx,bx
00000A39  A80E              test al,0xe
00000A3B  740A              jz 0xa47
00000A3D  48                dec ax
00000A3E  AA                stosb
00000A3F  8BC2              mov ax,dx
00000A41  AB                stosw
00000A42  3DFF3F            cmp ax,0x3fff
00000A45  742D              jz 0xa74
00000A47  8A1C              mov bl,[si]
00000A49  32FF              xor bh,bh
00000A4B  8A87DA58          mov al,[bx+0x58da]
00000A4F  AA                stosb
00000A50  03DB              add bx,bx
00000A52  8B87CE55          mov ax,[bx+0x55ce]
00000A56  AB                stosw
00000A57  83C603            add si,byte +0x3
00000A5A  D1EB              shr bx,1
00000A5C  83C303            add bx,byte +0x3
00000A5F  662EFF063010      inc dword [cs:0x1030]
00000A65  2E011E2C10        add [cs:0x102c],bx
00000A6A  7380              jnc 0x9ec
00000A6C  2EFF062E10        inc word [cs:0x102e]
00000A71  E978FF            jmp 0x9ec
00000A74  893E825B          mov [0x5b82],di
00000A78  C706844F8413      mov word [0x4f84],0x1384
00000A7E  C706884F8413      mov word [0x4f88],0x1384
00000A84  C606874F80        mov byte [0x4f87],0x80
00000A89  A18A4F            mov ax,[0x4f8a]
00000A8C  8ED8              mov ds,ax
00000A8E  FA                cli
00000A8F  8ED0              mov ss,ax
00000A91  8B2632C8          mov sp,[0xc832]
00000A95  FB                sti
00000A96  5F                pop di
00000A97  5E                pop si
00000A98  5D                pop bp
00000A99  07                pop es
00000A9A  C3                ret
00000A9B  90                nop
00000A9C  06                push es
00000A9D  55                push bp
00000A9E  56                push si
00000A9F  57                push di
00000AA0  1E                push ds
00000AA1  07                pop es
00000AA2  A1825B            mov ax,[0x5b82]
00000AA5  BED859            mov si,0x59d8
00000AA8  2BC6              sub ax,si
00000AAA  B503              mov ch,0x3
00000AAC  F6F5              div ch
00000AAE  8AE8              mov ch,al
00000AB0  8B3E7C5B          mov di,[0x5b7c]
00000AB4  8B15              mov dx,[di]
00000AB6  8A0E7E5B          mov cl,[0x5b7e]
00000ABA  84ED              test ch,ch
00000ABC  7436              jz 0xaf4
00000ABE  8BC0              mov ax,ax
00000AC0  8B4401            mov ax,[si+0x1]
00000AC3  D3C0              rol ax,cl
00000AC5  020C              add cl,[si]
00000AC7  F6C110            test cl,0x10
00000ACA  750C              jnz 0xad8
00000ACC  0BD0              or dx,ax
00000ACE  83C603            add si,byte +0x3
00000AD1  FECD              dec ch
00000AD3  75EB              jnz 0xac0
00000AD5  EB1D              jmp short 0xaf4
00000AD7  90                nop
00000AD8  80E1EF            and cl,0xef
00000ADB  BBFFFF            mov bx,0xffff
00000ADE  D3E3              shl bx,cl
00000AE0  8BE8              mov bp,ax
00000AE2  23C3              and ax,bx
00000AE4  0BC2              or ax,dx
00000AE6  AB                stosw
00000AE7  8BD3              mov dx,bx
00000AE9  F7D2              not dx
00000AEB  23D5              and dx,bp
00000AED  83C603            add si,byte +0x3
00000AF0  FECD              dec ch
00000AF2  75CC              jnz 0xac0
00000AF4  8915              mov [di],dx
00000AF6  880E7E5B          mov [0x5b7e],cl
00000AFA  893E7C5B          mov [0x5b7c],di
00000AFE  C706825BD859      mov word [0x5b82],0x59d8
00000B04  8D856EA0          lea ax,[di-0x5f92]
00000B08  393E7F5B          cmp [0x5b7f],di
00000B0C  7733              ja 0xb41
00000B0E  52                push dx
00000B0F  1E                push ds
00000B10  50                push ax
00000B11  1E                push ds
00000B12  68925F            push word 0x5f92
00000B15  8E1E904F          mov ds,[0x4f90]
00000B19  26FF1E8C4F        call far [es:0x4f8c]
00000B1E  83C406            add sp,byte +0x6
00000B21  1F                pop ds
00000B22  0BC0              or ax,ax
00000B24  7411              jz 0xb37
00000B26  1E                push ds
00000B27  07                pop es
00000B28  A18A4F            mov ax,[0x4f8a]
00000B2B  8ED8              mov ds,ax
00000B2D  FA                cli
00000B2E  8ED0              mov ss,ax
00000B30  BC54CD            mov sp,0xcd54
00000B33  FB                sti
00000B34  E9FDF7            jmp 0x334
00000B37  C7067C5B925F      mov word [0x5b7c],0x5f92
00000B3D  8F06925F          pop word [0x5f92]
00000B41  5F                pop di
00000B42  5E                pop si
00000B43  5D                pop bp
00000B44  07                pop es
00000B45  C3                ret
00000B46  55                push bp
00000B47  8BEC              mov bp,sp
00000B49  56                push si
00000B4A  57                push di
00000B4B  8B4604            mov ax,[bp+0x4]
00000B4E  A32004            mov [0x420],ax
00000B51  8B4606            mov ax,[bp+0x6]
00000B54  A32204            mov [0x422],ax
00000B57  8B4608            mov ax,[bp+0x8]
00000B5A  A32404            mov [0x424],ax
00000B5D  1E                push ds
00000B5E  07                pop es
00000B5F  B8FFFF            mov ax,0xffff
00000B62  FC                cld
00000B63  BF2002            mov di,0x220
00000B66  B90001            mov cx,0x100
00000B69  F3AB              rep stosw
00000B6B  BF2604            mov di,0x426
00000B6E  B90001            mov cx,0x100
00000B71  F3AB              rep stosw
00000B73  8B3E2404          mov di,[0x424]
00000B77  4F                dec di
00000B78  D1E7              shl di,1
00000B7A  BA0200            mov dx,0x2
00000B7D  8B2E2004          mov bp,[0x420]
00000B81  668BC0            mov eax,eax
00000B84  8A1B              mov bl,[bp+di]
00000B86  32FF              xor bh,bh
00000B88  D1E3              shl bx,1
00000B8A  8BC7              mov ax,di
00000B8C  D1E8              shr ax,1
00000B8E  87872002          xchg ax,[bx+0x220]
00000B92  8905              mov [di],ax
00000B94  2BFA              sub di,dx
00000B96  79EC              jns 0xb84
00000B98  BE2002            mov si,0x220
00000B9B  BA2004            mov dx,0x420
00000B9E  45                inc bp
00000B9F  AD                lodsw
00000BA0  0BC0              or ax,ax
00000BA2  7814              js 0xbb8
00000BA4  8BF8              mov di,ax
00000BA6  03FF              add di,di
00000BA8  8A1B              mov bl,[bp+di]
00000BAA  32FF              xor bh,bh
00000BAC  03DB              add bx,bx
00000BAE  87872604          xchg ax,[bx+0x426]
00000BB2  8705              xchg ax,[di]
00000BB4  0BC0              or ax,ax
00000BB6  79EC              jns 0xba4
00000BB8  3BF2              cmp si,dx
00000BBA  72E3              jc 0xb9f
00000BBC  BE2406            mov si,0x624
00000BBF  BA2604            mov dx,0x426
00000BC2  8B3E2404          mov di,[0x424]
00000BC6  4F                dec di
00000BC7  D1E7              shl di,1
00000BC9  033E2204          add di,[0x422]
00000BCD  FD                std
00000BCE  8BC0              mov ax,ax
00000BD0  AD                lodsw
00000BD1  0BC0              or ax,ax
00000BD3  780D              js 0xbe2
00000BD5  668BC0            mov eax,eax
00000BD8  AB                stosw
00000BD9  93                xchg ax,bx
00000BDA  03DB              add bx,bx
00000BDC  8B07              mov ax,[bx]
00000BDE  0BC0              or ax,ax
00000BE0  79F6              jns 0xbd8
00000BE2  3BF2              cmp si,dx
00000BE4  73EA              jnc 0xbd0
00000BE6  FC                cld
00000BE7  5F                pop di
00000BE8  5E                pop si
00000BE9  5D                pop bp
00000BEA  C3                ret
00000BEB  837B02FF          cmp word [bp+di+0x2],byte -0x1
00000BEF  7415              jz 0xc06
00000BF1  83C602            add si,byte +0x2
00000BF4  8B7B02            mov di,[bp+di+0x2]
00000BF7  57                push di
00000BF8  E8F0FF            call 0xbeb
00000BFB  5F                pop di
00000BFC  83C704            add di,byte +0x4
00000BFF  E8E9FF            call 0xbeb
00000C02  83EE02            sub si,byte +0x2
00000C05  C3                ret
00000C06  BB1E00            mov bx,0x1e
00000C09  3BF3              cmp si,bx
00000C0B  7702              ja 0xc0f
00000C0D  8BDE              mov bx,si
00000C0F  FF875E08          inc word [bx+0x85e]
00000C13  C3                ret
00000C14  55                push bp
00000C15  8BEC              mov bp,sp
00000C17  06                push es
00000C18  56                push si
00000C19  57                push di
00000C1A  8B4604            mov ax,[bp+0x4]
00000C1D  A32606            mov [0x626],ax
00000C20  8B4606            mov ax,[bp+0x6]
00000C23  A32C06            mov [0x62c],ax
00000C26  8B4608            mov ax,[bp+0x8]
00000C29  A32806            mov [0x628],ax
00000C2C  8B460A            mov ax,[bp+0xa]
00000C2F  A32A06            mov [0x62a],ax
00000C32  FF362606          push word [0x626]
00000C36  683E06            push word 0x63e
00000C39  FF362C06          push word [0x62c]
00000C3D  E806FF            call 0xb46
00000C40  83C406            add sp,byte +0x6
00000C43  BE3E06            mov si,0x63e
00000C46  8B162606          mov dx,[0x626]
00000C4A  D1E2              shl dx,1
00000C4C  03D6              add dx,si
00000C4E  8B2E2C06          mov bp,[0x62c]
00000C52  33C9              xor cx,cx
00000C54  AD                lodsw
00000C55  8BF8              mov di,ax
00000C57  D1E7              shl di,1
00000C59  3B0B              cmp cx,[bp+di]
00000C5B  7504              jnz 0xc61
00000C5D  3BF2              cmp si,dx
00000C5F  72F3              jc 0xc54
00000C61  83EE02            sub si,byte +0x2
00000C64  2BD6              sub dx,si
00000C66  83FA04            cmp dx,byte +0x4
00000C69  7305              jnc 0xc70
00000C6B  03F2              add si,dx
00000C6D  83EE04            sub si,byte +0x4
00000C70  89363806          mov [0x638],si
00000C74  A12606            mov ax,[0x626]
00000C77  8BDE              mov bx,si
00000C79  81EB3E06          sub bx,0x63e
00000C7D  D1EB              shr bx,1
00000C7F  2BC3              sub ax,bx
00000C81  D1E0              shl ax,1
00000C83  48                dec ax
00000C84  A33006            mov [0x630],ax
00000C87  8BD0              mov dx,ax
00000C89  C1E202            shl dx,byte 0x2
00000C8C  BD9E0A            mov bp,0xa9e
00000C8F  8BCE              mov cx,si
00000C91  33F6              xor si,si
00000C93  33FF              xor di,di
00000C95  C7063206FFFF      mov word [0x632],0xffff
00000C9B  8BD9              mov bx,cx
00000C9D  8B1F              mov bx,[bx]
00000C9F  D1E3              shl bx,1
00000CA1  031E2C06          add bx,[0x62c]
00000CA5  8B07              mov ax,[bx]
00000CA7  A33406            mov [0x634],ax
00000CAA  A12606            mov ax,[0x626]
00000CAD  D1E0              shl ax,1
00000CAF  053E06            add ax,0x63e
00000CB2  A33A06            mov [0x63a],ax
00000CB5  A13406            mov ax,[0x634]
00000CB8  3B063206          cmp ax,[0x632]
00000CBC  7730              ja 0xcee
00000CBE  8903              mov [bp+di],ax
00000CC0  C74302FFFF        mov word [bp+di+0x2],0xffff
00000CC5  83C102            add cx,byte +0x2
00000CC8  B8FFFF            mov ax,0xffff
00000CCB  3B0E3A06          cmp cx,[0x63a]
00000CCF  730C              jnc 0xcdd
00000CD1  8BD9              mov bx,cx
00000CD3  8B1F              mov bx,[bx]
00000CD5  D1E3              shl bx,1
00000CD7  031E2C06          add bx,[0x62c]
00000CDB  8B07              mov ax,[bx]
00000CDD  A33406            mov [0x634],ax
00000CE0  8BC7              mov ax,di
00000CE2  2BC6              sub ax,si
00000CE4  3D0400            cmp ax,0x4
00000CE7  7410              jz 0xcf9
00000CE9  83C704            add di,byte +0x4
00000CEC  EBC7              jmp short 0xcb5
00000CEE  A13206            mov ax,[0x632]
00000CF1  8903              mov [bp+di],ax
00000CF3  897302            mov [bp+di+0x2],si
00000CF6  83C608            add si,byte +0x8
00000CF9  B8FFFF            mov ax,0xffff
00000CFC  3BF7              cmp si,di
00000CFE  7305              jnc 0xd05
00000D00  8B02              mov ax,[bp+si]
00000D02  034204            add ax,[bp+si+0x4]
00000D05  A33206            mov [0x632],ax
00000D08  83C704            add di,byte +0x4
00000D0B  3BFA              cmp di,dx
00000D0D  72A6              jc 0xcb5
00000D0F  1E                push ds
00000D10  07                pop es
00000D11  33C0              xor ax,ax
00000D13  B91000            mov cx,0x10
00000D16  BF5E08            mov di,0x85e
00000D19  FC                cld
00000D1A  F3AB              rep stosw
00000D1C  8B3E3006          mov di,[0x630]
00000D20  4F                dec di
00000D21  C1E702            shl di,byte 0x2
00000D24  BEFEFF            mov si,0xfffe
00000D27  E8C1FE            call 0xbeb
00000D2A  83F10F            xor cx,byte +0xf
00000D2D  33D2              xor dx,dx
00000D2F  BE5E08            mov si,0x85e
00000D32  AD                lodsw
00000D33  D3E0              shl ax,cl
00000D35  03D0              add dx,ax
00000D37  49                dec cx
00000D38  7DF8              jnl 0xd32
00000D3A  29167C08          sub [0x87c],dx
00000D3E  4A                dec dx
00000D3F  7C13              jl 0xd54
00000D41  BE7A08            mov si,0x87a
00000D44  833C00            cmp word [si],byte +0x0
00000D47  7503              jnz 0xd4c
00000D49  83EE02            sub si,byte +0x2
00000D4C  FF0C              dec word [si]
00000D4E  83440202          add word [si+0x2],byte +0x2
00000D52  EBEA              jmp short 0xd3e
00000D54  8B3E2806          mov di,[0x628]
00000D58  8B0E2606          mov cx,[0x626]
00000D5C  33C0              xor ax,ax
00000D5E  8B0E2606          mov cx,[0x626]
00000D62  D1E9              shr cx,1
00000D64  7301              jnc 0xd67
00000D66  AA                stosb
00000D67  F3AB              rep stosw
00000D69  33D2              xor dx,dx
00000D6B  8B362606          mov si,[0x626]
00000D6F  4E                dec si
00000D70  D1E6              shl si,1
00000D72  8B2E2806          mov bp,[0x628]
00000D76  C7063C060000      mov word [0x63c],0x0
00000D7C  B90F00            mov cx,0xf
00000D7F  2BCA              sub cx,dx
00000D81  B80100            mov ax,0x1
00000D84  D3E0              shl ax,cl
00000D86  A33606            mov [0x636],ax
00000D89  8BDA              mov bx,dx
00000D8B  D1E3              shl bx,1
00000D8D  8B875E08          mov ax,[bx+0x85e]
00000D91  A32E06            mov [0x62e],ax
00000D94  0BC0              or ax,ax
00000D96  742E              jz 0xdc6
00000D98  8BCA              mov cx,dx
00000D9A  41                inc cx
00000D9B  8BBC3E06          mov di,[si+0x63e]
00000D9F  880B              mov [bp+di],cl
00000DA1  8B1E3C06          mov bx,[0x63c]
00000DA5  D1E3              shl bx,1
00000DA7  D1D8              rcr ax,1
00000DA9  E2FA              loop 0xda5
00000DAB  B90F00            mov cx,0xf
00000DAE  2BCA              sub cx,dx
00000DB0  D3E8              shr ax,cl
00000DB2  89847E08          mov [si+0x87e],ax
00000DB6  83EE02            sub si,byte +0x2
00000DB9  A13606            mov ax,[0x636]
00000DBC  01063C06          add [0x63c],ax
00000DC0  FF0E2E06          dec word [0x62e]
00000DC4  75D2              jnz 0xd98
00000DC6  42                inc dx
00000DC7  83FA10            cmp dx,byte +0x10
00000DCA  72B0              jc 0xd7c
00000DCC  FF362606          push word [0x626]
00000DD0  683E06            push word 0x63e
00000DD3  FF362806          push word [0x628]
00000DD7  E8E601            call 0xfc0
00000DDA  83C406            add sp,byte +0x6
00000DDD  BB3E06            mov bx,0x63e
00000DE0  A13806            mov ax,[0x638]
00000DE3  8BF0              mov si,ax
00000DE5  2BF3              sub si,bx
00000DE7  81C67E08          add si,0x87e
00000DEB  8B162606          mov dx,[0x626]
00000DEF  D1E2              shl dx,1
00000DF1  03D3              add dx,bx
00000DF3  2BD0              sub dx,ax
00000DF5  03D3              add dx,bx
00000DF7  8B2E2A06          mov bp,[0x62a]
00000DFB  B90200            mov cx,0x2
00000DFE  AD                lodsw
00000DFF  8B3F              mov di,[bx]
00000E01  D1E7              shl di,1
00000E03  8903              mov [bp+di],ax
00000E05  03D9              add bx,cx
00000E07  3BDA              cmp bx,dx
00000E09  72F3              jc 0xdfe
00000E0B  5F                pop di
00000E0C  5E                pop si
00000E0D  07                pop es
00000E0E  5D                pop bp
00000E0F  C3                ret
00000E10  55                push bp
00000E11  8BEC              mov bp,sp
00000E13  56                push si
00000E14  57                push di
00000E15  8B4604            mov ax,[bp+0x4]
00000E18  A31C13            mov [0x131c],ax
00000E1B  8B4606            mov ax,[bp+0x6]
00000E1E  A31E13            mov [0x131e],ax
00000E21  8B4608            mov ax,[bp+0x8]
00000E24  A31A13            mov [0x131a],ax
00000E27  8B3E825B          mov di,[0x5b82]
00000E2B  8B1E1A13          mov bx,[0x131a]
00000E2F  8BD3              mov dx,bx
00000E31  031E1C13          add bx,[0x131c]
00000E35  4B                dec bx
00000E36  8BF3              mov si,bx
00000E38  803C00            cmp byte [si],0x0
00000E3B  7505              jnz 0xe42
00000E3D  4E                dec si
00000E3E  3BF2              cmp si,dx
00000E40  73F6              jnc 0xe38
00000E42  A11E13            mov ax,[0x131e]
00000E45  AA                stosb
00000E46  8BC3              mov ax,bx
00000E48  2BC6              sub ax,si
00000E4A  AB                stosw
00000E4B  8BD6              mov dx,si
00000E4D  8B361A13          mov si,[0x131a]
00000E51  FC                cld
00000E52  3BF2              cmp si,dx
00000E54  771F              ja 0xe75
00000E56  AC                lodsb
00000E57  3C06              cmp al,0x6
00000E59  770B              ja 0xe66
00000E5B  8AD8              mov bl,al
00000E5D  B80300            mov ax,0x3
00000E60  AA                stosb
00000E61  8AC3              mov al,bl
00000E63  AB                stosw
00000E64  EBEC              jmp short 0xe52
00000E66  2C03              sub al,0x3
00000E68  AA                stosb
00000E69  B111              mov cl,0x11
00000E6B  2AC8              sub cl,al
00000E6D  B8FFFF            mov ax,0xffff
00000E70  D3E8              shr ax,cl
00000E72  AB                stosw
00000E73  EBDD              jmp short 0xe52
00000E75  893E825B          mov [0x5b82],di
00000E79  5F                pop di
00000E7A  5E                pop si
00000E7B  5D                pop bp
00000E7C  C3                ret
00000E7D  55                push bp
00000E7E  8BEC              mov bp,sp
00000E80  06                push es
00000E81  56                push si
00000E82  57                push di
00000E83  8B4604            mov ax,[bp+0x4]
00000E86  A32013            mov [0x1320],ax
00000E89  8B4606            mov ax,[bp+0x6]
00000E8C  A32213            mov [0x1322],ax
00000E8F  8B1E2213          mov bx,[0x1322]
00000E93  8BD3              mov dx,bx
00000E95  031E2013          add bx,[0x1320]
00000E99  4B                dec bx
00000E9A  8BF3              mov si,bx
00000E9C  803C00            cmp byte [si],0x0
00000E9F  7505              jnz 0xea6
00000EA1  4E                dec si
00000EA2  3BF2              cmp si,dx
00000EA4  73F6              jnc 0xe9c
00000EA6  2BDE              sub bx,si
00000EA8  53                push bx
00000EA9  8BEE              mov bp,si
00000EAB  1E                push ds
00000EAC  07                pop es
00000EAD  BF3813            mov di,0x1338
00000EB0  33C0              xor ax,ax
00000EB2  B91300            mov cx,0x13
00000EB5  FC                cld
00000EB6  F3AB              rep stosw
00000EB8  8B362213          mov si,[0x1322]
00000EBC  3BF5              cmp si,bp
00000EBE  773F              ja 0xeff
00000EC0  AC                lodsb
00000EC1  0AC0              or al,al
00000EC3  740C              jz 0xed1
00000EC5  32E4              xor ah,ah
00000EC7  D1E0              shl ax,1
00000EC9  8BD8              mov bx,ax
00000ECB  FF873C13          inc word [bx+0x133c]
00000ECF  EBEB              jmp short 0xebc
00000ED1  8BD6              mov dx,si
00000ED3  AC                lodsb
00000ED4  0AC0              or al,al
00000ED6  74FB              jz 0xed3
00000ED8  2BD6              sub dx,si
00000EDA  F7DA              neg dx
00000EDC  4E                dec si
00000EDD  83FA02            cmp dx,byte +0x2
00000EE0  7706              ja 0xee8
00000EE2  01163813          add [0x1338],dx
00000EE6  EBD4              jmp short 0xebc
00000EE8  83FA13            cmp dx,byte +0x13
00000EEB  770C              ja 0xef9
00000EED  7504              jnz 0xef3
00000EEF  FF063813          inc word [0x1338]
00000EF3  FF063A13          inc word [0x133a]
00000EF7  EBC3              jmp short 0xebc
00000EF9  FF063C13          inc word [0x133c]
00000EFD  EBBD              jmp short 0xebc
00000EFF  685E13            push word 0x135e
00000F02  682413            push word 0x1324
00000F05  683813            push word 0x1338
00000F08  6A13              push byte +0x13
00000F0A  E807FD            call 0xc14
00000F0D  83C408            add sp,byte +0x8
00000F10  682413            push word 0x1324
00000F13  6A05              push byte +0x5
00000F15  6A13              push byte +0x13
00000F17  E8F6FE            call 0xe10
00000F1A  83C406            add sp,byte +0x6
00000F1D  8B3E825B          mov di,[0x5b82]
00000F21  B009              mov al,0x9
00000F23  AA                stosb
00000F24  58                pop ax
00000F25  AB                stosw
00000F26  8306825B03        add word [0x5b82],byte +0x3
00000F2B  8B362213          mov si,[0x1322]
00000F2F  8BD5              mov dx,bp
00000F31  8B3E825B          mov di,[0x5b82]
00000F35  893E825B          mov [0x5b82],di
00000F39  52                push dx
00000F3A  E85FFB            call 0xa9c
00000F3D  5A                pop dx
00000F3E  8B3E825B          mov di,[0x5b82]
00000F42  81FF045B          cmp di,0x5b04
00000F46  73ED              jnc 0xf35
00000F48  3BF2              cmp si,dx
00000F4A  7769              ja 0xfb5
00000F4C  AC                lodsb
00000F4D  0AC0              or al,al
00000F4F  7412              jz 0xf63
00000F51  32E4              xor ah,ah
00000F53  8BD8              mov bx,ax
00000F55  8A872613          mov al,[bx+0x1326]
00000F59  AA                stosb
00000F5A  03DB              add bx,bx
00000F5C  8B876213          mov ax,[bx+0x1362]
00000F60  AB                stosw
00000F61  EBDF              jmp short 0xf42
00000F63  8BCE              mov cx,si
00000F65  AC                lodsb
00000F66  0AC0              or al,al
00000F68  74FB              jz 0xf65
00000F6A  2BCE              sub cx,si
00000F6C  F7D9              neg cx
00000F6E  4E                dec si
00000F6F  83F902            cmp cx,byte +0x2
00000F72  770C              ja 0xf80
00000F74  A02413            mov al,[0x1324]
00000F77  AA                stosb
00000F78  A15E13            mov ax,[0x135e]
00000F7B  AB                stosw
00000F7C  E2F6              loop 0xf74
00000F7E  EBC2              jmp short 0xf42
00000F80  83F913            cmp cx,byte +0x13
00000F83  771D              ja 0xfa2
00000F85  7509              jnz 0xf90
00000F87  A02413            mov al,[0x1324]
00000F8A  AA                stosb
00000F8B  A15E13            mov ax,[0x135e]
00000F8E  AB                stosw
00000F8F  49                dec cx
00000F90  A02513            mov al,[0x1325]
00000F93  AA                stosb
00000F94  A16013            mov ax,[0x1360]
00000F97  AB                stosw
00000F98  B004              mov al,0x4
00000F9A  AA                stosb
00000F9B  8BC1              mov ax,cx
00000F9D  2C03              sub al,0x3
00000F9F  AB                stosw
00000FA0  EBA0              jmp short 0xf42
00000FA2  A02613            mov al,[0x1326]
00000FA5  AA                stosb
00000FA6  A16213            mov ax,[0x1362]
00000FA9  AB                stosw
00000FAA  B009              mov al,0x9
00000FAC  AA                stosb
00000FAD  8BC1              mov ax,cx
00000FAF  2D1400            sub ax,0x14
00000FB2  AB                stosw
00000FB3  EB8D              jmp short 0xf42
00000FB5  893E825B          mov [0x5b82],di
00000FB9  5F                pop di
00000FBA  5E                pop si
00000FBB  07                pop es
00000FBC  5D                pop bp
00000FBD  C3                ret
00000FBE  8BC0              mov ax,ax
00000FC0  55                push bp
00000FC1  8BEC              mov bp,sp
00000FC3  56                push si
00000FC4  57                push di
00000FC5  8B4604            mov ax,[bp+0x4]
00000FC8  A32004            mov [0x420],ax
00000FCB  8B4606            mov ax,[bp+0x6]
00000FCE  A32204            mov [0x422],ax
00000FD1  8B4608            mov ax,[bp+0x8]
00000FD4  A32404            mov [0x424],ax
00000FD7  1E                push ds
00000FD8  07                pop es
00000FD9  BF2002            mov di,0x220
00000FDC  B8FFFF            mov ax,0xffff
00000FDF  B90001            mov cx,0x100
00000FE2  FC                cld
00000FE3  F3AB              rep stosw
00000FE5  8B3E2404          mov di,[0x424]
00000FE9  4F                dec di
00000FEA  8B2E2004          mov bp,[0x420]
00000FEE  8A1B              mov bl,[bp+di]
00000FF0  F6D3              not bl
00000FF2  32FF              xor bh,bh
00000FF4  D1E3              shl bx,1
00000FF6  8BC7              mov ax,di
00000FF8  87872002          xchg ax,[bx+0x220]
00000FFC  D1E7              shl di,1
00000FFE  8905              mov [di],ax
00001000  D1EF              shr di,1
00001002  4F                dec di
00001003  79E9              jns 0xfee
00001005  BE2002            mov si,0x220
00001008  BA2004            mov dx,0x420
0000100B  8B3E2204          mov di,[0x422]
0000100F  90                nop
00001010  AD                lodsw
00001011  0BC0              or ax,ax
00001013  780E              js 0x1023
00001015  668BC0            mov eax,eax
00001018  AB                stosw
00001019  8BD8              mov bx,ax
0000101B  D1E3              shl bx,1
0000101D  8B07              mov ax,[bx]
0000101F  0BC0              or ax,ax
00001021  79F5              jns 0x1018
00001023  3BF2              cmp si,dx
00001025  72E9              jc 0x1010
00001027  5F                pop di
00001028  5E                pop si
00001029  5D                pop bp
0000102A  C3                ret
0000102B  0000              add [bx+si],al
0000102D  0000              add [bx+si],al
0000102F  0000              add [bx+si],al
00001031  0000              add [bx+si],al
00001033  0000              add [bx+si],al
00001035  01800020          add [bx+si+0x2000],ax
00001039  0005              add [di],al
0000103B  0000              add [bx+si],al
0000103D  0102              add [bp+si],ax
0000103F  0203              add al,[bp+di]
00001041  0303              add ax,[bp+di]
00001043  0304              add ax,[si]
00001045  0404              add al,0x4
00001047  0404              add al,0x4
00001049  0404              add al,0x4
0000104B  0405              add al,0x5
0000104D  050505            add ax,0x505
00001050  050505            add ax,0x505
00001053  050505            add ax,0x505
00001056  050505            add ax,0x505
00001059  050505            add ax,0x505
0000105C  06                push es
0000105D  06                push es
0000105E  06                push es
0000105F  06                push es
00001060  06                push es
00001061  06                push es
00001062  06                push es
00001063  06                push es
00001064  06                push es
00001065  06                push es
00001066  06                push es
00001067  06                push es
00001068  06                push es
00001069  06                push es
0000106A  06                push es
0000106B  06                push es
0000106C  06                push es
0000106D  06                push es
0000106E  06                push es
0000106F  06                push es
00001070  06                push es
00001071  06                push es
00001072  06                push es
00001073  06                push es
00001074  06                push es
00001075  06                push es
00001076  06                push es
00001077  06                push es
00001078  06                push es
00001079  06                push es
0000107A  06                push es
0000107B  06                push es
0000107C  07                pop es
0000107D  07                pop es
0000107E  07                pop es
0000107F  07                pop es
00001080  07                pop es
00001081  07                pop es
00001082  07                pop es
00001083  07                pop es
00001084  07                pop es
00001085  07                pop es
00001086  07                pop es
00001087  07                pop es
00001088  07                pop es
00001089  07                pop es
0000108A  07                pop es
0000108B  07                pop es
0000108C  07                pop es
0000108D  07                pop es
0000108E  07                pop es
0000108F  07                pop es
00001090  07                pop es
00001091  07                pop es
00001092  07                pop es
00001093  07                pop es
00001094  07                pop es
00001095  07                pop es
00001096  07                pop es
00001097  07                pop es
00001098  07                pop es
00001099  07                pop es
0000109A  07                pop es
0000109B  07                pop es
0000109C  07                pop es
0000109D  07                pop es
0000109E  07                pop es
0000109F  07                pop es
000010A0  07                pop es
000010A1  07                pop es
000010A2  07                pop es
000010A3  07                pop es
000010A4  07                pop es
000010A5  07                pop es
000010A6  07                pop es
000010A7  07                pop es
000010A8  07                pop es
000010A9  07                pop es
000010AA  07                pop es
000010AB  07                pop es
000010AC  07                pop es
000010AD  07                pop es
000010AE  07                pop es
000010AF  07                pop es
000010B0  07                pop es
000010B1  07                pop es
000010B2  07                pop es
000010B3  07                pop es
000010B4  07                pop es
000010B5  07                pop es
000010B6  07                pop es
000010B7  07                pop es
000010B8  07                pop es
000010B9  07                pop es
000010BA  07                pop es
000010BB  07                pop es
000010BC  0300              add ax,[bx+si]
