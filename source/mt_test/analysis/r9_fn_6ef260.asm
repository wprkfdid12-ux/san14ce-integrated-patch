
game_static_only.bin:     file format pei-x86-64


Disassembly of section .text:

00000001406ef260 <.text+0x6ee260>:
   1406ef260:	48 8b c4             	mov    rax,rsp
   1406ef263:	55                   	push   rbp
   1406ef264:	41 54                	push   r12
   1406ef266:	41 55                	push   r13
   1406ef268:	41 56                	push   r14
   1406ef26a:	41 57                	push   r15
   1406ef26c:	48 8d a8 d8 f7 ff ff 	lea    rbp,[rax-0x828]
   1406ef273:	48 81 ec 00 09 00 00 	sub    rsp,0x900
   1406ef27a:	48 c7 45 e0 fe ff ff 	mov    QWORD PTR [rbp-0x20],0xfffffffffffffffe
   1406ef281:	ff 
   1406ef282:	48 89 58 10          	mov    QWORD PTR [rax+0x10],rbx
   1406ef286:	48 89 70 18          	mov    QWORD PTR [rax+0x18],rsi
   1406ef28a:	48 89 78 20          	mov    QWORD PTR [rax+0x20],rdi
   1406ef28e:	48 8b 05 fb c8 39 01 	mov    rax,QWORD PTR [rip+0x139c8fb]        # 0x141a8bb90
   1406ef295:	48 33 c4             	xor    rax,rsp
   1406ef298:	48 89 85 f0 07 00 00 	mov    QWORD PTR [rbp+0x7f0],rax
   1406ef29f:	48 8b fa             	mov    rdi,rdx
   1406ef2a2:	48 8b f1             	mov    rsi,rcx
   1406ef2a5:	48 89 4c 24 48       	mov    QWORD PTR [rsp+0x48],rcx
   1406ef2aa:	45 33 f6             	xor    r14d,r14d
   1406ef2ad:	4c 89 74 24 30       	mov    QWORD PTR [rsp+0x30],r14
   1406ef2b2:	44 89 74 24 28       	mov    DWORD PTR [rsp+0x28],r14d
   1406ef2b7:	48 8d 05 7a 8b d5 00 	lea    rax,[rip+0xd58b7a]        # 0x141447e38
   1406ef2be:	48 89 44 24 20       	mov    QWORD PTR [rsp+0x20],rax
   1406ef2c3:	45 33 c9             	xor    r9d,r9d
   1406ef2c6:	ba 4e 01 00 00       	mov    edx,0x14e
   1406ef2cb:	41 b8 b8 00 00 00    	mov    r8d,0xb8
   1406ef2d1:	e8 ca e6 17 00       	call   0x14086d9a0
   1406ef2d6:	48 89 be b8 01 00 00 	mov    QWORD PTR [rsi+0x1b8],rdi
   1406ef2dd:	48 8b cf             	mov    rcx,rdi
   1406ef2e0:	e8 1b d6 b3 ff       	call   0x14022c900
   1406ef2e5:	48 89 86 b0 01 00 00 	mov    QWORD PTR [rsi+0x1b0],rax
   1406ef2ec:	48 8d 9e 30 01 00 00 	lea    rbx,[rsi+0x130]
   1406ef2f3:	41 8d 56 04          	lea    edx,[r14+0x4]
   1406ef2f7:	45 8d 46 0d          	lea    r8d,[r14+0xd]
   1406ef2fb:	48 8b cb             	mov    rcx,rbx
   1406ef2fe:	e8 cd f0 23 00       	call   0x14092e3d0
   1406ef303:	b9 42 05 00 00       	mov    ecx,0x542
   1406ef308:	e8 43 cc c1 ff       	call   0x14030bf50
   1406ef30d:	4c 8b c0             	mov    r8,rax
   1406ef310:	41 8d 56 04          	lea    edx,[r14+0x4]
   1406ef314:	45 8d 4e 01          	lea    r9d,[r14+0x1]
   1406ef318:	48 8b cb             	mov    rcx,rbx
   1406ef31b:	e8 c0 0e 24 00       	call   0x1409301e0
   1406ef320:	44 89 74 24 28       	mov    DWORD PTR [rsp+0x28],r14d
   1406ef325:	c7 44 24 20 01 00 00 	mov    DWORD PTR [rsp+0x20],0x1
   1406ef32c:	00 
   1406ef32d:	45 33 c9             	xor    r9d,r9d
   1406ef330:	41 8d 56 08          	lea    edx,[r14+0x8]
   1406ef334:	45 8d 46 03          	lea    r8d,[r14+0x3]
   1406ef338:	48 8b cb             	mov    rcx,rbx
   1406ef33b:	e8 b0 e5 23 00       	call   0x14092d8f0
   1406ef340:	48 89 86 e0 01 00 00 	mov    QWORD PTR [rsi+0x1e0],rax
   1406ef347:	48 8b 38             	mov    rdi,QWORD PTR [rax]
   1406ef34a:	b9 fe 03 00 00       	mov    ecx,0x3fe
   1406ef34f:	e8 fc cb c1 ff       	call   0x14030bf50
   1406ef354:	48 8b d0             	mov    rdx,rax
   1406ef357:	48 8b 8e e0 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1e0]
   1406ef35e:	ff 97 d8 01 00 00    	call   QWORD PTR [rdi+0x1d8]
   1406ef364:	48 8d 05 ed 1c d6 00 	lea    rax,[rip+0xd61ced]        # 0x141451058
   1406ef36b:	48 89 44 24 50       	mov    QWORD PTR [rsp+0x50],rax
   1406ef370:	48 89 74 24 58       	mov    QWORD PTR [rsp+0x58],rsi
   1406ef375:	48 8d 44 24 50       	lea    rax,[rsp+0x50]
   1406ef37a:	48 89 45 88          	mov    QWORD PTR [rbp-0x78],rax
   1406ef37e:	4c 8d 44 24 50       	lea    r8,[rsp+0x50]
   1406ef383:	ba fe ff ff 7f       	mov    edx,0x7ffffffe
   1406ef388:	48 8b 8e e0 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1e0]
   1406ef38f:	e8 6c 3f ab ff       	call   0x1401a3300
   1406ef394:	48 8b 86 e0 01 00 00 	mov    rax,QWORD PTR [rsi+0x1e0]
   1406ef39b:	c7 80 84 00 00 00 01 	mov    DWORD PTR [rax+0x84],0x1
   1406ef3a2:	00 00 00 
   1406ef3a5:	c7 80 80 00 00 00 02 	mov    DWORD PTR [rax+0x80],0x2
   1406ef3ac:	00 00 00 
   1406ef3af:	41 8d 56 11          	lea    edx,[r14+0x11]
   1406ef3b3:	45 8d 46 0d          	lea    r8d,[r14+0xd]
   1406ef3b7:	48 8b cb             	mov    rcx,rbx
   1406ef3ba:	e8 11 f0 23 00       	call   0x14092e3d0
   1406ef3bf:	b9 85 06 00 00       	mov    ecx,0x685
   1406ef3c4:	e8 87 cb c1 ff       	call   0x14030bf50
   1406ef3c9:	4c 8b c0             	mov    r8,rax
   1406ef3cc:	41 8d 56 11          	lea    edx,[r14+0x11]
   1406ef3d0:	45 8d 4e 01          	lea    r9d,[r14+0x1]
   1406ef3d4:	48 8b cb             	mov    rcx,rbx
   1406ef3d7:	e8 04 0e 24 00       	call   0x1409301e0
   1406ef3dc:	45 33 c0             	xor    r8d,r8d
   1406ef3df:	41 8d 56 13          	lea    edx,[r14+0x13]
   1406ef3e3:	48 8b cb             	mov    rcx,rbx
   1406ef3e6:	e8 e5 ef 23 00       	call   0x14092e3d0
   1406ef3eb:	45 8d 4e 01          	lea    r9d,[r14+0x1]
   1406ef3ef:	4c 8b 05 a2 87 35 01 	mov    r8,QWORD PTR [rip+0x13587a2]        # 0x141a47b98
   1406ef3f6:	41 8d 56 13          	lea    edx,[r14+0x13]
   1406ef3fa:	48 8b cb             	mov    rcx,rbx
   1406ef3fd:	e8 de 0d 24 00       	call   0x1409301e0
   1406ef402:	b9 48 02 00 00       	mov    ecx,0x248
   1406ef407:	e8 a4 67 d0 ff       	call   0x1403f5bb0
   1406ef40c:	48 89 44 24 40       	mov    QWORD PTR [rsp+0x40],rax
   1406ef411:	48 85 c0             	test   rax,rax
   1406ef414:	74 0d                	je     0x1406ef423
   1406ef416:	48 8b c8             	mov    rcx,rax
   1406ef419:	e8 f2 ab 1a 00       	call   0x14089a010
   1406ef41e:	48 8b c8             	mov    rcx,rax
   1406ef421:	eb 03                	jmp    0x1406ef426
   1406ef423:	49 8b ce             	mov    rcx,r14
   1406ef426:	48 89 8e f8 01 00 00 	mov    QWORD PTR [rsi+0x1f8],rcx
   1406ef42d:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406ef430:	48 89 74 24 28       	mov    QWORD PTR [rsp+0x28],rsi
   1406ef435:	44 89 74 24 20       	mov    DWORD PTR [rsp+0x20],r14d
   1406ef43a:	45 33 c9             	xor    r9d,r9d
   1406ef43d:	45 33 c0             	xor    r8d,r8d
   1406ef440:	33 d2                	xor    edx,edx
   1406ef442:	ff 90 90 01 00 00    	call   QWORD PTR [rax+0x190]
   1406ef448:	44 89 74 24 28       	mov    DWORD PTR [rsp+0x28],r14d
   1406ef44d:	c7 44 24 20 01 00 00 	mov    DWORD PTR [rsp+0x20],0x1
   1406ef454:	00 
   1406ef455:	41 b9 08 00 00 00    	mov    r9d,0x8
   1406ef45b:	4c 8b 86 f8 01 00 00 	mov    r8,QWORD PTR [rsi+0x1f8]
   1406ef462:	41 8d 51 0c          	lea    edx,[r9+0xc]
   1406ef466:	48 8b cb             	mov    rcx,rbx
   1406ef469:	e8 02 e1 23 00       	call   0x14092d570
   1406ef46e:	b9 48 02 00 00       	mov    ecx,0x248
   1406ef473:	e8 38 67 d0 ff       	call   0x1403f5bb0
   1406ef478:	48 89 44 24 40       	mov    QWORD PTR [rsp+0x40],rax
   1406ef47d:	48 85 c0             	test   rax,rax
   1406ef480:	74 0d                	je     0x1406ef48f
   1406ef482:	48 8b c8             	mov    rcx,rax
   1406ef485:	e8 86 ab 1a 00       	call   0x14089a010
   1406ef48a:	48 8b c8             	mov    rcx,rax
   1406ef48d:	eb 03                	jmp    0x1406ef492
   1406ef48f:	49 8b ce             	mov    rcx,r14
   1406ef492:	48 89 8e 00 02 00 00 	mov    QWORD PTR [rsi+0x200],rcx
   1406ef499:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406ef49c:	48 89 74 24 28       	mov    QWORD PTR [rsp+0x28],rsi
   1406ef4a1:	44 89 74 24 20       	mov    DWORD PTR [rsp+0x20],r14d
   1406ef4a6:	45 33 c9             	xor    r9d,r9d
   1406ef4a9:	45 33 c0             	xor    r8d,r8d
   1406ef4ac:	41 8d 51 01          	lea    edx,[r9+0x1]
   1406ef4b0:	ff 90 90 01 00 00    	call   QWORD PTR [rax+0x190]
   1406ef4b6:	44 89 74 24 28       	mov    DWORD PTR [rsp+0x28],r14d
   1406ef4bb:	c7 44 24 20 01 00 00 	mov    DWORD PTR [rsp+0x20],0x1
   1406ef4c2:	00 
   1406ef4c3:	41 b9 08 00 00 00    	mov    r9d,0x8
   1406ef4c9:	4c 8b 86 00 02 00 00 	mov    r8,QWORD PTR [rsi+0x200]
   1406ef4d0:	41 8d 51 0d          	lea    edx,[r9+0xd]
   1406ef4d4:	48 8b cb             	mov    rcx,rbx
   1406ef4d7:	e8 94 e0 23 00       	call   0x14092d570
   1406ef4dc:	b9 98 01 00 00       	mov    ecx,0x198
   1406ef4e1:	e8 ca 66 d0 ff       	call   0x1403f5bb0
   1406ef4e6:	48 89 44 24 40       	mov    QWORD PTR [rsp+0x40],rax
   1406ef4eb:	48 85 c0             	test   rax,rax
   1406ef4ee:	74 0d                	je     0x1406ef4fd
   1406ef4f0:	48 8b c8             	mov    rcx,rax
   1406ef4f3:	e8 98 ae 15 00       	call   0x14084a390
   1406ef4f8:	48 8b c8             	mov    rcx,rax
   1406ef4fb:	eb 03                	jmp    0x1406ef500
   1406ef4fd:	49 8b ce             	mov    rcx,r14
   1406ef500:	48 89 8e e8 01 00 00 	mov    QWORD PTR [rsi+0x1e8],rcx
   1406ef507:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406ef50a:	c7 44 24 20 10 00 00 	mov    DWORD PTR [rsp+0x20],0x10
   1406ef511:	00 
   1406ef512:	4c 8b ce             	mov    r9,rsi
   1406ef515:	45 33 c0             	xor    r8d,r8d
   1406ef518:	33 d2                	xor    edx,edx
   1406ef51a:	ff 90 90 01 00 00    	call   QWORD PTR [rax+0x190]
   1406ef520:	48 8d 05 69 1b d6 00 	lea    rax,[rip+0xd61b69]        # 0x141451090
   1406ef527:	48 89 44 24 50       	mov    QWORD PTR [rsp+0x50],rax
   1406ef52c:	48 89 74 24 58       	mov    QWORD PTR [rsp+0x58],rsi
   1406ef531:	48 8d 44 24 50       	lea    rax,[rsp+0x50]
   1406ef536:	48 89 45 88          	mov    QWORD PTR [rbp-0x78],rax
   1406ef53a:	4c 8d 44 24 50       	lea    r8,[rsp+0x50]
   1406ef53f:	ba fe ff ff 7f       	mov    edx,0x7ffffffe
   1406ef544:	48 8b 8e e8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1e8]
   1406ef54b:	e8 b0 3d ab ff       	call   0x1401a3300
   1406ef550:	44 89 74 24 28       	mov    DWORD PTR [rsp+0x28],r14d
   1406ef555:	c7 44 24 20 01 00 00 	mov    DWORD PTR [rsp+0x20],0x1
   1406ef55c:	00 
   1406ef55d:	41 b9 04 00 00 00    	mov    r9d,0x4
   1406ef563:	4c 8b 86 e8 01 00 00 	mov    r8,QWORD PTR [rsi+0x1e8]
   1406ef56a:	41 8d 51 1c          	lea    edx,[r9+0x1c]
   1406ef56e:	48 8b cb             	mov    rcx,rbx
   1406ef571:	e8 fa df 23 00       	call   0x14092d570
   1406ef576:	b9 98 01 00 00       	mov    ecx,0x198
   1406ef57b:	e8 30 66 d0 ff       	call   0x1403f5bb0
   1406ef580:	48 89 44 24 40       	mov    QWORD PTR [rsp+0x40],rax
   1406ef585:	48 85 c0             	test   rax,rax
   1406ef588:	74 0d                	je     0x1406ef597
   1406ef58a:	48 8b c8             	mov    rcx,rax
   1406ef58d:	e8 fe ad 15 00       	call   0x14084a390
   1406ef592:	48 8b c8             	mov    rcx,rax
   1406ef595:	eb 03                	jmp    0x1406ef59a
   1406ef597:	49 8b ce             	mov    rcx,r14
   1406ef59a:	48 89 8e f0 01 00 00 	mov    QWORD PTR [rsi+0x1f0],rcx
   1406ef5a1:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406ef5a4:	c7 44 24 20 11 00 00 	mov    DWORD PTR [rsp+0x20],0x11
   1406ef5ab:	00 
   1406ef5ac:	4c 8b ce             	mov    r9,rsi
   1406ef5af:	45 33 c0             	xor    r8d,r8d
   1406ef5b2:	33 d2                	xor    edx,edx
   1406ef5b4:	ff 90 90 01 00 00    	call   QWORD PTR [rax+0x190]
   1406ef5ba:	48 8d 05 07 1b d6 00 	lea    rax,[rip+0xd61b07]        # 0x1414510c8
   1406ef5c1:	48 89 44 24 50       	mov    QWORD PTR [rsp+0x50],rax
   1406ef5c6:	48 89 74 24 58       	mov    QWORD PTR [rsp+0x58],rsi
   1406ef5cb:	48 8d 44 24 50       	lea    rax,[rsp+0x50]
   1406ef5d0:	48 89 45 88          	mov    QWORD PTR [rbp-0x78],rax
   1406ef5d4:	4c 8d 44 24 50       	lea    r8,[rsp+0x50]
   1406ef5d9:	ba fe ff ff 7f       	mov    edx,0x7ffffffe
   1406ef5de:	48 8b 8e f0 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1f0]
   1406ef5e5:	e8 16 3d ab ff       	call   0x1401a3300
   1406ef5ea:	44 89 74 24 28       	mov    DWORD PTR [rsp+0x28],r14d
   1406ef5ef:	c7 44 24 20 01 00 00 	mov    DWORD PTR [rsp+0x20],0x1
   1406ef5f6:	00 
   1406ef5f7:	41 b9 04 00 00 00    	mov    r9d,0x4
   1406ef5fd:	4c 8b 86 f0 01 00 00 	mov    r8,QWORD PTR [rsi+0x1f0]
   1406ef604:	41 8d 51 20          	lea    edx,[r9+0x20]
   1406ef608:	48 8b cb             	mov    rcx,rbx
   1406ef60b:	e8 60 df 23 00       	call   0x14092d570
   1406ef610:	c7 44 24 20 fe 03 00 	mov    DWORD PTR [rsp+0x20],0x3fe
   1406ef617:	00 
   1406ef618:	ba 16 00 00 00       	mov    edx,0x16
   1406ef61d:	41 b9 c3 03 00 00    	mov    r9d,0x3c3
   1406ef623:	44 8d 42 ff          	lea    r8d,[rdx-0x1]
   1406ef627:	48 8b cb             	mov    rcx,rbx
   1406ef62a:	e8 f1 e4 23 00       	call   0x14092db20
   1406ef62f:	c7 44 24 20 fe 03 00 	mov    DWORD PTR [rsp+0x20],0x3fe
   1406ef636:	00 
   1406ef637:	ba 19 00 00 00       	mov    edx,0x19
   1406ef63c:	41 b9 c9 03 00 00    	mov    r9d,0x3c9
   1406ef642:	44 8d 42 fc          	lea    r8d,[rdx-0x4]
   1406ef646:	48 8b cb             	mov    rcx,rbx
   1406ef649:	e8 d2 e4 23 00       	call   0x14092db20
   1406ef64e:	45 33 c0             	xor    r8d,r8d
   1406ef651:	41 8d 50 1d          	lea    edx,[r8+0x1d]
   1406ef655:	48 8b cb             	mov    rcx,rbx
   1406ef658:	e8 73 ed 23 00       	call   0x14092e3d0
   1406ef65d:	45 33 c0             	xor    r8d,r8d
   1406ef660:	41 8d 50 1f          	lea    edx,[r8+0x1f]
   1406ef664:	48 8b cb             	mov    rcx,rbx
   1406ef667:	e8 64 ed 23 00       	call   0x14092e3d0
   1406ef66c:	45 33 c0             	xor    r8d,r8d
   1406ef66f:	41 8d 50 1e          	lea    edx,[r8+0x1e]
   1406ef673:	48 8b cb             	mov    rcx,rbx
   1406ef676:	e8 55 ed 23 00       	call   0x14092e3d0
   1406ef67b:	33 d2                	xor    edx,edx
   1406ef67d:	41 b8 00 08 00 00    	mov    r8d,0x800
   1406ef683:	48 8d 4d f0          	lea    rcx,[rbp-0x10]
   1406ef687:	e8 14 ba 92 00       	call   0x14101b0a0
   1406ef68c:	48 8d 7d f0          	lea    rdi,[rbp-0x10]
   1406ef690:	33 c0                	xor    eax,eax
   1406ef692:	b9 00 08 00 00       	mov    ecx,0x800
   1406ef697:	f3 aa                	rep stos BYTE PTR es:[rdi],al
   1406ef699:	8b 15 81 1c 10 02    	mov    edx,DWORD PTR [rip+0x2101c81]        # 0x1427f1320
   1406ef69f:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   1406ef6a6:	00 00 
   1406ef6a8:	b9 10 00 00 00       	mov    ecx,0x10
   1406ef6ad:	48 8b 04 d0          	mov    rax,QWORD PTR [rax+rdx*8]
   1406ef6b1:	48 03 c1             	add    rax,rcx
   1406ef6b4:	48 89 45 98          	mov    QWORD PTR [rbp-0x68],rax
   1406ef6b8:	8b 00                	mov    eax,DWORD PTR [rax]
   1406ef6ba:	39 05 a8 38 a5 01    	cmp    DWORD PTR [rip+0x1a538a8],eax        # 0x142142f68
   1406ef6c0:	7e 2d                	jle    0x1406ef6ef
   1406ef6c2:	48 8d 0d 9f 38 a5 01 	lea    rcx,[rip+0x1a5389f]        # 0x142142f68
   1406ef6c9:	e8 be 9a 90 00       	call   0x140ff918c
   1406ef6ce:	83 3d 93 38 a5 01 ff 	cmp    DWORD PTR [rip+0x1a53893],0xffffffff        # 0x142142f68
   1406ef6d5:	75 18                	jne    0x1406ef6ef
   1406ef6d7:	48 8d 0d f2 c8 c4 00 	lea    rcx,[rip+0xc4c8f2]        # 0x14133bfd0
   1406ef6de:	e8 b9 98 90 00       	call   0x140ff8f9c
   1406ef6e3:	48 8d 0d 7e 38 a5 01 	lea    rcx,[rip+0x1a5387e]        # 0x142142f68
   1406ef6ea:	e8 3d 9a 90 00       	call   0x140ff912c
   1406ef6ef:	c7 44 24 20 01 00 00 	mov    DWORD PTR [rsp+0x20],0x1
   1406ef6f6:	00 
   1406ef6f7:	4c 8d 0d d6 70 ca 00 	lea    r9,[rip+0xca70d6]        # 0x1413967d4
   1406ef6fe:	41 b8 00 08 00 00    	mov    r8d,0x800
   1406ef704:	48 8d 55 f0          	lea    rdx,[rbp-0x10]
   1406ef708:	48 8d 0d 56 38 a5 01 	lea    rcx,[rip+0x1a53856]        # 0x142142f65
   1406ef70f:	e8 dc 6b aa ff       	call   0x1401962f0
   1406ef714:	41 b9 01 00 00 00    	mov    r9d,0x1
   1406ef71a:	4c 8d 45 f0          	lea    r8,[rbp-0x10]
   1406ef71e:	41 8d 51 1c          	lea    edx,[r9+0x1c]
   1406ef722:	48 8b cb             	mov    rcx,rbx
   1406ef725:	e8 b6 0a 24 00       	call   0x1409301e0
   1406ef72a:	41 b9 01 00 00 00    	mov    r9d,0x1
   1406ef730:	4c 8d 45 f0          	lea    r8,[rbp-0x10]
   1406ef734:	41 8d 51 1e          	lea    edx,[r9+0x1e]
   1406ef738:	48 8b cb             	mov    rcx,rbx
   1406ef73b:	e8 a0 0a 24 00       	call   0x1409301e0
   1406ef740:	b9 0c 06 00 00       	mov    ecx,0x60c
   1406ef745:	e8 06 c8 c1 ff       	call   0x14030bf50
   1406ef74a:	4c 8b c0             	mov    r8,rax
   1406ef74d:	ba 1e 00 00 00       	mov    edx,0x1e
   1406ef752:	44 8d 4a e3          	lea    r9d,[rdx-0x1d]
   1406ef756:	48 8b cb             	mov    rcx,rbx
   1406ef759:	e8 82 0a 24 00       	call   0x1409301e0
   1406ef75e:	e8 3d 78 d0 ff       	call   0x1403f6fa0
   1406ef763:	48 8b c8             	mov    rcx,rax
   1406ef766:	e8 15 2a ae ff       	call   0x1401d2180
   1406ef76b:	4c 8b e0             	mov    r12,rax
   1406ef76e:	ba 2c 00 00 00       	mov    edx,0x2c
   1406ef773:	44 8d 42 e1          	lea    r8d,[rdx-0x1f]
   1406ef777:	48 8b cb             	mov    rcx,rbx
   1406ef77a:	e8 51 ec 23 00       	call   0x14092e3d0
   1406ef77f:	b9 1f 08 00 00       	mov    ecx,0x81f
   1406ef784:	e8 c7 c7 c1 ff       	call   0x14030bf50
   1406ef789:	4c 8b c0             	mov    r8,rax
   1406ef78c:	ba 2c 00 00 00       	mov    edx,0x2c
   1406ef791:	44 8d 4a d5          	lea    r9d,[rdx-0x2b]
   1406ef795:	48 8b cb             	mov    rcx,rbx
   1406ef798:	e8 43 0a 24 00       	call   0x1409301e0
   1406ef79d:	48 8b 8e b8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1b8]
   1406ef7a4:	0f b6 41 10          	movzx  eax,BYTE PTR [rcx+0x10]
   1406ef7a8:	83 f8 33             	cmp    eax,0x33
   1406ef7ab:	77 13                	ja     0x1406ef7c0
   1406ef7ad:	8b c8                	mov    ecx,eax
   1406ef7af:	48 8b 05 62 e7 08 02 	mov    rax,QWORD PTR [rip+0x208e762]        # 0x14277df18
   1406ef7b6:	48 8b 94 c8 30 df 00 	mov    rdx,QWORD PTR [rax+rcx*8+0xdf30]
   1406ef7bd:	00 
   1406ef7be:	eb 0e                	jmp    0x1406ef7ce
   1406ef7c0:	48 8b 05 51 e7 08 02 	mov    rax,QWORD PTR [rip+0x208e751]        # 0x14277df18
   1406ef7c7:	48 8b 90 30 df 00 00 	mov    rdx,QWORD PTR [rax+0xdf30]
   1406ef7ce:	4c 8d aa c4 01 00 00 	lea    r13,[rdx+0x1c4]
   1406ef7d5:	4c 89 6c 24 40       	mov    QWORD PTR [rsp+0x40],r13
   1406ef7da:	41 be 01 00 00 00    	mov    r14d,0x1
   1406ef7e0:	48 8d 3d 0d 1c d4 00 	lea    rdi,[rip+0xd41c0d]        # 0x1414313f4
   1406ef7e7:	41 bf 00 8d 07 00    	mov    r15d,0x78d00
   1406ef7ed:	48 c7 45 90 06 00 00 	mov    QWORD PTR [rbp-0x70],0x6
   1406ef7f4:	00 
   1406ef7f5:	48 8b 75 98          	mov    rsi,QWORD PTR [rbp-0x68]
   1406ef7f9:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
   1406ef800:	41 b8 0d 00 00 00    	mov    r8d,0xd
   1406ef806:	8b 57 fc             	mov    edx,DWORD PTR [rdi-0x4]
   1406ef809:	48 8b cb             	mov    rcx,rbx
   1406ef80c:	e8 bf eb 23 00       	call   0x14092e3d0
   1406ef811:	48 8b 05 00 e7 08 02 	mov    rax,QWORD PTR [rip+0x208e700]        # 0x14277df18
   1406ef818:	49 83 fe 06          	cmp    r14,0x6
   1406ef81c:	7f 06                	jg     0x1406ef824
   1406ef81e:	4d 8b 04 07          	mov    r8,QWORD PTR [r15+rax*1]
   1406ef822:	eb 07                	jmp    0x1406ef82b
   1406ef824:	4c 8b 80 f8 8c 07 00 	mov    r8,QWORD PTR [rax+0x78cf8]
   1406ef82b:	49 83 c0 10          	add    r8,0x10
   1406ef82f:	41 b9 01 00 00 00    	mov    r9d,0x1
   1406ef835:	8b 57 fc             	mov    edx,DWORD PTR [rdi-0x4]
   1406ef838:	48 8b cb             	mov    rcx,rbx
   1406ef83b:	e8 a0 09 24 00       	call   0x1409301e0
   1406ef840:	45 33 c0             	xor    r8d,r8d
   1406ef843:	8b 17                	mov    edx,DWORD PTR [rdi]
   1406ef845:	48 8b cb             	mov    rcx,rbx
   1406ef848:	e8 83 eb 23 00       	call   0x14092e3d0
   1406ef84d:	47 0f b6 2c 2e       	movzx  r13d,BYTE PTR [r14+r13*1]
   1406ef852:	8b 06                	mov    eax,DWORD PTR [rsi]
   1406ef854:	39 05 0e 37 a5 01    	cmp    DWORD PTR [rip+0x1a5370e],eax        # 0x142142f68
   1406ef85a:	7e 2d                	jle    0x1406ef889
   1406ef85c:	48 8d 0d 05 37 a5 01 	lea    rcx,[rip+0x1a53705]        # 0x142142f68
   1406ef863:	e8 24 99 90 00       	call   0x140ff918c
   1406ef868:	83 3d f9 36 a5 01 ff 	cmp    DWORD PTR [rip+0x1a536f9],0xffffffff        # 0x142142f68
   1406ef86f:	75 18                	jne    0x1406ef889
   1406ef871:	48 8d 0d 58 c7 c4 00 	lea    rcx,[rip+0xc4c758]        # 0x14133bfd0
   1406ef878:	e8 1f 97 90 00       	call   0x140ff8f9c
   1406ef87d:	48 8d 0d e4 36 a5 01 	lea    rcx,[rip+0x1a536e4]        # 0x142142f68
   1406ef884:	e8 a3 98 90 00       	call   0x140ff912c
   1406ef889:	41 0f b6 c5          	movzx  eax,r13b
   1406ef88d:	89 44 24 20          	mov    DWORD PTR [rsp+0x20],eax
   1406ef891:	4c 8d 0d 3c 6f ca 00 	lea    r9,[rip+0xca6f3c]        # 0x1413967d4
   1406ef898:	41 b8 00 10 00 00    	mov    r8d,0x1000
   1406ef89e:	49 8b d4             	mov    rdx,r12
   1406ef8a1:	48 8d 0d bd 36 a5 01 	lea    rcx,[rip+0x1a536bd]        # 0x142142f65
   1406ef8a8:	e8 43 6a aa ff       	call   0x1401962f0
   1406ef8ad:	41 b9 01 00 00 00    	mov    r9d,0x1
   1406ef8b3:	4d 8b c4             	mov    r8,r12
   1406ef8b6:	8b 17                	mov    edx,DWORD PTR [rdi]
   1406ef8b8:	48 8b cb             	mov    rcx,rbx
   1406ef8bb:	e8 20 09 24 00       	call   0x1409301e0
   1406ef8c0:	41 b8 0f 00 00 00    	mov    r8d,0xf
   1406ef8c6:	8b 57 08             	mov    edx,DWORD PTR [rdi+0x8]
   1406ef8c9:	48 8b cb             	mov    rcx,rbx
   1406ef8cc:	e8 ff ea 23 00       	call   0x14092e3d0
   1406ef8d1:	41 b9 01 00 00 00    	mov    r9d,0x1
   1406ef8d7:	4c 8b 05 ba 82 35 01 	mov    r8,QWORD PTR [rip+0x13582ba]        # 0x141a47b98
   1406ef8de:	8b 57 08             	mov    edx,DWORD PTR [rdi+0x8]
   1406ef8e1:	48 8b cb             	mov    rcx,rbx
   1406ef8e4:	e8 f7 08 24 00       	call   0x1409301e0
   1406ef8e9:	49 83 c7 08          	add    r15,0x8
   1406ef8ed:	49 ff c6             	inc    r14
   1406ef8f0:	48 83 c7 10          	add    rdi,0x10
   1406ef8f4:	48 83 6d 90 01       	sub    QWORD PTR [rbp-0x70],0x1
   1406ef8f9:	4c 8b 6c 24 40       	mov    r13,QWORD PTR [rsp+0x40]
   1406ef8fe:	0f 85 fc fe ff ff    	jne    0x1406ef800
   1406ef904:	ba 50 00 00 00       	mov    edx,0x50
   1406ef909:	44 8d 42 bd          	lea    r8d,[rdx-0x43]
   1406ef90d:	48 8b cb             	mov    rcx,rbx
   1406ef910:	e8 bb ea 23 00       	call   0x14092e3d0
   1406ef915:	b9 0c 02 00 00       	mov    ecx,0x20c
   1406ef91a:	e8 31 c6 c1 ff       	call   0x14030bf50
   1406ef91f:	4c 8b c0             	mov    r8,rax
   1406ef922:	ba 50 00 00 00       	mov    edx,0x50
   1406ef927:	44 8d 4a b1          	lea    r9d,[rdx-0x4f]
   1406ef92b:	48 8b cb             	mov    rcx,rbx
   1406ef92e:	e8 ad 08 24 00       	call   0x1409301e0
   1406ef933:	45 33 c0             	xor    r8d,r8d
   1406ef936:	41 8d 50 52          	lea    edx,[r8+0x52]
   1406ef93a:	48 8b cb             	mov    rcx,rbx
   1406ef93d:	e8 8e ea 23 00       	call   0x14092e3d0
   1406ef942:	45 33 c0             	xor    r8d,r8d
   1406ef945:	41 8d 50 54          	lea    edx,[r8+0x54]
   1406ef949:	48 8b cb             	mov    rcx,rbx
   1406ef94c:	e8 7f ea 23 00       	call   0x14092e3d0
   1406ef951:	45 33 c0             	xor    r8d,r8d
   1406ef954:	41 8d 50 55          	lea    edx,[r8+0x55]
   1406ef958:	48 8b cb             	mov    rcx,rbx
   1406ef95b:	e8 70 ea 23 00       	call   0x14092e3d0
   1406ef960:	45 33 c0             	xor    r8d,r8d
   1406ef963:	41 8d 50 56          	lea    edx,[r8+0x56]
   1406ef967:	48 8b cb             	mov    rcx,rbx
   1406ef96a:	e8 61 ea 23 00       	call   0x14092e3d0
   1406ef96f:	4c 8b fe             	mov    r15,rsi
   1406ef972:	8b 06                	mov    eax,DWORD PTR [rsi]
   1406ef974:	39 05 ee 35 a5 01    	cmp    DWORD PTR [rip+0x1a535ee],eax        # 0x142142f68
   1406ef97a:	48 8b 74 24 48       	mov    rsi,QWORD PTR [rsp+0x48]
   1406ef97f:	7e 2d                	jle    0x1406ef9ae
   1406ef981:	48 8d 0d e0 35 a5 01 	lea    rcx,[rip+0x1a535e0]        # 0x142142f68
   1406ef988:	e8 ff 97 90 00       	call   0x140ff918c
   1406ef98d:	83 3d d4 35 a5 01 ff 	cmp    DWORD PTR [rip+0x1a535d4],0xffffffff        # 0x142142f68
   1406ef994:	75 18                	jne    0x1406ef9ae
   1406ef996:	48 8d 0d 33 c6 c4 00 	lea    rcx,[rip+0xc4c633]        # 0x14133bfd0
   1406ef99d:	e8 fa 95 90 00       	call   0x140ff8f9c
   1406ef9a2:	48 8d 0d bf 35 a5 01 	lea    rcx,[rip+0x1a535bf]        # 0x142142f68
   1406ef9a9:	e8 7e 97 90 00       	call   0x140ff912c
   1406ef9ae:	48 8b 8e b0 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1b0]
   1406ef9b5:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406ef9b8:	ff 90 90 00 00 00    	call   QWORD PTR [rax+0x90]
   1406ef9be:	89 44 24 20          	mov    DWORD PTR [rsp+0x20],eax
   1406ef9c2:	4c 8d 0d 0b 6e ca 00 	lea    r9,[rip+0xca6e0b]        # 0x1413967d4
   1406ef9c9:	41 b8 00 10 00 00    	mov    r8d,0x1000
   1406ef9cf:	49 8b d4             	mov    rdx,r12
   1406ef9d2:	48 8d 0d 8c 35 a5 01 	lea    rcx,[rip+0x1a5358c]        # 0x142142f65
   1406ef9d9:	e8 12 69 aa ff       	call   0x1401962f0
   1406ef9de:	41 b9 01 00 00 00    	mov    r9d,0x1
   1406ef9e4:	4d 8b c4             	mov    r8,r12
   1406ef9e7:	41 8d 51 51          	lea    edx,[r9+0x51]
   1406ef9eb:	48 8b cb             	mov    rcx,rbx
   1406ef9ee:	e8 ed 07 24 00       	call   0x1409301e0
   1406ef9f3:	41 b9 01 00 00 00    	mov    r9d,0x1
   1406ef9f9:	4d 8b c4             	mov    r8,r12
   1406ef9fc:	41 8d 51 53          	lea    edx,[r9+0x53]
   1406efa00:	48 8b cb             	mov    rcx,rbx
   1406efa03:	e8 d8 07 24 00       	call   0x1409301e0
   1406efa08:	41 8b 07             	mov    eax,DWORD PTR [r15]
   1406efa0b:	39 05 57 35 a5 01    	cmp    DWORD PTR [rip+0x1a53557],eax        # 0x142142f68
   1406efa11:	7e 2d                	jle    0x1406efa40
   1406efa13:	48 8d 0d 4e 35 a5 01 	lea    rcx,[rip+0x1a5354e]        # 0x142142f68
   1406efa1a:	e8 6d 97 90 00       	call   0x140ff918c
   1406efa1f:	83 3d 42 35 a5 01 ff 	cmp    DWORD PTR [rip+0x1a53542],0xffffffff        # 0x142142f68
   1406efa26:	75 18                	jne    0x1406efa40
   1406efa28:	48 8d 0d a1 c5 c4 00 	lea    rcx,[rip+0xc4c5a1]        # 0x14133bfd0
   1406efa2f:	e8 68 95 90 00       	call   0x140ff8f9c
   1406efa34:	48 8d 0d 2d 35 a5 01 	lea    rcx,[rip+0x1a5352d]        # 0x142142f68
   1406efa3b:	e8 ec 96 90 00       	call   0x140ff912c
   1406efa40:	48 c7 44 24 20 00 00 	mov    QWORD PTR [rsp+0x20],0x0
   1406efa47:	00 00 
   1406efa49:	4c 8d 0d 70 08 d5 00 	lea    r9,[rip+0xd50870]        # 0x1414402c0
   1406efa50:	41 b8 00 10 00 00    	mov    r8d,0x1000
   1406efa56:	49 8b d4             	mov    rdx,r12
   1406efa59:	48 8d 0d 05 35 a5 01 	lea    rcx,[rip+0x1a53505]        # 0x142142f65
   1406efa60:	e8 8b 68 aa ff       	call   0x1401962f0
   1406efa65:	41 b9 01 00 00 00    	mov    r9d,0x1
   1406efa6b:	4d 8b c4             	mov    r8,r12
   1406efa6e:	41 8d 51 54          	lea    edx,[r9+0x54]
   1406efa72:	48 8b cb             	mov    rcx,rbx
   1406efa75:	e8 66 07 24 00       	call   0x1409301e0
   1406efa7a:	33 d2                	xor    edx,edx
   1406efa7c:	41 b8 00 08 00 00    	mov    r8d,0x800
   1406efa82:	48 8d 4d f0          	lea    rcx,[rbp-0x10]
   1406efa86:	e8 15 b6 92 00       	call   0x14101b0a0
   1406efa8b:	48 8d 7d f0          	lea    rdi,[rbp-0x10]
   1406efa8f:	33 c0                	xor    eax,eax
   1406efa91:	b9 00 08 00 00       	mov    ecx,0x800
   1406efa96:	f3 aa                	rep stos BYTE PTR es:[rdi],al
   1406efa98:	41 8b 07             	mov    eax,DWORD PTR [r15]
   1406efa9b:	39 05 c7 34 a5 01    	cmp    DWORD PTR [rip+0x1a534c7],eax        # 0x142142f68
   1406efaa1:	7e 2d                	jle    0x1406efad0
   1406efaa3:	48 8d 0d be 34 a5 01 	lea    rcx,[rip+0x1a534be]        # 0x142142f68
   1406efaaa:	e8 dd 96 90 00       	call   0x140ff918c
   1406efaaf:	83 3d b2 34 a5 01 ff 	cmp    DWORD PTR [rip+0x1a534b2],0xffffffff        # 0x142142f68
   1406efab6:	75 18                	jne    0x1406efad0
   1406efab8:	48 8d 0d 11 c5 c4 00 	lea    rcx,[rip+0xc4c511]        # 0x14133bfd0
   1406efabf:	e8 d8 94 90 00       	call   0x140ff8f9c
   1406efac4:	48 8d 0d 9d 34 a5 01 	lea    rcx,[rip+0x1a5349d]        # 0x142142f68
   1406efacb:	e8 5c 96 90 00       	call   0x140ff912c
   1406efad0:	b9 ab 03 00 00       	mov    ecx,0x3ab
   1406efad5:	e8 76 c4 c1 ff       	call   0x14030bf50
   1406efada:	4c 8b c8             	mov    r9,rax
   1406efadd:	8b 05 65 8d 35 01    	mov    eax,DWORD PTR [rip+0x1358d65]        # 0x141a48848
   1406efae3:	89 44 24 28          	mov    DWORD PTR [rsp+0x28],eax
   1406efae7:	c7 44 24 20 01 00 00 	mov    DWORD PTR [rsp+0x20],0x1
   1406efaee:	00 
   1406efaef:	41 b8 00 08 00 00    	mov    r8d,0x800
   1406efaf5:	48 8d 55 f0          	lea    rdx,[rbp-0x10]
   1406efaf9:	48 8d 0d 65 34 a5 01 	lea    rcx,[rip+0x1a53465]        # 0x142142f65
   1406efb00:	e8 eb 67 aa ff       	call   0x1401962f0
   1406efb05:	41 b9 01 00 00 00    	mov    r9d,0x1
   1406efb0b:	4c 8d 45 f0          	lea    r8,[rbp-0x10]
   1406efb0f:	41 8d 51 55          	lea    edx,[r9+0x55]
   1406efb13:	48 8b cb             	mov    rcx,rbx
   1406efb16:	e8 c5 06 24 00       	call   0x1409301e0
   1406efb1b:	ba 5a 00 00 00       	mov    edx,0x5a
   1406efb20:	44 8d 42 b3          	lea    r8d,[rdx-0x4d]
   1406efb24:	48 8b cb             	mov    rcx,rbx
   1406efb27:	e8 a4 e8 23 00       	call   0x14092e3d0
   1406efb2c:	b9 92 02 00 00       	mov    ecx,0x292
   1406efb31:	e8 1a c4 c1 ff       	call   0x14030bf50
   1406efb36:	4c 8b c0             	mov    r8,rax
   1406efb39:	ba 5a 00 00 00       	mov    edx,0x5a
   1406efb3e:	44 8d 4a a7          	lea    r9d,[rdx-0x59]
   1406efb42:	48 8b cb             	mov    rcx,rbx
   1406efb45:	e8 96 06 24 00       	call   0x1409301e0
   1406efb4a:	45 33 c0             	xor    r8d,r8d
   1406efb4d:	41 8d 50 5c          	lea    edx,[r8+0x5c]
   1406efb51:	48 8b cb             	mov    rcx,rbx
   1406efb54:	e8 77 e8 23 00       	call   0x14092e3d0
   1406efb59:	45 33 c0             	xor    r8d,r8d
   1406efb5c:	41 8d 50 5e          	lea    edx,[r8+0x5e]
   1406efb60:	48 8b cb             	mov    rcx,rbx
   1406efb63:	e8 68 e8 23 00       	call   0x14092e3d0
   1406efb68:	45 33 c0             	xor    r8d,r8d
   1406efb6b:	41 8d 50 5f          	lea    edx,[r8+0x5f]
   1406efb6f:	48 8b cb             	mov    rcx,rbx
   1406efb72:	e8 59 e8 23 00       	call   0x14092e3d0
   1406efb77:	48 8b 86 b8 01 00 00 	mov    rax,QWORD PTR [rsi+0x1b8]
   1406efb7e:	44 0f b6 68 14       	movzx  r13d,BYTE PTR [rax+0x14]
   1406efb83:	e8 18 74 d0 ff       	call   0x1403f6fa0
   1406efb88:	4c 8b f0             	mov    r14,rax
   1406efb8b:	48 8d 88 10 b0 04 00 	lea    rcx,[rax+0x4b010]
   1406efb92:	e8 f9 8b c1 ff       	call   0x140308790
   1406efb97:	48 8b c8             	mov    rcx,rax
   1406efb9a:	ff 15 88 07 c5 00    	call   QWORD PTR [rip+0xc50788]        # 0x141340328
   1406efba0:	41 ff 46 08          	inc    DWORD PTR [r14+0x8]
   1406efba4:	41 83 7e 08 1e       	cmp    DWORD PTR [r14+0x8],0x1e
   1406efba9:	7c 08                	jl     0x1406efbb3
   1406efbab:	41 c7 46 08 00 00 00 	mov    DWORD PTR [r14+0x8],0x0
   1406efbb2:	00 
   1406efbb3:	49 63 46 08          	movsxd rax,DWORD PTR [r14+0x8]
   1406efbb7:	48 c1 e0 0c          	shl    rax,0xc
   1406efbbb:	49 8d be 0c d0 02 00 	lea    rdi,[r14+0x2d00c]
   1406efbc2:	48 03 f8             	add    rdi,rax
   1406efbc5:	33 c0                	xor    eax,eax
   1406efbc7:	b9 00 10 00 00       	mov    ecx,0x1000
   1406efbcc:	f3 aa                	rep stos BYTE PTR es:[rdi],al
   1406efbce:	49 63 7e 08          	movsxd rdi,DWORD PTR [r14+0x8]
   1406efbd2:	48 c1 e7 0c          	shl    rdi,0xc
   1406efbd6:	49 8d 8e 10 b0 04 00 	lea    rcx,[r14+0x4b010]
   1406efbdd:	e8 ae 8b c1 ff       	call   0x140308790
   1406efbe2:	48 8b c8             	mov    rcx,rax
   1406efbe5:	ff 15 ed 04 c5 00    	call   QWORD PTR [rip+0xc504ed]        # 0x1413400d8
   1406efbeb:	41 8b 07             	mov    eax,DWORD PTR [r15]
   1406efbee:	39 05 74 33 a5 01    	cmp    DWORD PTR [rip+0x1a53374],eax        # 0x142142f68
   1406efbf4:	7e 2d                	jle    0x1406efc23
   1406efbf6:	48 8d 0d 6b 33 a5 01 	lea    rcx,[rip+0x1a5336b]        # 0x142142f68
   1406efbfd:	e8 8a 95 90 00       	call   0x140ff918c
   1406efc02:	83 3d 5f 33 a5 01 ff 	cmp    DWORD PTR [rip+0x1a5335f],0xffffffff        # 0x142142f68
   1406efc09:	75 18                	jne    0x1406efc23
   1406efc0b:	48 8d 0d be c3 c4 00 	lea    rcx,[rip+0xc4c3be]        # 0x14133bfd0
   1406efc12:	e8 85 93 90 00       	call   0x140ff8f9c
   1406efc17:	48 8d 0d 4a 33 a5 01 	lea    rcx,[rip+0x1a5334a]        # 0x142142f68
   1406efc1e:	e8 09 95 90 00       	call   0x140ff912c
   1406efc23:	44 89 6c 24 20       	mov    DWORD PTR [rsp+0x20],r13d
   1406efc28:	4c 8d 0d a5 6b ca 00 	lea    r9,[rip+0xca6ba5]        # 0x1413967d4
   1406efc2f:	41 b8 00 10 00 00    	mov    r8d,0x1000
   1406efc35:	49 8d 96 0c d0 02 00 	lea    rdx,[r14+0x2d00c]
   1406efc3c:	48 03 d7             	add    rdx,rdi
   1406efc3f:	48 8d 0d 1f 33 a5 01 	lea    rcx,[rip+0x1a5331f]        # 0x142142f65
   1406efc46:	e8 a5 66 aa ff       	call   0x1401962f0
   1406efc4b:	41 b9 01 00 00 00    	mov    r9d,0x1
   1406efc51:	4d 8d 86 0c d0 02 00 	lea    r8,[r14+0x2d00c]
   1406efc58:	4c 03 c7             	add    r8,rdi
   1406efc5b:	41 8d 51 5b          	lea    edx,[r9+0x5b]
   1406efc5f:	48 8b cb             	mov    rcx,rbx
   1406efc62:	e8 79 05 24 00       	call   0x1409301e0
   1406efc67:	e8 34 73 d0 ff       	call   0x1403f6fa0
   1406efc6c:	4c 8b f0             	mov    r14,rax
   1406efc6f:	48 8d 88 10 b0 04 00 	lea    rcx,[rax+0x4b010]
   1406efc76:	e8 15 8b c1 ff       	call   0x140308790
   1406efc7b:	48 8b c8             	mov    rcx,rax
   1406efc7e:	ff 15 a4 06 c5 00    	call   QWORD PTR [rip+0xc506a4]        # 0x141340328
   1406efc84:	41 ff 46 08          	inc    DWORD PTR [r14+0x8]
   1406efc88:	41 83 7e 08 1e       	cmp    DWORD PTR [r14+0x8],0x1e
   1406efc8d:	7c 08                	jl     0x1406efc97
   1406efc8f:	41 c7 46 08 00 00 00 	mov    DWORD PTR [r14+0x8],0x0
   1406efc96:	00 
   1406efc97:	49 63 46 08          	movsxd rax,DWORD PTR [r14+0x8]
   1406efc9b:	48 c1 e0 0c          	shl    rax,0xc
   1406efc9f:	49 8d be 0c d0 02 00 	lea    rdi,[r14+0x2d00c]
   1406efca6:	48 03 f8             	add    rdi,rax
   1406efca9:	33 c0                	xor    eax,eax
   1406efcab:	b9 00 10 00 00       	mov    ecx,0x1000
   1406efcb0:	f3 aa                	rep stos BYTE PTR es:[rdi],al
   1406efcb2:	49 63 7e 08          	movsxd rdi,DWORD PTR [r14+0x8]
   1406efcb6:	48 c1 e7 0c          	shl    rdi,0xc
   1406efcba:	49 8d 8e 10 b0 04 00 	lea    rcx,[r14+0x4b010]
   1406efcc1:	e8 ca 8a c1 ff       	call   0x140308790
   1406efcc6:	48 8b c8             	mov    rcx,rax
   1406efcc9:	ff 15 09 04 c5 00    	call   QWORD PTR [rip+0xc50409]        # 0x1413400d8
   1406efccf:	41 8b 07             	mov    eax,DWORD PTR [r15]
   1406efcd2:	39 05 90 32 a5 01    	cmp    DWORD PTR [rip+0x1a53290],eax        # 0x142142f68
   1406efcd8:	7e 2d                	jle    0x1406efd07
   1406efcda:	48 8d 0d 87 32 a5 01 	lea    rcx,[rip+0x1a53287]        # 0x142142f68
   1406efce1:	e8 a6 94 90 00       	call   0x140ff918c
   1406efce6:	83 3d 7b 32 a5 01 ff 	cmp    DWORD PTR [rip+0x1a5327b],0xffffffff        # 0x142142f68
   1406efced:	75 18                	jne    0x1406efd07
   1406efcef:	48 8d 0d da c2 c4 00 	lea    rcx,[rip+0xc4c2da]        # 0x14133bfd0
   1406efcf6:	e8 a1 92 90 00       	call   0x140ff8f9c
   1406efcfb:	48 8d 0d 66 32 a5 01 	lea    rcx,[rip+0x1a53266]        # 0x142142f68
   1406efd02:	e8 25 94 90 00       	call   0x140ff912c
   1406efd07:	44 89 6c 24 20       	mov    DWORD PTR [rsp+0x20],r13d
   1406efd0c:	4c 8d 0d c1 6a ca 00 	lea    r9,[rip+0xca6ac1]        # 0x1413967d4
   1406efd13:	41 b8 00 10 00 00    	mov    r8d,0x1000
   1406efd19:	49 8d 96 0c d0 02 00 	lea    rdx,[r14+0x2d00c]
   1406efd20:	48 03 d7             	add    rdx,rdi
   1406efd23:	48 8d 0d 3b 32 a5 01 	lea    rcx,[rip+0x1a5323b]        # 0x142142f65
   1406efd2a:	e8 c1 65 aa ff       	call   0x1401962f0
   1406efd2f:	41 b9 01 00 00 00    	mov    r9d,0x1
   1406efd35:	4d 8d 86 0c d0 02 00 	lea    r8,[r14+0x2d00c]
   1406efd3c:	4c 03 c7             	add    r8,rdi
   1406efd3f:	41 8d 51 5d          	lea    edx,[r9+0x5d]
   1406efd43:	48 8b cb             	mov    rcx,rbx
   1406efd46:	e8 95 04 24 00       	call   0x1409301e0
   1406efd4b:	e8 50 72 d0 ff       	call   0x1403f6fa0
   1406efd50:	4c 8b f0             	mov    r14,rax
   1406efd53:	48 8d 88 10 b0 04 00 	lea    rcx,[rax+0x4b010]
   1406efd5a:	e8 31 8a c1 ff       	call   0x140308790
   1406efd5f:	48 8b c8             	mov    rcx,rax
   1406efd62:	ff 15 c0 05 c5 00    	call   QWORD PTR [rip+0xc505c0]        # 0x141340328
   1406efd68:	41 ff 46 08          	inc    DWORD PTR [r14+0x8]
   1406efd6c:	45 33 ed             	xor    r13d,r13d
   1406efd6f:	41 83 7e 08 1e       	cmp    DWORD PTR [r14+0x8],0x1e
   1406efd74:	7c 04                	jl     0x1406efd7a
   1406efd76:	45 89 6e 08          	mov    DWORD PTR [r14+0x8],r13d
   1406efd7a:	49 63 46 08          	movsxd rax,DWORD PTR [r14+0x8]
   1406efd7e:	48 c1 e0 0c          	shl    rax,0xc
   1406efd82:	49 8d be 0c d0 02 00 	lea    rdi,[r14+0x2d00c]
   1406efd89:	48 03 f8             	add    rdi,rax
   1406efd8c:	33 c0                	xor    eax,eax
   1406efd8e:	b9 00 10 00 00       	mov    ecx,0x1000
   1406efd93:	f3 aa                	rep stos BYTE PTR es:[rdi],al
   1406efd95:	49 63 7e 08          	movsxd rdi,DWORD PTR [r14+0x8]
   1406efd99:	48 c1 e7 0c          	shl    rdi,0xc
   1406efd9d:	49 8d 8e 10 b0 04 00 	lea    rcx,[r14+0x4b010]
   1406efda4:	e8 e7 89 c1 ff       	call   0x140308790
   1406efda9:	48 8b c8             	mov    rcx,rax
   1406efdac:	ff 15 26 03 c5 00    	call   QWORD PTR [rip+0xc50326]        # 0x1413400d8
   1406efdb2:	41 8b 07             	mov    eax,DWORD PTR [r15]
   1406efdb5:	39 05 ad 31 a5 01    	cmp    DWORD PTR [rip+0x1a531ad],eax        # 0x142142f68
   1406efdbb:	7e 2d                	jle    0x1406efdea
   1406efdbd:	48 8d 0d a4 31 a5 01 	lea    rcx,[rip+0x1a531a4]        # 0x142142f68
   1406efdc4:	e8 c3 93 90 00       	call   0x140ff918c
   1406efdc9:	83 3d 98 31 a5 01 ff 	cmp    DWORD PTR [rip+0x1a53198],0xffffffff        # 0x142142f68
   1406efdd0:	75 18                	jne    0x1406efdea
   1406efdd2:	48 8d 0d f7 c1 c4 00 	lea    rcx,[rip+0xc4c1f7]        # 0x14133bfd0
   1406efdd9:	e8 be 91 90 00       	call   0x140ff8f9c
   1406efdde:	48 8d 0d 83 31 a5 01 	lea    rcx,[rip+0x1a53183]        # 0x142142f68
   1406efde5:	e8 42 93 90 00       	call   0x140ff912c
   1406efdea:	44 89 6c 24 20       	mov    DWORD PTR [rsp+0x20],r13d
   1406efdef:	4c 8d 0d ca 04 d5 00 	lea    r9,[rip+0xd504ca]        # 0x1414402c0
   1406efdf6:	41 b8 00 10 00 00    	mov    r8d,0x1000
   1406efdfc:	49 8d 96 0c d0 02 00 	lea    rdx,[r14+0x2d00c]
   1406efe03:	48 03 d7             	add    rdx,rdi
   1406efe06:	48 8d 0d 58 31 a5 01 	lea    rcx,[rip+0x1a53158]        # 0x142142f65
   1406efe0d:	e8 de 64 aa ff       	call   0x1401962f0
   1406efe12:	41 b9 01 00 00 00    	mov    r9d,0x1
   1406efe18:	4c 8d 87 0c d0 02 00 	lea    r8,[rdi+0x2d00c]
   1406efe1f:	4d 03 c6             	add    r8,r14
   1406efe22:	41 8d 51 5e          	lea    edx,[r9+0x5e]
   1406efe26:	48 8b cb             	mov    rcx,rbx
   1406efe29:	e8 b2 03 24 00       	call   0x1409301e0
   1406efe2e:	b9 78 01 00 00       	mov    ecx,0x178
   1406efe33:	e8 78 5d d0 ff       	call   0x1403f5bb0
   1406efe38:	48 8b f8             	mov    rdi,rax
   1406efe3b:	48 89 44 24 48       	mov    QWORD PTR [rsp+0x48],rax
   1406efe40:	48 85 c0             	test   rax,rax
   1406efe43:	74 3c                	je     0x1406efe81
   1406efe45:	33 d2                	xor    edx,edx
   1406efe47:	41 b8 78 01 00 00    	mov    r8d,0x178
   1406efe4d:	48 8b c8             	mov    rcx,rax
   1406efe50:	e8 4b b2 92 00       	call   0x14101b0a0
   1406efe55:	48 8b cf             	mov    rcx,rdi
   1406efe58:	e8 23 48 17 00       	call   0x140864680
   1406efe5d:	90                   	nop
   1406efe5e:	48 8d 05 cb 26 c5 00 	lea    rax,[rip+0xc526cb]        # 0x141342530
   1406efe65:	48 89 07             	mov    QWORD PTR [rdi],rax
   1406efe68:	48 8d 8f 30 01 00 00 	lea    rcx,[rdi+0x130]
   1406efe6f:	e8 ac a1 23 00       	call   0x14092a020
   1406efe74:	90                   	nop
   1406efe75:	48 8d 05 fc 4b cf 00 	lea    rax,[rip+0xcf4bfc]        # 0x1413e4a78
   1406efe7c:	48 89 07             	mov    QWORD PTR [rdi],rax
   1406efe7f:	eb 03                	jmp    0x1406efe84
   1406efe81:	49 8b fd             	mov    rdi,r13
   1406efe84:	48 89 be c0 01 00 00 	mov    QWORD PTR [rsi+0x1c0],rdi
   1406efe8b:	48 8b 3f             	mov    rdi,QWORD PTR [rdi]
   1406efe8e:	b9 1e 08 00 00       	mov    ecx,0x81e
   1406efe93:	e8 b8 c0 c1 ff       	call   0x14030bf50
   1406efe98:	48 8b d0             	mov    rdx,rax
   1406efe9b:	4c 8b 05 f6 7c 35 01 	mov    r8,QWORD PTR [rip+0x1357cf6]        # 0x141a47b98
   1406efea2:	48 8b 8e c0 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c0]
   1406efea9:	ff 97 88 01 00 00    	call   QWORD PTR [rdi+0x188]
   1406efeaf:	b9 88 01 00 00       	mov    ecx,0x188
   1406efeb4:	e8 f7 5c d0 ff       	call   0x1403f5bb0
   1406efeb9:	48 89 44 24 48       	mov    QWORD PTR [rsp+0x48],rax
   1406efebe:	48 85 c0             	test   rax,rax
   1406efec1:	74 0d                	je     0x1406efed0
   1406efec3:	48 8b c8             	mov    rcx,rax
   1406efec6:	e8 35 a8 15 00       	call   0x14084a700
   1406efecb:	48 8b c8             	mov    rcx,rax
   1406efece:	eb 03                	jmp    0x1406efed3
   1406efed0:	49 8b cd             	mov    rcx,r13
   1406efed3:	48 89 8e c8 01 00 00 	mov    QWORD PTR [rsi+0x1c8],rcx
   1406efeda:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406efedd:	33 d2                	xor    edx,edx
   1406efedf:	44 8d 42 01          	lea    r8d,[rdx+0x1]
   1406efee3:	ff 90 88 01 00 00    	call   QWORD PTR [rax+0x188]
   1406efee9:	41 83 c9 ff          	or     r9d,0xffffffff
   1406efeed:	45 33 c0             	xor    r8d,r8d
   1406efef0:	41 8d 51 06          	lea    edx,[r9+0x6]
   1406efef4:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   1406efefb:	e8 20 f9 16 00       	call   0x14085f820
   1406eff00:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   1406eff07:	48 81 c1 30 01 00 00 	add    rcx,0x130
   1406eff0e:	ba 05 00 00 00       	mov    edx,0x5
   1406eff13:	e8 78 c4 23 00       	call   0x14092c390
   1406eff18:	c7 80 84 00 00 00 01 	mov    DWORD PTR [rax+0x84],0x1
   1406eff1f:	00 00 00 
   1406eff22:	44 89 a8 80 00 00 00 	mov    DWORD PTR [rax+0x80],r13d
   1406eff29:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   1406eff30:	48 8b b9 80 01 00 00 	mov    rdi,QWORD PTR [rcx+0x180]
   1406eff37:	ba 05 00 00 00       	mov    edx,0x5
   1406eff3c:	e8 9f db 15 00       	call   0x14084dae0
   1406eff41:	48 8b d8             	mov    rbx,rax
   1406eff44:	48 8b cf             	mov    rcx,rdi
   1406eff47:	e8 84 e5 15 00       	call   0x14084e4d0
   1406eff4c:	48 8b c8             	mov    rcx,rax
   1406eff4f:	45 33 c9             	xor    r9d,r9d
   1406eff52:	41 b8 4e 5c 00 00    	mov    r8d,0x5c4e
   1406eff58:	48 8b d3             	mov    rdx,rbx
   1406eff5b:	e8 70 ad 1a 00       	call   0x14089acd0
   1406eff60:	41 83 c9 ff          	or     r9d,0xffffffff
   1406eff64:	41 8d 51 05          	lea    edx,[r9+0x5]
   1406eff68:	45 8d 41 02          	lea    r8d,[r9+0x2]
   1406eff6c:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   1406eff73:	e8 a8 f8 16 00       	call   0x14085f820
   1406eff78:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   1406eff7f:	48 8b b9 80 01 00 00 	mov    rdi,QWORD PTR [rcx+0x180]
   1406eff86:	ba 04 00 00 00       	mov    edx,0x4
   1406eff8b:	e8 50 db 15 00       	call   0x14084dae0
   1406eff90:	48 8b d8             	mov    rbx,rax
   1406eff93:	48 8b cf             	mov    rcx,rdi
   1406eff96:	e8 35 e5 15 00       	call   0x14084e4d0
   1406eff9b:	48 8b c8             	mov    rcx,rax
   1406eff9e:	45 33 c9             	xor    r9d,r9d
   1406effa1:	41 b8 33 27 00 00    	mov    r8d,0x2733
   1406effa7:	48 8b d3             	mov    rdx,rbx
   1406effaa:	e8 21 ad 1a 00       	call   0x14089acd0
   1406effaf:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   1406effb6:	48 81 c1 30 01 00 00 	add    rcx,0x130
   1406effbd:	ba 04 00 00 00       	mov    edx,0x4
   1406effc2:	e8 c9 c3 23 00       	call   0x14092c390
   1406effc7:	c7 80 84 00 00 00 01 	mov    DWORD PTR [rax+0x84],0x1
   1406effce:	00 00 00 
   1406effd1:	c7 80 80 00 00 00 01 	mov    DWORD PTR [rax+0x80],0x1
   1406effd8:	00 00 00 
   1406effdb:	48 8d 05 1e 11 d6 00 	lea    rax,[rip+0xd6111e]        # 0x141451100
   1406effe2:	48 89 44 24 50       	mov    QWORD PTR [rsp+0x50],rax
   1406effe7:	48 89 74 24 58       	mov    QWORD PTR [rsp+0x58],rsi
   1406effec:	48 8d 44 24 50       	lea    rax,[rsp+0x50]
   1406efff1:	48 89 45 88          	mov    QWORD PTR [rbp-0x78],rax
   1406efff5:	48 8d 44 24 50       	lea    rax,[rsp+0x50]
   1406efffa:	48 89 44 24 48       	mov    QWORD PTR [rsp+0x48],rax
   1406effff:	48 8b be c8 01 00 00 	mov    rdi,QWORD PTR [rsi+0x1c8]
   1406f0006:	48 8d 45 a0          	lea    rax,[rbp-0x60]
   1406f000a:	48 89 44 24 40       	mov    QWORD PTR [rsp+0x40],rax
   1406f000f:	4c 89 6d d8          	mov    QWORD PTR [rbp-0x28],r13
   1406f0013:	48 8d 55 a0          	lea    rdx,[rbp-0x60]
   1406f0017:	48 8d 4c 24 50       	lea    rcx,[rsp+0x50]
   1406f001c:	ff 15 de 10 d6 00    	call   QWORD PTR [rip+0xd610de]        # 0x141451100
   1406f0022:	48 89 45 d8          	mov    QWORD PTR [rbp-0x28],rax
   1406f0026:	48 8d 4f 70          	lea    rcx,[rdi+0x70]
   1406f002a:	4c 8d 45 a0          	lea    r8,[rbp-0x60]
   1406f002e:	ba 78 05 00 00       	mov    edx,0x578
   1406f0033:	e8 a8 6b 17 00       	call   0x140866be0
   1406f0038:	90                   	nop
   1406f0039:	48 8b 4d 88          	mov    rcx,QWORD PTR [rbp-0x78]
   1406f003d:	48 85 c9             	test   rcx,rcx
   1406f0040:	74 11                	je     0x1406f0053
   1406f0042:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406f0045:	48 8d 7c 24 50       	lea    rdi,[rsp+0x50]
   1406f004a:	48 3b cf             	cmp    rcx,rdi
   1406f004d:	0f 95 c2             	setne  dl
   1406f0050:	ff 50 20             	call   QWORD PTR [rax+0x20]
   1406f0053:	48 8b 86 c8 01 00 00 	mov    rax,QWORD PTR [rsi+0x1c8]
   1406f005a:	48 8b 88 80 01 00 00 	mov    rcx,QWORD PTR [rax+0x180]
   1406f0061:	e8 6a e4 15 00       	call   0x14084e4d0
   1406f0066:	48 8b c8             	mov    rcx,rax
   1406f0069:	ba 4e 5c 00 00       	mov    edx,0x5c4e
   1406f006e:	e8 6d 0e 1b 00       	call   0x1408a0ee0
   1406f0073:	b9 a0 00 00 00       	mov    ecx,0xa0
   1406f0078:	e8 d3 be c1 ff       	call   0x14030bf50
   1406f007d:	45 33 c9             	xor    r9d,r9d
   1406f0080:	4c 8b c0             	mov    r8,rax
   1406f0083:	41 8d 51 0e          	lea    edx,[r9+0xe]
   1406f0087:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   1406f008e:	e8 2d f7 16 00       	call   0x14085f7c0
   1406f0093:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   1406f009a:	48 81 c1 30 01 00 00 	add    rcx,0x130
   1406f00a1:	ba 0e 00 00 00       	mov    edx,0xe
   1406f00a6:	e8 e5 c2 23 00       	call   0x14092c390
   1406f00ab:	c7 80 84 00 00 00 01 	mov    DWORD PTR [rax+0x84],0x1
   1406f00b2:	00 00 00 
   1406f00b5:	c7 80 80 00 00 00 03 	mov    DWORD PTR [rax+0x80],0x3
   1406f00bc:	00 00 00 
   1406f00bf:	b9 a1 00 00 00       	mov    ecx,0xa1
   1406f00c4:	e8 87 be c1 ff       	call   0x14030bf50
   1406f00c9:	4c 8b c0             	mov    r8,rax
   1406f00cc:	45 33 c9             	xor    r9d,r9d
   1406f00cf:	41 8d 51 11          	lea    edx,[r9+0x11]
   1406f00d3:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   1406f00da:	e8 e1 f6 16 00       	call   0x14085f7c0
   1406f00df:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   1406f00e6:	48 81 c1 30 01 00 00 	add    rcx,0x130
   1406f00ed:	ba 11 00 00 00       	mov    edx,0x11
   1406f00f2:	e8 99 c2 23 00       	call   0x14092c390
   1406f00f7:	c7 80 84 00 00 00 01 	mov    DWORD PTR [rax+0x84],0x1
   1406f00fe:	00 00 00 
   1406f0101:	c7 80 80 00 00 00 04 	mov    DWORD PTR [rax+0x80],0x4
   1406f0108:	00 00 00 
   1406f010b:	b9 a2 00 00 00       	mov    ecx,0xa2
   1406f0110:	e8 3b be c1 ff       	call   0x14030bf50
   1406f0115:	45 33 c9             	xor    r9d,r9d
   1406f0118:	4c 8b c0             	mov    r8,rax
   1406f011b:	41 8d 51 12          	lea    edx,[r9+0x12]
   1406f011f:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   1406f0126:	e8 95 f6 16 00       	call   0x14085f7c0
   1406f012b:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   1406f0132:	48 81 c1 30 01 00 00 	add    rcx,0x130
   1406f0139:	ba 12 00 00 00       	mov    edx,0x12
   1406f013e:	e8 4d c2 23 00       	call   0x14092c390
   1406f0143:	c7 80 84 00 00 00 01 	mov    DWORD PTR [rax+0x84],0x1
   1406f014a:	00 00 00 
   1406f014d:	c7 80 80 00 00 00 05 	mov    DWORD PTR [rax+0x80],0x5
   1406f0154:	00 00 00 
   1406f0157:	b9 80 01 00 00       	mov    ecx,0x180
   1406f015c:	e8 4f 5a d0 ff       	call   0x1403f5bb0
   1406f0161:	48 89 44 24 48       	mov    QWORD PTR [rsp+0x48],rax
   1406f0166:	48 85 c0             	test   rax,rax
   1406f0169:	74 0d                	je     0x1406f0178
   1406f016b:	48 8b c8             	mov    rcx,rax
   1406f016e:	e8 4d a1 15 00       	call   0x14084a2c0
   1406f0173:	48 8b c8             	mov    rcx,rax
   1406f0176:	eb 03                	jmp    0x1406f017b
   1406f0178:	49 8b cd             	mov    rcx,r13
   1406f017b:	48 89 8e d0 01 00 00 	mov    QWORD PTR [rsi+0x1d0],rcx
   1406f0182:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406f0185:	45 33 c9             	xor    r9d,r9d
   1406f0188:	33 d2                	xor    edx,edx
   1406f018a:	45 8d 41 01          	lea    r8d,[r9+0x1]
   1406f018e:	ff 90 88 01 00 00    	call   QWORD PTR [rax+0x188]
   1406f0194:	b9 98 01 00 00       	mov    ecx,0x198
   1406f0199:	e8 12 5a d0 ff       	call   0x1403f5bb0
   1406f019e:	48 89 44 24 48       	mov    QWORD PTR [rsp+0x48],rax
   1406f01a3:	48 85 c0             	test   rax,rax
   1406f01a6:	74 0d                	je     0x1406f01b5
   1406f01a8:	48 8b c8             	mov    rcx,rax
   1406f01ab:	e8 b0 a3 15 00       	call   0x14084a560
   1406f01b0:	48 8b c8             	mov    rcx,rax
   1406f01b3:	eb 03                	jmp    0x1406f01b8
   1406f01b5:	49 8b cd             	mov    rcx,r13
   1406f01b8:	48 89 8e d8 01 00 00 	mov    QWORD PTR [rsi+0x1d8],rcx
   1406f01bf:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1406f01c2:	ff 90 88 01 00 00    	call   QWORD PTR [rax+0x188]
   1406f01c8:	4c 8b c6             	mov    r8,rsi
   1406f01cb:	ba 01 00 00 00       	mov    edx,0x1
   1406f01d0:	48 8b 8e d8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1d8]
   1406f01d7:	e8 74 64 16 00       	call   0x140856650
   1406f01dc:	4c 8b 86 c0 01 00 00 	mov    r8,QWORD PTR [rsi+0x1c0]
   1406f01e3:	ba 04 00 00 00       	mov    edx,0x4
   1406f01e8:	48 8b 8e d8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1d8]
   1406f01ef:	e8 5c 64 16 00       	call   0x140856650
   1406f01f4:	4c 8b 86 c8 01 00 00 	mov    r8,QWORD PTR [rsi+0x1c8]
   1406f01fb:	ba 06 00 00 00       	mov    edx,0x6
   1406f0200:	48 8b 8e d8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1d8]
   1406f0207:	e8 44 64 16 00       	call   0x140856650
   1406f020c:	4c 8b 86 d0 01 00 00 	mov    r8,QWORD PTR [rsi+0x1d0]
   1406f0213:	ba ff ff ff 7f       	mov    edx,0x7fffffff
   1406f0218:	48 8b 8e d8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1d8]
   1406f021f:	e8 2c 64 16 00       	call   0x140856650
   1406f0224:	48 8d 05 0d 0f d6 00 	lea    rax,[rip+0xd60f0d]        # 0x141451138
   1406f022b:	48 89 44 24 50       	mov    QWORD PTR [rsp+0x50],rax
   1406f0230:	48 89 74 24 58       	mov    QWORD PTR [rsp+0x58],rsi
   1406f0235:	48 8d 44 24 50       	lea    rax,[rsp+0x50]
   1406f023a:	48 89 45 88          	mov    QWORD PTR [rbp-0x78],rax
   1406f023e:	48 8d 44 24 50       	lea    rax,[rsp+0x50]
   1406f0243:	48 89 44 24 48       	mov    QWORD PTR [rsp+0x48],rax
   1406f0248:	48 8b be d8 01 00 00 	mov    rdi,QWORD PTR [rsi+0x1d8]
   1406f024f:	48 8d 45 a0          	lea    rax,[rbp-0x60]
   1406f0253:	48 89 44 24 40       	mov    QWORD PTR [rsp+0x40],rax
   1406f0258:	4c 89 6d d8          	mov    QWORD PTR [rbp-0x28],r13
   1406f025c:	48 8d 55 a0          	lea    rdx,[rbp-0x60]
   1406f0260:	48 8d 4c 24 50       	lea    rcx,[rsp+0x50]
   1406f0265:	ff 15 cd 0e d6 00    	call   QWORD PTR [rip+0xd60ecd]        # 0x141451138
   1406f026b:	48 89 45 d8          	mov    QWORD PTR [rbp-0x28],rax
   1406f026f:	48 8d 4f 70          	lea    rcx,[rdi+0x70]
   1406f0273:	4c 8d 45 a0          	lea    r8,[rbp-0x60]
   1406f0277:	ba b8 0b 00 00       	mov    edx,0xbb8
   1406f027c:	e8 5f 69 17 00       	call   0x140866be0
   1406f0281:	90                   	nop
   1406f0282:	48 8b 4d 88          	mov    rcx,QWORD PTR [rbp-0x78]
   1406f0286:	48 85 c9             	test   rcx,rcx
   1406f0289:	74 11                	je     0x1406f029c
   1406f028b:	48 8b 39             	mov    rdi,QWORD PTR [rcx]
   1406f028e:	48 8d 54 24 50       	lea    rdx,[rsp+0x50]
   1406f0293:	48 3b ca             	cmp    rcx,rdx
   1406f0296:	0f 95 c2             	setne  dl
   1406f0299:	ff 57 20             	call   QWORD PTR [rdi+0x20]
   1406f029c:	b8 01 00 00 00       	mov    eax,0x1
   1406f02a1:	48 8b 8d f0 07 00 00 	mov    rcx,QWORD PTR [rbp+0x7f0]
   1406f02a8:	48 33 cc             	xor    rcx,rsp
   1406f02ab:	e8 c0 91 90 00       	call   0x140ff9470
   1406f02b0:	4c 8d 9c 24 00 09 00 	lea    r11,[rsp+0x900]
   1406f02b7:	00 
   1406f02b8:	49 8b 5b 38          	mov    rbx,QWORD PTR [r11+0x38]
   1406f02bc:	49 8b 73 40          	mov    rsi,QWORD PTR [r11+0x40]
   1406f02c0:	49 8b 7b 48          	mov    rdi,QWORD PTR [r11+0x48]
   1406f02c4:	49 8b e3             	mov    rsp,r11
   1406f02c7:	41 5f                	pop    r15
   1406f02c9:	41 5e                	pop    r14
   1406f02cb:	41 5d                	pop    r13
   1406f02cd:	41 5c                	pop    r12
   1406f02cf:	5d                   	pop    rbp
   1406f02d0:	c3                   	ret
