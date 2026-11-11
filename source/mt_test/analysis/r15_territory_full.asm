
work/game_runtime.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140042880 <.text+0x41880>:
   140042880:	40 53                	rex push rbx
   140042882:	48 83 ec 20          	sub    rsp,0x20
   140042886:	ba 02 00 00 00       	mov    edx,0x2
   14004288b:	48 8b d9             	mov    rbx,rcx
   14004288e:	e8 fd 68 26 00       	call   0x1402a9190
   140042893:	33 d2                	xor    edx,edx
   140042895:	ff c0                	inc    eax
   140042897:	39 83 e8 01 00 00    	cmp    DWORD PTR [rbx+0x1e8],eax
   14004289d:	0f 9d c2             	setge  dl
   1400428a0:	8b c2                	mov    eax,edx
   1400428a2:	48 83 c4 20          	add    rsp,0x20
   1400428a6:	5b                   	pop    rbx
   1400428a7:	c3                   	ret
