
game_static_only.bin:     file format pei-x86-64


Disassembly of section .text:

00000001406d9127 <.text+0x6d8127>:
   1406d9127:	48 89 5c 24 40       	mov    QWORD PTR [rsp+0x40],rbx
   1406d912c:	49 8b cf             	mov    rcx,r15
   1406d912f:	48 89 6c 24 48       	mov    QWORD PTR [rsp+0x48],rbp
   1406d9134:	33 db                	xor    ebx,ebx
   1406d9136:	4c 89 74 24 50       	mov    QWORD PTR [rsp+0x50],r14
   1406d913b:	e8 a0 69 96 ff       	call   0x14003fae0
   1406d9140:	48 63 c8             	movsxd rcx,eax
   1406d9143:	85 c0                	test   eax,eax
   1406d9145:	7e 41                	jle    0x1406d9188
   1406d9147:	4c 8b 0d ca 4d 0a 02 	mov    r9,QWORD PTR [rip+0x20a4dca]        # 0x14277df18
   1406d914e:	4d 8d 81 b8 5d 08 00 	lea    r8,[r9+0x85db8]
   1406d9155:	4d 8d 04 c8          	lea    r8,[r8+rcx*8]
   1406d9159:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
   1406d9160:	48 83 f9 06          	cmp    rcx,0x6
   1406d9164:	7f 05                	jg     0x1406d916b
   1406d9166:	49 8b 10             	mov    rdx,QWORD PTR [r8]
   1406d9169:	eb 07                	jmp    0x1406d9172
   1406d916b:	49 8b 91 b8 5d 08 00 	mov    rdx,QWORD PTR [r9+0x85db8]
   1406d9172:	33 c0                	xor    eax,eax
   1406d9174:	39 42 30             	cmp    DWORD PTR [rdx+0x30],eax
   1406d9177:	0f 95 c0             	setne  al
   1406d917a:	48 ff c9             	dec    rcx
   1406d917d:	03 d8                	add    ebx,eax
   1406d917f:	49 83 e8 08          	sub    r8,0x8
   1406d9183:	48 85 c9             	test   rcx,rcx
   1406d9186:	7f d8                	jg     0x1406d9160
   1406d9188:	48 8b 8e 98 01 00 00 	mov    rcx,QWORD PTR [rsi+0x198]
   1406d918f:	40 32 ed             	xor    bpl,bpl
   1406d9192:	45 0f b7 47 10       	movzx  r8d,WORD PTR [r15+0x10]
   1406d9197:	45 32 f6             	xor    r14b,r14b
   1406d919a:	48 8b c1             	mov    rax,rcx
   1406d919d:	48 8b 51 08          	mov    rdx,QWORD PTR [rcx+0x8]
   1406d91a1:	40 38 6a 19          	cmp    BYTE PTR [rdx+0x19],bpl
   1406d91a5:	75 18                	jne    0x1406d91bf
   1406d91a7:	44 39 42 1c          	cmp    DWORD PTR [rdx+0x1c],r8d
   1406d91ab:	7d 06                	jge    0x1406d91b3
   1406d91ad:	48 8b 52 10          	mov    rdx,QWORD PTR [rdx+0x10]
   1406d91b1:	eb 06                	jmp    0x1406d91b9
   1406d91b3:	48 8b c2             	mov    rax,rdx
   1406d91b6:	48 8b 12             	mov    rdx,QWORD PTR [rdx]
   1406d91b9:	80 7a 19 00          	cmp    BYTE PTR [rdx+0x19],0x0
   1406d91bd:	74 e8                	je     0x1406d91a7
   1406d91bf:	48 3b c1             	cmp    rax,rcx
   1406d91c2:	74 06                	je     0x1406d91ca
   1406d91c4:	44 3b 40 1c          	cmp    r8d,DWORD PTR [rax+0x1c]
   1406d91c8:	7d 03                	jge    0x1406d91cd
   1406d91ca:	48 8b c1             	mov    rax,rcx
   1406d91cd:	48 3b c1             	cmp    rax,rcx
   1406d91d0:	74 27                	je     0x1406d91f9
   1406d91d2:	0f b6 48 20          	movzx  ecx,BYTE PTR [rax+0x20]
   1406d91d6:	0f b6 c1             	movzx  eax,cl
   1406d91d9:	24 0f                	and    al,0xf
   1406d91db:	88 44 24 58          	mov    BYTE PTR [rsp+0x58],al
   1406d91df:	c0 e9 04             	shr    cl,0x4
   1406d91e2:	88 4c 24 59          	mov    BYTE PTR [rsp+0x59],cl
   1406d91e6:	0f b7 44 24 58       	movzx  eax,WORD PTR [rsp+0x58]
   1406d91eb:	66 89 44 24 58       	mov    WORD PTR [rsp+0x58],ax
   1406d91f0:	0f b6 e8             	movzx  ebp,al
   1406d91f3:	44 0f b6 74 24 59    	movzx  r14d,BYTE PTR [rsp+0x59]
   1406d91f9:	41 0f b7 87 c2 01 00 	movzx  eax,WORD PTR [r15+0x1c2]
   1406d9200:	00 
   1406d9201:	33 d2                	xor    edx,edx
   1406d9203:	45 33 c9             	xor    r9d,r9d
   1406d9206:	66 89 44 24 58       	mov    WORD PTR [rsp+0x58],ax
   1406d920b:	4c 63 d3             	movsxd r10,ebx
   1406d920e:	85 db                	test   ebx,ebx
   1406d9210:	7e 49                	jle    0x1406d925b
   1406d9212:	0f b6 74 24 59       	movzx  esi,BYTE PTR [rsp+0x59]
   1406d9217:	44 0f b6 dd          	movzx  r11d,bpl
   1406d921b:	41 0f b6 de          	movzx  ebx,r14b
   1406d921f:	90                   	nop
   1406d9220:	45 33 c0             	xor    r8d,r8d
   1406d9223:	41 3b d3             	cmp    edx,r11d
   1406d9226:	7d 0c                	jge    0x1406d9234
   1406d9228:	0f b6 c8             	movzx  ecx,al
   1406d922b:	3b d1                	cmp    edx,ecx
   1406d922d:	41 0f 9c c0          	setl   r8b
   1406d9231:	41 ff c0             	inc    r8d
   1406d9234:	46 89 04 8f          	mov    DWORD PTR [rdi+r9*4],r8d
   1406d9238:	45 33 c0             	xor    r8d,r8d
   1406d923b:	3b d3                	cmp    edx,ebx
   1406d923d:	7d 0d                	jge    0x1406d924c
   1406d923f:	40 0f b6 ce          	movzx  ecx,sil
   1406d9243:	3b d1                	cmp    edx,ecx
   1406d9245:	41 0f 9c c0          	setl   r8b
   1406d9249:	41 ff c0             	inc    r8d
   1406d924c:	46 89 44 8f 0c       	mov    DWORD PTR [rdi+r9*4+0xc],r8d
   1406d9251:	ff c2                	inc    edx
   1406d9253:	49 ff c1             	inc    r9
   1406d9256:	4d 3b ca             	cmp    r9,r10
   1406d9259:	7c c5                	jl     0x1406d9220
   1406d925b:	49 8b cf             	mov    rcx,r15
   1406d925e:	33 db                	xor    ebx,ebx
   1406d9260:	e8 7b 68 96 ff       	call   0x14003fae0
   1406d9265:	48 63 c8             	movsxd rcx,eax
   1406d9268:	85 c0                	test   eax,eax
   1406d926a:	7e 3c                	jle    0x1406d92a8
   1406d926c:	4c 8b 0d a5 4c 0a 02 	mov    r9,QWORD PTR [rip+0x20a4ca5]        # 0x14277df18
   1406d9273:	4d 8d 81 b8 5d 08 00 	lea    r8,[r9+0x85db8]
   1406d927a:	4d 8d 04 c8          	lea    r8,[r8+rcx*8]
   1406d927e:	66 90                	xchg   ax,ax
   1406d9280:	48 83 f9 06          	cmp    rcx,0x6
   1406d9284:	7f 05                	jg     0x1406d928b
   1406d9286:	49 8b 10             	mov    rdx,QWORD PTR [r8]
   1406d9289:	eb 07                	jmp    0x1406d9292
   1406d928b:	49 8b 91 b8 5d 08 00 	mov    rdx,QWORD PTR [r9+0x85db8]
   1406d9292:	33 c0                	xor    eax,eax
   1406d9294:	39 42 30             	cmp    DWORD PTR [rdx+0x30],eax
   1406d9297:	0f 95 c0             	setne  al
   1406d929a:	48 ff c9             	dec    rcx
   1406d929d:	03 d8                	add    ebx,eax
   1406d929f:	49 83 e8 08          	sub    r8,0x8
   1406d92a3:	48 85 c9             	test   rcx,rcx
   1406d92a6:	7f d8                	jg     0x1406d9280
   1406d92a8:	41 0f b6 ce          	movzx  ecx,r14b
   1406d92ac:	40 0f b6 c5          	movzx  eax,bpl
   1406d92b0:	03 c8                	add    ecx,eax
   1406d92b2:	3b d9                	cmp    ebx,ecx
   1406d92b4:	48 8b 5c 24 40       	mov    rbx,QWORD PTR [rsp+0x40]
   1406d92b9:	7e 17                	jle    0x1406d92d2
