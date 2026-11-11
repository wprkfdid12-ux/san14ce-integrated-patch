
work/game_runtime.exe:     file format pei-x86-64


Disassembly of section .text:

0000000140226d54 <.text+0x225d54>:
   140226d54:	e8 b7 ad 01 00       	call   0x140241b10
   140226d59:	48 8d 8b d0 01 00 00 	lea    rcx,[rbx+0x1d0]
   140226d60:	89 b3 b8 01 00 00    	mov    DWORD PTR [rbx+0x1b8],esi
   140226d66:	40 88 b3 bc 01 00 00 	mov    BYTE PTR [rbx+0x1bc],sil
   140226d6d:	0f b7 c6             	movzx  eax,si
   140226d70:	66 89 83 c2 01 00 00 	mov    WORD PTR [rbx+0x1c2],ax
   140226d77:	ba 03 00 00 00       	mov    edx,0x3
   140226d7c:	89 b3 be 01 00 00    	mov    DWORD PTR [rbx+0x1be],esi
   140226d82:	40 88 b3 c4 01 00 00 	mov    BYTE PTR [rbx+0x1c4],sil
   140226d89:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   140226d8c:	48 89 41 08          	mov    QWORD PTR [rcx+0x8],rax
   140226d90:	66 89 74 24 30       	mov    WORD PTR [rsp+0x30],si
   140226d95:	e8 76 ac 01 00       	call   0x140241a10
   140226d9a:	89 b3 e8 01 00 00    	mov    DWORD PTR [rbx+0x1e8],esi
