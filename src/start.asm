; + ---------- + -- + -- + -- + -- + ----------- + -- + ----- + -- + ---- + ---------- +
; | 31      24 | 23 | 22 | 21 | 20 | 19       16 | 15 | 14 13 | 12 | 11 8 | 7        0 |
; + ---------- + -- + -- + -- + -- + ----------- + -- + ----- + -- + ---- + ---------- + (+4)
; | Base 31:24 | Gr | Db | Lo | Av | Limit 19:16 | Pr | Ring  | Dt | Type | Base 23:16 |
; + -------------------------------------------- + ----------------------------------- +
; | 31                                        16 | 15                                0 |
; + -------------------------------------------- + ----------------------------------- + (+0)
; | Base 15:00                                   | Limit 15:00                         |
; + -------------------------------------------- + ----------------------------------- +
;
; Gr - Granularity, implicitly multiply limit by 1024
; Db - Default operation size, 0=16bit/1=32bit
; Lo - Long, marks 64bit code segments
; Av - Available, Can be used by the OS
; Pr - Present, marks the existance of this segment in memory
; Dt - Descriptor type, 0=System, 1=Code/Data
;
; Type  -  4 bit
; Ring  -  2 bit, 00=Most Privileged, 11=Least Privileged
; Limit - 20 bit
; Base  - 32 bit

; Dummy segment
mov [0x500], dword 0
mov [0x504], dword 0

; Code segment
; [31:16] Base 0x????0000
; [16:00] Limit 0x?FFFF
mov [0x508], dword 0x0000FFFF

; [31:16] Base 0x00??????, Gr=1, Db=1, Lo=0, Av=0, Limit 0xF????
; [16:00] Pr=1 Ring=00, Dt=1, Type=0b1011 [ECRA], Base=0x??00????
mov [0x50C], dword 0x00CF9B00

; Data segment
; [31:16] Base 0x????0000
; [16:00] Limit 0x?FFFF
mov [0x510], dword 0x0000FFFF

; [31:16] Base 0x00??????, Gr=1, Db=1, Lo=0, Av=0, Limit 0xF????
; [16:00] Pr=1 Ring=00, Dt=1, Type=0b0011 [EDWA], Base=0x??00????
mov [0x514], dword 0x00CF9300
