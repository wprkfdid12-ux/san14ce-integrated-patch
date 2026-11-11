
work/game_runtime.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140232b70 <.text+0x231b70>:
   140232b70:	44 0f b6 81 e6 00 00 	movzx  r8d,BYTE PTR [rcx+0xe6]
   140232b77:	00 
   140232b78:	33 c0                	xor    eax,eax
   140232b7a:	44 3b c2             	cmp    r8d,edx
   140232b7d:	0f 94 c0             	sete   al
   140232b80:	c3                   	ret
