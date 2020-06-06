	.module	bgdraw
	.globl	_bgDraw
XLIM	=	69
	; sp+2 work pointer1
	; sp+4 work pointer2
	; sp+6 dp
	; sp+8 n
	; sp+9 half
	; ret dp
	.area	_CODE
_bgDraw:
	push	ix
	ld	ix,#2
	add	ix,sp
	ld	l,2(ix)
	ld	h,3(ix)
	ld	(p0),hl
	ld	l,4(ix)
	ld	h,5(ix)
	ld	(p1),hl
	ld	l,6(ix)
	ld	h,7(ix)
	ld	(dp),hl
	ld	b,#25
7$:
	push	bc
	ld	hl,(p0)
	ld	e,(hl)
	inc	hl
	ld	d,(hl)	;de=p->p
	inc	hl
	ld	b,(hl)	;b=count
	inc	hl
	ld	c,8(ix)	;c=n
	ld	a,c
	or	a	;flag for jr
	ld	a,(hl)	;a=data
	ex	de,hl	;hl=p->p
	ld	de,(dp)
	jr	z,2$
1$:
	ld	(de),a
	inc	de
	dec	c
	jr	z,2$
	djnz	1$
	ld	b,(hl)
	inc	hl
	ld	a,(hl)
	inc	hl
	jr	1$
2$:
	push	de
	ld	hl,(p0)
	ld	a,9(ix)
	or	a
	jr	z,9$
	ld	e,(hl)
	inc	hl
	ld	d,(hl)	;de=p->p
	inc	hl
	ld	b,(hl)	;b=p->c
	djnz	6$
	ld	a,(de)
	inc	de
	ld	b,a	;b=count
	inc	hl
	ld	a,(de)	;a=data
	inc	de
	ld	(hl),a
	dec	hl
	dec	hl
	dec	hl
	ld	(hl),e
	inc	hl
	ld	(hl),d	;store p->p
	inc	hl
6$:
	ld	(hl),b
	jr	5$
9$:
	inc	hl
	inc	hl
5$:
	inc	hl
	inc	hl
	ld	(p0),hl
	ld	a,#XLIM
	sub	8(ix)	;flag for jr
	ld	c,a
	ld	hl,(p1)
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	inc	hl
	ld	b,(hl)	;b=count
	inc	hl
	ld	a,(hl)	;a=data
	inc	hl
	ld	(p1),hl
	ex	de,hl	;hl=source
	pop	de
	jr	z,4$
3$:
	ld	(de),a
	inc	de
	dec	c
	jr	z,4$
	djnz	3$
	ld	b,(hl)
	inc	hl
	ld	a,(hl)
	inc	hl
	jr	3$
4$:
	ld	hl,#120-XLIM
	add	hl,de
	ld	(dp),hl
	pop	bc
	dec	b
	jp	nz,7$
	ld	a,9(ix)
	or	a
	jr	z,8$
;_halfScroll:
	ld	l,6(ix)
	ld	h,7(ix)
	ld	de,#120-XLIM
	ld	c,#25
12$:
	ld	b,#XLIM
11$:
	inc	hl
	ld	a,(hl)
	dec	hl
	rrd
	inc	hl
	djnz	11$
	add	hl,de
	dec	c
	jr	nz,12$
8$:
	ld	l,6(ix)
	ld	h,7(ix)
	ld	de,#XLIM-1
	add	hl,de
	ld	e,#120
	ld	b,#25
	xor	a
10$:
	ld	(hl),a
	add	hl,de
	djnz	10$
	pop	ix
	ret
	.area	_DATA
p0:
	.ds	2
p1:
	.ds	2
dp:
	.ds	2
