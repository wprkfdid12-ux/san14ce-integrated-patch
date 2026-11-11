
work/game_runtime.exe:     file format pei-x86-64


Disassembly of section .text:

000000014035a290 <.text+0x359290>:
   14035a290:	48 89 5c 24 08       	mov    QWORD PTR [rsp+0x8],rbx
   14035a295:	57                   	push   rdi
   14035a296:	48 83 ec 30          	sub    rsp,0x30
   14035a29a:	ba 03 00 00 00       	mov    edx,0x3
   14035a29f:	48 8b d9             	mov    rbx,rcx
   14035a2a2:	e8 e9 ee f4 ff       	call   0x1402a9190
   14035a2a7:	0f b6 bb 82 01 00 00 	movzx  edi,BYTE PTR [rbx+0x182]
   14035a2ae:	03 f8                	add    edi,eax
   14035a2b0:	e8 eb cc 09 00       	call   0x1403f6fa0
   14035a2b5:	48 8b c8             	mov    rcx,rax
   14035a2b8:	e8 c3 7e e7 ff       	call   0x1401d2180
   14035a2bd:	48 8b d8             	mov    rbx,rax
   14035a2c0:	b8 0a 00 00 00       	mov    eax,0xa
   14035a2c5:	3b f8                	cmp    edi,eax
   14035a2c7:	0f 4f f8             	cmovg  edi,eax
   14035a2ca:	e8 b1 b2 e3 ff       	call   0x140195580
   14035a2cf:	48 8b c8             	mov    rcx,rax
   14035a2d2:	89 7c 24 20          	mov    DWORD PTR [rsp+0x20],edi
   14035a2d6:	4c 8d 0d 23 96 05 01 	lea    r9,[rip+0x1059623]        # 0x1413b3900
   14035a2dd:	41 b8 00 10 00 00    	mov    r8d,0x1000
   14035a2e3:	48 8b d3             	mov    rdx,rbx
   14035a2e6:	e8 05 c0 e3 ff       	call   0x1401962f0
   14035a2eb:	48 8b c3             	mov    rax,rbx
   14035a2ee:	48 8b 5c 24 40       	mov    rbx,QWORD PTR [rsp+0x40]
   14035a2f3:	48 83 c4 30          	add    rsp,0x30
   14035a2f7:	5f                   	pop    rdi
   14035a2f8:	c3                   	ret
