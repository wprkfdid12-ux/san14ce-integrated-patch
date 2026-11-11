
game_static_only.bin:     file format pei-x86-64


Disassembly of section .text:

0000000140770080 <.text+0x76f080>:
   140770080:	40 53                	rex push rbx
   140770082:	55                   	push   rbp
   140770083:	56                   	push   rsi
   140770084:	57                   	push   rdi
   140770085:	48 83 ec 38          	sub    rsp,0x38
   140770089:	48 8b da             	mov    rbx,rdx
   14077008c:	48 8b e9             	mov    rbp,rcx
   14077008f:	48 8b 0d 32 26 06 02 	mov    rcx,QWORD PTR [rip+0x2062632]        # 0x1427d26c8
   140770096:	33 d2                	xor    edx,edx
   140770098:	0f 57 db             	xorps  xmm3,xmm3
   14077009b:	0f 57 d2             	xorps  xmm2,xmm2
   14077009e:	e8 bd 28 c9 ff       	call   0x140402960
   1407700a3:	8b 83 80 00 00 00    	mov    eax,DWORD PTR [rbx+0x80]
   1407700a9:	48 8d 8d 30 01 00 00 	lea    rcx,[rbp+0x130]
   1407700b0:	48 8d 54 24 68       	lea    rdx,[rsp+0x68]
   1407700b5:	89 44 24 68          	mov    DWORD PTR [rsp+0x68],eax
   1407700b9:	e8 52 b5 1b 00       	call   0x14092b610
   1407700be:	48 63 d0             	movsxd rdx,eax
   1407700c1:	45 32 c9             	xor    r9b,r9b
   1407700c4:	40 32 ff             	xor    dil,dil
   1407700c7:	48 8b 84 d5 f8 01 00 	mov    rax,QWORD PTR [rbp+rdx*8+0x1f8]
   1407700ce:	00 
   1407700cf:	48 8b 88 78 01 00 00 	mov    rcx,QWORD PTR [rax+0x178]
   1407700d6:	44 0f b7 59 10       	movzx  r11d,WORD PTR [rcx+0x10]
   1407700db:	48 8b 8d 98 01 00 00 	mov    rcx,QWORD PTR [rbp+0x198]
   1407700e2:	48 8b c1             	mov    rax,rcx
   1407700e5:	48 8b 51 08          	mov    rdx,QWORD PTR [rcx+0x8]
   1407700e9:	40 38 7a 19          	cmp    BYTE PTR [rdx+0x19],dil
   1407700ed:	75 19                	jne    0x140770108
   1407700ef:	90                   	nop
   1407700f0:	44 39 5a 1c          	cmp    DWORD PTR [rdx+0x1c],r11d
   1407700f4:	7d 06                	jge    0x1407700fc
   1407700f6:	48 8b 52 10          	mov    rdx,QWORD PTR [rdx+0x10]
   1407700fa:	eb 06                	jmp    0x140770102
   1407700fc:	48 8b c2             	mov    rax,rdx
   1407700ff:	48 8b 12             	mov    rdx,QWORD PTR [rdx]
   140770102:	80 7a 19 00          	cmp    BYTE PTR [rdx+0x19],0x0
   140770106:	74 e8                	je     0x1407700f0
   140770108:	48 3b c1             	cmp    rax,rcx
   14077010b:	74 06                	je     0x140770113
   14077010d:	44 3b 58 1c          	cmp    r11d,DWORD PTR [rax+0x1c]
   140770111:	7d 03                	jge    0x140770116
   140770113:	48 8b c1             	mov    rax,rcx
   140770116:	48 3b c1             	cmp    rax,rcx
   140770119:	74 27                	je     0x140770142
   14077011b:	0f b6 48 20          	movzx  ecx,BYTE PTR [rax+0x20]
   14077011f:	0f b6 c1             	movzx  eax,cl
   140770122:	24 0f                	and    al,0xf
   140770124:	88 44 24 60          	mov    BYTE PTR [rsp+0x60],al
   140770128:	c0 e9 04             	shr    cl,0x4
   14077012b:	88 4c 24 61          	mov    BYTE PTR [rsp+0x61],cl
   14077012f:	0f b7 44 24 60       	movzx  eax,WORD PTR [rsp+0x60]
   140770134:	66 89 44 24 60       	mov    WORD PTR [rsp+0x60],ax
   140770139:	44 0f b6 c8          	movzx  r9d,al
   14077013d:	0f b6 7c 24 61       	movzx  edi,BYTE PTR [rsp+0x61]
   140770142:	8b 5c 24 68          	mov    ebx,DWORD PTR [rsp+0x68]
   140770146:	4c 8d 15 93 70 d0 00 	lea    r10,[rip+0xd07093]        # 0x1414771e0
   14077014d:	45 33 c0             	xor    r8d,r8d
   140770150:	41 3b 1a             	cmp    ebx,DWORD PTR [r10]
   140770153:	75 1d                	jne    0x140770172
   140770155:	41 8d 50 01          	lea    edx,[r8+0x1]
   140770159:	41 0f b6 c9          	movzx  ecx,r9b
   14077015d:	ff c1                	inc    ecx
   14077015f:	0f b6 c2             	movzx  eax,dl
   140770162:	3b c8                	cmp    ecx,eax
   140770164:	75 06                	jne    0x14077016c
   140770166:	44 0f b6 ca          	movzx  r9d,dl
   14077016a:	eb 25                	jmp    0x140770191
   14077016c:	44 8d 4a ff          	lea    r9d,[rdx-0x1]
   140770170:	eb 1f                	jmp    0x140770191
   140770172:	41 3b 5a 0c          	cmp    ebx,DWORD PTR [r10+0xc]
   140770176:	75 19                	jne    0x140770191
   140770178:	41 8d 50 01          	lea    edx,[r8+0x1]
   14077017c:	40 0f b6 cf          	movzx  ecx,dil
   140770180:	ff c1                	inc    ecx
   140770182:	0f b6 c2             	movzx  eax,dl
   140770185:	3b c8                	cmp    ecx,eax
   140770187:	75 05                	jne    0x14077018e
   140770189:	0f b6 fa             	movzx  edi,dl
   14077018c:	eb 03                	jmp    0x140770191
   14077018e:	8d 7a ff             	lea    edi,[rdx-0x1]
   140770191:	41 ff c0             	inc    r8d
   140770194:	49 83 c2 04          	add    r10,0x4
   140770198:	41 83 f8 03          	cmp    r8d,0x3
   14077019c:	7c b2                	jl     0x140770150
   14077019e:	48 8b 85 98 01 00 00 	mov    rax,QWORD PTR [rbp+0x198]
   1407701a5:	44 89 5c 24 70       	mov    DWORD PTR [rsp+0x70],r11d
   1407701aa:	48 8b d8             	mov    rbx,rax
   1407701ad:	40 c0 e7 04          	shl    dil,0x4
   1407701b1:	41 02 f9             	add    dil,r9b
   1407701b4:	48 8b 48 08          	mov    rcx,QWORD PTR [rax+0x8]
   1407701b8:	80 79 19 00          	cmp    BYTE PTR [rcx+0x19],0x0
   1407701bc:	75 1a                	jne    0x1407701d8
   1407701be:	66 90                	xchg   ax,ax
   1407701c0:	44 39 59 1c          	cmp    DWORD PTR [rcx+0x1c],r11d
   1407701c4:	7d 06                	jge    0x1407701cc
   1407701c6:	48 8b 49 10          	mov    rcx,QWORD PTR [rcx+0x10]
   1407701ca:	eb 06                	jmp    0x1407701d2
   1407701cc:	48 8b d9             	mov    rbx,rcx
   1407701cf:	48 8b 09             	mov    rcx,QWORD PTR [rcx]
   1407701d2:	80 79 19 00          	cmp    BYTE PTR [rcx+0x19],0x0
   1407701d6:	74 e8                	je     0x1407701c0
   1407701d8:	48 3b d8             	cmp    rbx,rax
   1407701db:	74 06                	je     0x1407701e3
   1407701dd:	44 3b 5b 1c          	cmp    r11d,DWORD PTR [rbx+0x1c]
   1407701e1:	7d 49                	jge    0x14077022c
   1407701e3:	48 8d 44 24 70       	lea    rax,[rsp+0x70]
   1407701e8:	4c 8d 4c 24 60       	lea    r9,[rsp+0x60]
   1407701ed:	48 89 44 24 78       	mov    QWORD PTR [rsp+0x78],rax
   1407701f2:	4c 8d 44 24 78       	lea    r8,[rsp+0x78]
   1407701f7:	48 8d 15 e2 11 cc 00 	lea    rdx,[rip+0xcc11e2]        # 0x1414313e0
   1407701fe:	48 8d 8d 98 01 00 00 	lea    rcx,[rbp+0x198]
   140770205:	e8 a6 03 8b ff       	call   0x1400205b0
   14077020a:	4c 8b c3             	mov    r8,rbx
   14077020d:	48 89 44 24 20       	mov    QWORD PTR [rsp+0x20],rax
   140770212:	48 8d 54 24 60       	lea    rdx,[rsp+0x60]
   140770217:	48 8d 8d 98 01 00 00 	lea    rcx,[rbp+0x198]
   14077021e:	4c 8d 48 1c          	lea    r9,[rax+0x1c]
   140770222:	e8 79 07 8b ff       	call   0x1400209a0
   140770227:	48 8b 5c 24 60       	mov    rbx,QWORD PTR [rsp+0x60]
   14077022c:	40 0f b6 c7          	movzx  eax,dil
   140770230:	48 8b cd             	mov    rcx,rbp
   140770233:	89 43 20             	mov    DWORD PTR [rbx+0x20],eax
   140770236:	e8 d5 15 01 00       	call   0x140781810
   14077023b:	48 83 c4 38          	add    rsp,0x38
   14077023f:	5f                   	pop    rdi
   140770240:	5e                   	pop    rsi
   140770241:	5d                   	pop    rbp
   140770242:	5b                   	pop    rbx
   140770243:	c3                   	ret
   140770244:	cc                   	int3
   140770245:	cc                   	int3
   140770246:	cc                   	int3
   140770247:	cc                   	int3
   140770248:	cc                   	int3
   140770249:	cc                   	int3
   14077024a:	cc                   	int3
   14077024b:	cc                   	int3
   14077024c:	cc                   	int3
   14077024d:	cc                   	int3
   14077024e:	cc                   	int3
   14077024f:	cc                   	int3
   140770250:	48 89 5c 24 10       	mov    QWORD PTR [rsp+0x10],rbx
   140770255:	57                   	push   rdi
   140770256:	48 83 ec 20          	sub    rsp,0x20
   14077025a:	48 8b f9             	mov    rdi,rcx
   14077025d:	ba 03 00 00 00       	mov    edx,0x3
   140770262:	48 8b 89 90 04 00 00 	mov    rcx,QWORD PTR [rcx+0x490]
   140770269:	48 81 c1 30 01 00 00 	add    rcx,0x130
   140770270:	e8 1b c1 1b 00       	call   0x14092c390
   140770275:	48 8b c8             	mov    rcx,rax
   140770278:	8b 90 80 00 00 00    	mov    edx,DWORD PTR [rax+0x80]
   14077027e:	e8 cd a8 0f 00       	call   0x14086ab50
   140770283:	48 8b c8             	mov    rcx,rax
   140770286:	48 8b d8             	mov    rbx,rax
   140770289:	e8 32 e2 bb ff       	call   0x14032e4c0
   14077028e:	85 c0                	test   eax,eax
   140770290:	0f 84 1a 01 00 00    	je     0x1407703b0
   140770296:	0f b7 43 12          	movzx  eax,WORD PTR [rbx+0x12]
   14077029a:	48 89 74 24 30       	mov    QWORD PTR [rsp+0x30],rsi
   14077029f:	3d 70 17 00 00       	cmp    eax,0x1770
   1407702a4:	77 11                	ja     0x1407702b7
   1407702a6:	48 8b 0d 6b dc 00 02 	mov    rcx,QWORD PTR [rip+0x200dc6b]        # 0x14277df18
   1407702ad:	48 8b b4 c1 58 01 00 	mov    rsi,QWORD PTR [rcx+rax*8+0x158]
   1407702b4:	00 
   1407702b5:	eb 0e                	jmp    0x1407702c5
   1407702b7:	48 8b 05 5a dc 00 02 	mov    rax,QWORD PTR [rip+0x200dc5a]        # 0x14277df18
   1407702be:	48 8b b0 50 3a 07 00 	mov    rsi,QWORD PTR [rax+0x73a50]
   1407702c5:	48 8b 97 70 04 00 00 	mov    rdx,QWORD PTR [rdi+0x470]
   1407702cc:	45 33 c0             	xor    r8d,r8d
   1407702cf:	48 8b cb             	mov    rcx,rbx
   1407702d2:	e8 69 df b2 ff       	call   0x14029e240
   1407702d7:	b8 fe ff 00 00       	mov    eax,0xfffe
   1407702dc:	66 21 86 5e 01 00 00 	and    WORD PTR [rsi+0x15e],ax
   1407702e3:	80 7b 10 02          	cmp    BYTE PTR [rbx+0x10],0x2
   1407702e7:	75 2f                	jne    0x140770318
   1407702e9:	48 8b cb             	mov    rcx,rbx
   1407702ec:	e8 bf ba ab ff       	call   0x14022bdb0
   1407702f1:	48 8b c8             	mov    rcx,rax
   1407702f4:	e8 d7 29 ac ff       	call   0x140232cd0
   1407702f9:	85 c0                	test   eax,eax
   1407702fb:	74 1b                	je     0x140770318
   1407702fd:	48 8b 8f 70 04 00 00 	mov    rcx,QWORD PTR [rdi+0x470]
   140770304:	66 83 79 4e 33       	cmp    WORD PTR [rcx+0x4e],0x33
   140770309:	77 0d                	ja     0x140770318
   14077030b:	e8 f0 99 ab ff       	call   0x140229d00
   140770310:	48 8b c8             	mov    rcx,rax
   140770313:	e8 88 cf b8 ff       	call   0x1402fd2a0
   140770318:	e8 13 d5 b1 ff       	call   0x14028d830
   14077031d:	48 8b 8f 70 04 00 00 	mov    rcx,QWORD PTR [rdi+0x470]
   140770324:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   140770327:	ff 90 80 00 00 00    	call   QWORD PTR [rax+0x80]
   14077032d:	48 8b c8             	mov    rcx,rax
   140770330:	48 8b f0             	mov    rsi,rax
   140770333:	e8 88 e1 bb ff       	call   0x14032e4c0
   140770338:	85 c0                	test   eax,eax
   14077033a:	74 67                	je     0x1407703a3
   14077033c:	48 8b 8f 70 04 00 00 	mov    rcx,QWORD PTR [rdi+0x470]
   140770343:	0f b6 5e 14          	movzx  ebx,BYTE PTR [rsi+0x14]
   140770347:	ff c3                	inc    ebx
   140770349:	48 8b 11             	mov    rdx,QWORD PTR [rcx]
   14077034c:	ff 92 80 00 00 00    	call   QWORD PTR [rdx+0x80]
   140770352:	48 8b c8             	mov    rcx,rax
   140770355:	e8 16 86 b6 ff       	call   0x1402d8970
   14077035a:	3b d8                	cmp    ebx,eax
   14077035c:	0f 4c c3             	cmovl  eax,ebx
   14077035f:	85 c0                	test   eax,eax
   140770361:	79 04                	jns    0x140770367
   140770363:	33 c0                	xor    eax,eax
   140770365:	eb 0a                	jmp    0x140770371
   140770367:	b9 c8 00 00 00       	mov    ecx,0xc8
   14077036c:	3b c1                	cmp    eax,ecx
   14077036e:	0f 4f c1             	cmovg  eax,ecx
   140770371:	88 46 14             	mov    BYTE PTR [rsi+0x14],al
   140770374:	e8 97 2b a6 ff       	call   0x1401d2f10
   140770379:	48 85 c0             	test   rax,rax
   14077037c:	74 25                	je     0x1407703a3
   14077037e:	e8                   	.byte 0xe8
   14077037f:	8d                   	.byte 0x8d
