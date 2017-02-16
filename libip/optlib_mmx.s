
; back in the day (2002), egcs (the C compiler of the day) generated
; awful code and the Pentium III cpus were slow enough to justify 
; hand-written assembly for trivial things like memcpy.
; Those days are gone and this file is now history.  -- F.Bergo, 2013
	
; requires NASM to be assembled
; the code written here requires an MMX-enabled Pentium-class CPU
; generic C fallbacks are in optlib.c

global MemCpy
global MemSet
global RgbSet
global RgbRect   ; void RgbRect(void *dest, int x,int y,int w,int h,int rgb,
	         ;              int rowoffset)
global FloatFill
	
section .text

MemCpy:
	push ebp
	mov  ebp, esp
	mov  [ebp-16], ebx
	mov  [ebp-12], ecx
	mov  [ebp-8],  esi
	mov  [ebp-4],  edi
		
	mov ebx, [ebp+16]
	mov ecx, ebx

	mov edi, [ebp+8]
	mov esi, [ebp+12]

	and ebx, 0111b ; remainder
	shr ecx, 3     ; # of quadwords

	cmp ecx, 0
	jz  .loop1
.loop2: movq mm0,   [esi]
	add  esi, 8
	movq [edi],   mm0
	add  edi, 8
	dec  ecx
	jnz .loop2
.loop1: 
	cmp  ebx, 0
	jz   .epil1
	mov  ecx, ebx
	cld
	rep  movsb
.epil1:
	emms
	mov ebx, [ebp-16]
	mov ecx, [ebp-12]
	mov esi, [ebp-8]
	mov edi, [ebp-4]
	leave
	ret

MemSet:
	push ebp
	mov  ebp, esp

	mov [ebp-4],  edx
	mov [ebp-8],  ecx
	mov [ebp-12], ebx
	mov [ebp-16], eax

	mov edx, [ebp+8]

	mov eax, [ebp+12]  ;  00 00 00 XX
	mov ah,  al        ;  00 00 XX XX
	mov ecx, eax       ; (00 00 XX XX)
	shl ecx, 16        ; (XX XX 00 00)
	or  eax, ecx       ;  XX XX XX XX

	mov [ebp-20], eax 
	mov [ebp-24], eax ; ebp-24 = 8x{c}

	mov ecx, [ebp+16]
	mov ebx, ecx
	shr ecx, 3
	and ebx, 0111b

	cmp ecx, 0
	jz  .loop1
	movq mm0, [ebp-24]
.loop2: 
	movq [edx], mm0
	add  edx, 8
	dec  ecx
	jnz .loop2
.loop1:
	cmp ebx, 0
	jz .epil1
	mov eax, [ebp-20]
.loop3:
	mov [edx], al
	inc edx
	dec ebx
	jnz .loop3	
.epil1:
	emms
	mov eax, [ebp-16]
	mov ebx, [ebp-12]
	mov ecx, [ebp-8]
	mov edx, [ebp-4]
	leave
	ret

RgbSet:	
	push ebp
	mov  ebp, esp
;	sub  esp, 40
	mov [ebp-4],  edi
	mov [ebp-8],  ecx
	mov [ebp-12], edx
	mov [ebp-16], eax
	mov [ebp-20], ebx
	mov [ebp-24], esi
	; ebp-27 : ebp-25 : free. ebp-28: dirty pad byte
	; ebp-40 : ebp-29 = 12-byte RGB pattern to copy

	mov edi, [ebp+8]  ; destination ptr
	mov eax, [ebp+12] ; RGB color
	mov ecx, [ebp+16] ; triplet count

	; convert eax from XXRRGGBB to 00BBGGRR
	mov ebx, eax
	mov edx, eax
	and eax, 0x0000ff00
	and ebx, 0x000000ff
	and edx, 0x00ff0000
	shl ebx, 16
	shr edx, 16
	or  eax, ebx
	or  eax, edx

	mov [ebp-40], eax
	mov [ebp-37], eax
	mov [ebp-34], eax
	mov [ebp-31], eax

	; calc iterations and remainder
	mov ebx, ecx
	shr ecx, 2     ; number of 12-byte patterns to copy (count div 4)
	and ebx, 0011b ; remainder

	cmp ecx, 0
	jz  .loop1
	movq mm0, [ebp-40]
	movd mm1, [ebp-32]
	
.loop2:
	movq [edi],   mm0
	movd [edi+8], mm1
	add edi, 12
	dec ecx
	jnz .loop2

.loop1:
	cmp ebx, 0
	jz  .epil1
	mov ecx, ebx
	lea ecx, [ecx+ecx*2]  ; ecx := 3 * ecx
	mov esi, ebp
	sub esi, 40
	cld
	rep movsb

.epil1:
	emms
	mov esi, [ebp-24]
	mov ebx, [ebp-20]
	mov eax, [ebp-16]
	mov edx, [ebp-12]
	mov ecx, [ebp-8]
	mov edi, [ebp-4]
	leave
	ret

FloatFill:
	push ebp
	mov  ebp, esp
;	sub esp, 
	mov  [ebp-4],  ebx
	mov  [ebp-8],  ecx
	mov  [ebp-12], edx

	mov edx, [ebp+8]  ; dest
	mov ecx, [ebp+16] ; count
	mov ebx, ecx

	movd mm0, [ebp+12]
	movd mm1, [ebp+12]
	punpckldq mm0, mm1
	
	shr ecx, 1
	cmp ecx, 0
	jz  .loop1
.loop2:
	movq [edx], mm0
	add  edx, 8
	dec  ecx
	jnz .loop2	
.loop1:
	and  ebx, 1
	jz   .epil1
	movd [edx], mm0
.epil1:
	emms
	mov edx, [ebp-12]
	mov ecx, [ebp-8]
	mov ebx, [ebp-4]
	leave
	ret

;global RgbRect ; void *dest, int x, y, w, h, rgb, rowoffset
;calls MMX-based RgbSet and MemCpy, so it requires MMX too
RgbRect:
	push ebp
	mov  ebp, esp
	sub  esp, 16
	mov  [ebp-4],  eax
	mov  [ebp-8],  ebx
	mov  [ebp-12], ecx
	mov  [ebp-16], edx

	; does not check bounds of x,y,w,h!
	
	mov  eax, [ebp+16]    ; y
	mov  edx, [ebp+32]    ; rowoffset
	mul  edx              ; eax := y*rowoffset
	mov  edx, [ebp+12]    ; x
	lea  edx, [edx+edx*2] ; 3 * x
	add  edx, eax         ; y*rowoffset + 3 *x
	mov  eax, [ebp+8]     ; dest
	add  edx, eax         ; edx := dest[y*rowoffset + 3*x]

	mov  eax, [ebp+20]    ; w
	push eax
	mov  eax, [ebp+28]    ; rgb
	push eax
	push edx              ; dest
	call RgbSet
	add  esp, 12
	
	mov  ecx, [ebp+24]    ; h
	dec  ecx
	js    .epil1
	jecxz .epil1
	mov  eax, [ebp+32]    ; rowoffset
	mov  ebx, [ebp+20]    ; w
	lea  ebx, [ebx+ebx*2] ; w*3
	push ebx              ; count
	push edx              ; src
	add  edx, eax         
	push edx              ; dest
.loop1
	call MemCpy
	mov  edx, [esp]
	mov  [esp+4], edx
	add  edx, eax
	mov  [esp], edx
	dec  ecx
	jnz .loop1
	
	add esp, 12
.epil1:
	mov  eax, [ebp-4]
	mov  ebx, [ebp-8]
	mov  ecx, [ebp-12]
	mov  edx, [ebp-16]
	leave
	ret
