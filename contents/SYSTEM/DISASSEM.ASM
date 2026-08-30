; ASM65 ASSEMBLER
;
; REV 1.4   8/11/76

FORMAT          := $40
LENGTH          := $41
LMNEM           := $42
RMNEM           := $43
PCL             := $44
PCH             := $45
COUNT           := $46
PRBYTE          := $FFDC
CHAROUT         := $FFEF
        .ORG    $0800
DSMBL:  LDA     #$13            ; COUNT FOR 20 INSTR DSMBLY.
        STA     COUNT
DSMBL2: JSR     INSTDSP         ; DISASSEMBLE AND DISPLAY INSTR.
        JSR     PCADJ
        STA     PCL             ; UPDATE PCL,H TO NEXT INSTR.
        STY     PCH
        DEC     COUNT           ; DONE FIRST 19 INSTRS.?
        BNE     DSMBL2          ; * YES, LOOP.  ELSE DSMBL 20TH.
INSTDSP:JSR     PRPC            ; PRINT PCL,H.
        LDA     (PCL,X)         ; GET OP CODE.
        TAY
        LSR     A               ; * EVEN/ODD TEST.
        BCC     IEVEN
        LSR     A               ; *  TEST B1.
        BCS     ERR             ; *  XXXXXX11 INSTR INVALID.
        CMP     #$22
        BEQ     ERR             ; *  10001001 INSTR INVALID.
        AND     #$07            ; MASK 3 BITS FOR ADDRESS MODE &
        ORA     #$80            ; *  ADD INDEXING OFFSET.
IEVEN:  LSR     A               ; * LSB INTO CARRY FOR
        TAX                     ; * LEFT/FIGHT TEST BELOW.
        LDA     MODE,X          ; INDEX INTO ADDRESS MODE TABL.
        BCS     RTMODE          ; IF CARRY SET USE LSD FOR
        LSR     A               ; * PRINT FORMAT INDEX.
        LSR     A
        LSR     A               ; * IF CARRY CLEAR USE MSD.
        LSR     A
RTMODE: AND     #$0F            ; MASK FOR 4-BIT INDEX.
        BNE     GETFMT          ; $0 FOR INVALID OPCODES.
ERR:    LDY     #$80            ; SUBSTITUTE $80 FOR INVALID OP,
        LDA     #$00            ; SET PRINT FORMAT INDEX TO 0.
GETFMT: TAX
        LDA     MODE2,X         ; INDEX INTO PRINT FORMAT TABLE.
        STA     FORMAT          ; SAVE FOR ADDRESS FIELD FORMAT.
        AND     #$03            ; MASK 2-BIT LENGTH.  0=1-BYTE,
        STA     LENGTH          ; *  1=2-BYTE, 2=3-BYTE.
        TYA                     ; * OP CODE.
        AND     #$8F            ; MASK IT FOR 1XXX1010 TEST.
        TAX                     ; * SAVE IT.
        TYA                     ; * OP CODE TO A AGAIN.
        LDY     #$03
        CPX     #$8A
        BEQ     MNNDX3
MNNDX1: LSR     A
        BCC     MNNDX3          ; FORM INDEX INTO MNEMONIC TABL
        LSR     A
MNNDX2: LSR     A               ; * 1XXX1010 -> 00101XXX
        ORA     #$20            ; * XXXYYY01 -> 00111XXX
        DEY                     ; * XXXYYY10 -> 00110XXX
        BNE     MNNDX2          ; * XXXYY100 -> 00100XXX
        INY                     ; * XXXXX000 -> 000XXXXX
MNNDX3: DEY
        BNE     MNNDX1
        PHA                     ; * SAVE MNEMONIC TABLE INDEX.
PROP:   LDA     (PCL),Y
        JSR     PRBYTE
        LDX     #$01
PROPBL: JSR     PRBL2
        CPY     LENGTH          ; PRINT INSTR (1 to 3 BYTES)
        INY                     ; *  IN A 12-CHARACTER FIELD.
        BCC     PROP
        LDX     #$03            ; CHAR COUNT FOR MNEMONIC PRINT.
        CPY     #$04
        BCC     PROPBL
        PLA                     ; * RECOVER MNEMONIC INDEX.
        TAY
        LDA     MNEML,Y
        STA     LMNEM           ; FETCH 3-CHAR MNEMONIC.
        LDA     MNEMR,Y         ; *  (PACKED IN 2 BYTES)
        STA     RMNEM
PRMN1:  LDA     #$00
        LDY     #$05
PRMN2:  ASL     RMNEM
        ROL     LMNEM           ; SHIFT 5 BITS OF CHAR INTO A.
        ROL     A               ; *   (CLEARS CARRY)
        DEY
        BNE     PRMN2
        ADC     #$BF            ; ADD '?' OFFSET.
        JSR     CHAROUT         ; OUTPUT A CHARACTER OF MNEMONIC
        DEX
        BNE     PRMN1           ; OUTPUT 3 BLANKS.
        JSR     PRBLNK          ; COUNT FOR 6 PRINT FORMAT BITS.
        LDX     #$06
PRADR1: CPX     #$03            ; IF X=3 THEN PRINT ADDRESS VAL.
        BNE     PRADR3
        LDY     LENGTH          ; NO PRINT IF LENGTH=0.
        BEQ     PRADR3
PRADR2: LDA     FORMAT          ; HANDLE REL ADRESSING MODE
        CMP     #$E8            ; SPECIAL (PRINT TARGET ADR)
        LDA     (PCL),Y         ; *   (NOT DISPACEMENT)
        BCS     RELADR          ; OUTPUT 1- OR 2-BYTE ADDRESS.
        JSR     PRBYTE          ; *  MORE SIGNIFICANT BYTE FIRST
        DEY
        BNE     PRADR2
PRADR3: ASL     FORMAT          ; TEST NEXT PRINT FORMAT BIT.
        BCC     PRADR4          ; IF 0, DON'T PRINT
        LDA     CHAR1-1,X       ; *  CORRESPONDING CHARS.
        JSR     CHAROUT         ; OUTPUT 1 OR 2 CHARS.
        LDA     CHAR2-1,X       ; *  (IF CHAR FROM CHAR2 IS 0,
        BEQ     PRADR4          ; *    DON'T OUTPUT IT)
        JSR     CHAROUT
PRADR4: DEX
        BNE     PRADR1
        RTS                     ; *RETURN IF DONE 6 FORMAT BITS.
RELADR: JSR     PCADJ3          ; PCL,H + DISPL + 1 TO A,Y.
        TAX
        INX
        BNE     PRNTYX          ;      +1 TO X,Y.
        INY
PRNTYX: TYA
PRNTAX: JSR     PRBYTE          ; PRINT TARGET ADR OF BRANCH
PRNTX:  TXA                     ; *  AND RETURN
        JMP     PRBYTE
PRPC:   LDA     #$8D
        JSR     CHAROUT         ; OUTPUT CARRIAGE RETURN.
        LDA     PCH
        LDX     PCL
        JSR     PRNTAX          ; OUTPUT PCH AND PCL.
        LDA     #$AD
        JSR     CHAROUT         ; OUTPUT '-'
PRBLNK: LDX     #$03            ; BLANK COUNT.
PRBL2:  LDA     #$A0
        JSR     CHAROUT         ; OUTPUT A BLANK.
PRBL3:  DEX
        BNE     PRBL2           ; LOOP UNTIL COUNT = 0.
        RTS
PCADJ:  LDA     LENGTH          ; 0=1-BYTE, 1=2-BYTE, 2=3-BYTE.
PCADJ2: SEC
PCADJ3: LDY     PCH
        TAX                     ; * TEST DISPL SIGN (FOR REL
        BPL     PCADJ4          ; *   BRANCH).  EXTEND NEG
        DEY                     ; *   BY DECREMENTING PCH.
PCADJ4: ADC     PCL
        BCC     RTS1            ; PCL+LENGTH (OR DISPL) +1 TO A.
        INY                     ; *  CARRY INTO Y (PCH)
RTS1:   RTS

MODE:   .BYTE   $40
        .BYTE   $02
        .BYTE   $45
        .BYTE   $03
        .BYTE   $D0
        .BYTE   $08
        .BYTE   $40
        .BYTE   $09
        .BYTE   $30             ; XXXXXXZ0 INSTRS.
        .BYTE   $22
        .BYTE   $45             ; *  Z=0, LEFT HALF-BYTE
        .BYTE   $33             ; *  Z=1, RIGHT HALF-BYTE
        .BYTE   $D0
        .BYTE   $08
        .BYTE   $40
        .BYTE   $09
        .BYTE   $40
        .BYTE   $02
        .BYTE   $45
        .BYTE   $33
        .BYTE   $D0
        .BYTE   $08
        .BYTE   $40
        .BYTE   $09
        .BYTE   $40
        .BYTE   $00
        .BYTE   $40
        .BYTE   $B0
        .BYTE   $D0
        .BYTE   $00
        .BYTE   $40
        .BYTE   $00
        .BYTE   $00
        .BYTE   $22
        .BYTE   $44
        .BYTE   $33
        .BYTE   $D0
        .BYTE   $8C
        .BYTE   $44
        .BYTE   $00
        .BYTE   $11
        .BYTE   $22
        .BYTE   $44
        .BYTE   $33
        .BYTE   $D0
        .BYTE   $8C
        .BYTE   $44
        .BYTE   $9A
        .BYTE   $10
        .BYTE   $22
        .BYTE   $44
        .BYTE   $33
        .BYTE   $D0
        .BYTE   $08
        .BYTE   $40
        .BYTE   $09
        .BYTE   $10
        .BYTE   $22
        .BYTE   $44
        .BYTE   $33
        .BYTE   $D0
        .BYTE   $08
        .BYTE   $40
        .BYTE   $09
        .BYTE   $62
        .BYTE   $13             ; YYXXXZ01 INSTRS.
        .BYTE   $78
        .BYTE   $A9
MODE2:  .BYTE   $00             ; ERR
        .BYTE   $21             ; IMM
        .BYTE   $81             ; Z-PAG
        .BYTE   $82             ; ABS
        .BYTE   $00             ; IMPL
        .BYTE   $00             ; ACC
        .BYTE   $59             ; (Z-PAG,X)
        .BYTE   $4D             ; (Z-PAG),Y
        .BYTE   $91             ; Z-PAG,X
        .BYTE   $92             ; ABS,X
        .BYTE   $86             ; ABS,Y
        .BYTE   $4A             ; (ABS)
        .BYTE   $85             ; Z-PAG,Y
        .BYTE   $9D             ; REL
CHAR1:  .BYTE   $AC             ; ','
        .BYTE   $A9             ; ')'
        .BYTE   $AC             ; ','
        .BYTE   $A3             ; '#'
        .BYTE   $A8             ; '('
        .BYTE   $A4             ; '$'
CHAR2:  .BYTE   $D9             ; 'Y'
        .BYTE   $00
        .BYTE   $D8             ; 'X'
        .BYTE   $A4             ; '$'
        .BYTE   $A4             ; '$'
        .BYTE   $00
MNEML:  .BYTE   $1C             ; XXXXX000 INSTRS.
        .BYTE   $8A
        .BYTE   $1C
        .BYTE   $23
        .BYTE   $5D
        .BYTE   $8B
        .BYTE   $1B
        .BYTE   $A1
        .BYTE   $9D
        .BYTE   $8A
        .BYTE   $1D
        .BYTE   $23
        .BYTE   $9D
        .BYTE   $8B
        .BYTE   $1D
        .BYTE   $A1
        .BYTE   $00
        .BYTE   $29
        .BYTE   $19
        .BYTE   $AE
        .BYTE   $69
        .BYTE   $A8
        .BYTE   $19
        .BYTE   $23
        .BYTE   $24
        .BYTE   $53
        .BYTE   $1B
        .BYTE   $23
        .BYTE   $24
        .BYTE   $53
        .BYTE   $19
        .BYTE   $A1
        .BYTE   $00             ; XXXYY100 INSTRS.
        .BYTE   $1A
        .BYTE   $5B
        .BYTE   $5B
        .BYTE   $A5
        .BYTE   $69
        .BYTE   $24
        .BYTE   $24
        .BYTE   $AE             ; 1XXX1010 INSTRS.
        .BYTE   $AE
        .BYTE   $A8
        .BYTE   $AD
        .BYTE   $29
        .BYTE   $00
        .BYTE   $7C
        .BYTE   $00
        .BYTE   $15             ; XXXYYY10 INSTRS.
        .BYTE   $9C
        .BYTE   $6D
        .BYTE   $00
        .BYTE   $A5
        .BYTE   $69
        .BYTE   $29
        .BYTE   $53
        .BYTE   $84             ; XXXYYY01 INSTRS.
        .BYTE   $13
        .BYTE   $34
        .BYTE   $11
        .BYTE   $A5
        .BYTE   $69
        .BYTE   $23
        .BYTE   $A0
MNEMR:  .BYTE   $D8             ; XXXXX000 INSTRS.
        .BYTE   $62
        .BYTE   $5A
        .BYTE   $48
        .BYTE   $26
        .BYTE   $62
        .BYTE   $94
        .BYTE   $88
        .BYTE   $54
        .BYTE   $44
        .BYTE   $C8
        .BYTE   $54
        .BYTE   $68
        .BYTE   $44
        .BYTE   $E8
        .BYTE   $94
        .BYTE   $00
        .BYTE   $B4
        .BYTE   $08
        .BYTE   $84
        .BYTE   $74
        .BYTE   $B4
        .BYTE   $28
        .BYTE   $6E
        .BYTE   $74
        .BYTE   $F4
        .BYTE   $CC
        .BYTE   $4A
        .BYTE   $72
        .BYTE   $F2
        .BYTE   $A4
        .BYTE   $8A
        .BYTE   $00             ; XXXYY100 INSTRS.
        .BYTE   $AA
        .BYTE   $A2
        .BYTE   $A2
        .BYTE   $74
        .BYTE   $74
        .BYTE   $74
        .BYTE   $72
        .BYTE   $44             ; 1XXX1010 INSTRS.
        .BYTE   $68
        .BYTE   $B2
        .BYTE   $32
        .BYTE   $B2
        .BYTE   $00
        .BYTE   $22
        .BYTE   $00
        .BYTE   $1A             ; XXXYYY10 INSTRS.
        .BYTE   $1A
        .BYTE   $26
        .BYTE   $00
        .BYTE   $72
        .BYTE   $72
        .BYTE   $88
        .BYTE   $C8
        .BYTE   $C4             ; XXXYYY01 INSTRS.
        .BYTE   $CA
        .BYTE   $26
        .BYTE   $48
        .BYTE   $44
        .BYTE   $44
        .BYTE   $A2
        .BYTE   $C8
