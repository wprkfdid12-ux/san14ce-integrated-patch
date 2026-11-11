
/home/claude/r40/mt_test/source/build/SAN14_MeritTraits.dll:     file format pei-x86-64


Disassembly of section .text:

00000001800034e0 <.text+0x24e0>:
   1800034e0:	41 57                	push   r15
   1800034e2:	41 56                	push   r14
   1800034e4:	41 55                	push   r13
   1800034e6:	41 54                	push   r12
   1800034e8:	56                   	push   rsi
   1800034e9:	57                   	push   rdi
   1800034ea:	55                   	push   rbp
   1800034eb:	53                   	push   rbx
   1800034ec:	b8 88 2c 00 00       	mov    eax,0x2c88
   1800034f1:	e8 f6 a3 02 00       	call   0x18002d8ec
   1800034f6:	48 29 c4             	sub    rsp,rax
   1800034f9:	66 44 0f 29 bc 24 70 	movapd XMMWORD PTR [rsp+0x2c70],xmm15
   180003500:	2c 00 00 
   180003503:	66 44 0f 29 b4 24 60 	movapd XMMWORD PTR [rsp+0x2c60],xmm14
   18000350a:	2c 00 00 
   18000350d:	66 44 0f 29 ac 24 50 	movapd XMMWORD PTR [rsp+0x2c50],xmm13
   180003514:	2c 00 00 
   180003517:	66 44 0f 29 a4 24 40 	movapd XMMWORD PTR [rsp+0x2c40],xmm12
   18000351e:	2c 00 00 
   180003521:	66 44 0f 29 9c 24 30 	movapd XMMWORD PTR [rsp+0x2c30],xmm11
   180003528:	2c 00 00 
   18000352b:	66 44 0f 29 94 24 20 	movapd XMMWORD PTR [rsp+0x2c20],xmm10
   180003532:	2c 00 00 
   180003535:	66 44 0f 29 8c 24 10 	movapd XMMWORD PTR [rsp+0x2c10],xmm9
   18000353c:	2c 00 00 
   18000353f:	66 44 0f 29 84 24 00 	movapd XMMWORD PTR [rsp+0x2c00],xmm8
   180003546:	2c 00 00 
   180003549:	66 0f 29 bc 24 f0 2b 	movapd XMMWORD PTR [rsp+0x2bf0],xmm7
   180003550:	00 00 
   180003552:	66 0f 29 b4 24 e0 2b 	movapd XMMWORD PTR [rsp+0x2be0],xmm6
   180003559:	00 00 
   18000355b:	48 89 cd             	mov    rbp,rcx
   18000355e:	4c 8b b4 24 f8 2c 00 	mov    r14,QWORD PTR [rsp+0x2cf8]
   180003565:	00 
   180003566:	66 48 0f 6e c1       	movq   xmm0,rcx
   18000356b:	66 48 0f 6e ca       	movq   xmm1,rdx
   180003570:	66 0f 6c c8          	punpcklqdq xmm1,xmm0
   180003574:	66 49 0f 6e c0       	movq   xmm0,r8
   180003579:	66 49 0f 6e d6       	movq   xmm2,r14
   18000357e:	66 0f 6c d0          	punpcklqdq xmm2,xmm0
   180003582:	66 0f ef c0          	pxor   xmm0,xmm0
   180003586:	66 0f 76 d0          	pcmpeqd xmm2,xmm0
   18000358a:	66 0f 76 c8          	pcmpeqd xmm1,xmm0
   18000358e:	66 0f 6f c1          	movdqa xmm0,xmm1
   180003592:	0f c6 c2 dd          	shufps xmm0,xmm2,0xdd
   180003596:	0f c6 ca 88          	shufps xmm1,xmm2,0x88
   18000359a:	0f 54 c8             	andps  xmm1,xmm0
   18000359d:	0f 50 c9             	movmskps ecx,xmm1
   1800035a0:	b8 ff ff ff ff       	mov    eax,0xffffffff
   1800035a5:	85 c9                	test   ecx,ecx
   1800035a7:	74 6c                	je     0x180003615
   1800035a9:	0f 28 b4 24 e0 2b 00 	movaps xmm6,XMMWORD PTR [rsp+0x2be0]
   1800035b0:	00 
   1800035b1:	0f 28 bc 24 f0 2b 00 	movaps xmm7,XMMWORD PTR [rsp+0x2bf0]
   1800035b8:	00 
   1800035b9:	44 0f 28 84 24 00 2c 	movaps xmm8,XMMWORD PTR [rsp+0x2c00]
   1800035c0:	00 00 
   1800035c2:	44 0f 28 8c 24 10 2c 	movaps xmm9,XMMWORD PTR [rsp+0x2c10]
   1800035c9:	00 00 
   1800035cb:	44 0f 28 94 24 20 2c 	movaps xmm10,XMMWORD PTR [rsp+0x2c20]
   1800035d2:	00 00 
   1800035d4:	44 0f 28 9c 24 30 2c 	movaps xmm11,XMMWORD PTR [rsp+0x2c30]
   1800035db:	00 00 
   1800035dd:	44 0f 28 a4 24 40 2c 	movaps xmm12,XMMWORD PTR [rsp+0x2c40]
   1800035e4:	00 00 
   1800035e6:	44 0f 28 ac 24 50 2c 	movaps xmm13,XMMWORD PTR [rsp+0x2c50]
   1800035ed:	00 00 
   1800035ef:	44 0f 28 b4 24 60 2c 	movaps xmm14,XMMWORD PTR [rsp+0x2c60]
   1800035f6:	00 00 
   1800035f8:	44 0f 28 bc 24 70 2c 	movaps xmm15,XMMWORD PTR [rsp+0x2c70]
   1800035ff:	00 00 
   180003601:	48 81 c4 88 2c 00 00 	add    rsp,0x2c88
   180003608:	5b                   	pop    rbx
   180003609:	5d                   	pop    rbp
   18000360a:	5f                   	pop    rdi
   18000360b:	5e                   	pop    rsi
   18000360c:	41 5c                	pop    r12
   18000360e:	41 5d                	pop    r13
   180003610:	41 5e                	pop    r14
   180003612:	41 5f                	pop    r15
   180003614:	c3                   	ret
   180003615:	49 89 d5             	mov    r13,rdx
   180003618:	80 7a 12 00          	cmp    BYTE PTR [rdx+0x12],0x0
   18000361c:	74 8b                	je     0x1800035a9
   18000361e:	4c 89 84 24 10 01 00 	mov    QWORD PTR [rsp+0x110],r8
   180003625:	00 
   180003626:	45 89 cf             	mov    r15d,r9d
   180003629:	e8 d2 f7 ff ff       	call   0x180002e00
   18000362e:	f0 48 ff 05 3a 1a 0b 	lock inc QWORD PTR [rip+0xb1a3a]        # 0x1800b5070
   180003635:	00 
   180003636:	8b 05 b8 6d 04 00    	mov    eax,DWORD PTR [rip+0x46db8]        # 0x18004a3f4
   18000363c:	c7 44 24 20 00 00 00 	mov    DWORD PTR [rsp+0x20],0x0
   180003643:	00 
   180003644:	b9 00 00 00 00       	mov    ecx,0x0
   180003649:	48 89 4c 24 38       	mov    QWORD PTR [rsp+0x38],rcx
   18000364e:	c7 44 24 30 00 00 00 	mov    DWORD PTR [rsp+0x30],0x0
   180003655:	00 
   180003656:	89 84 24 08 01 00 00 	mov    DWORD PTR [rsp+0x108],eax
   18000365d:	83 f8 02             	cmp    eax,0x2
   180003660:	48 89 6c 24 40       	mov    QWORD PTR [rsp+0x40],rbp
   180003665:	0f 85 0c 01 00 00    	jne    0x180003777
   18000366b:	4c 8d a4 24 a0 27 00 	lea    r12,[rsp+0x27a0]
   180003672:	00 
   180003673:	ba 40 04 00 00       	mov    edx,0x440
   180003678:	4c 89 e1             	mov    rcx,r12
   18000367b:	e8 f0 da ff ff       	call   0x180001170
   180003680:	48 8d 9c 24 60 23 00 	lea    rbx,[rsp+0x2360]
   180003687:	00 
   180003688:	ba 40 04 00 00       	mov    edx,0x440
   18000368d:	48 89 d9             	mov    rcx,rbx
   180003690:	e8 db da ff ff       	call   0x180001170
   180003695:	31 c0                	xor    eax,eax
   180003697:	48 89 44 24 38       	mov    QWORD PTR [rsp+0x38],rax
   18000369c:	48 8d 35 5d 6d 04 00 	lea    rsi,[rip+0x46d5d]        # 0x18004a400
   1800036a3:	c7 44 24 30 00 00 00 	mov    DWORD PTR [rsp+0x30],0x0
   1800036aa:	00 
   1800036ab:	31 ff                	xor    edi,edi
   1800036ad:	eb 13                	jmp    0x1800036c2
   1800036af:	90                   	nop
   1800036b0:	48 8b 6c 24 40       	mov    rbp,QWORD PTR [rsp+0x40]
   1800036b5:	48 ff c7             	inc    rdi
   1800036b8:	48 83 ff 09          	cmp    rdi,0x9
   1800036bc:	0f 84 b5 00 00 00    	je     0x180003777
   1800036c2:	0f b7 6c 7d 02       	movzx  ebp,WORD PTR [rbp+rdi*2+0x2]
   1800036c7:	8d 45 01             	lea    eax,[rbp+0x1]
   1800036ca:	66 83 f8 02          	cmp    ax,0x2
   1800036ce:	72 e0                	jb     0x1800036b0
   1800036d0:	4c 89 e9             	mov    rcx,r13
   1800036d3:	89 ea                	mov    edx,ebp
   1800036d5:	e8 e6 db ff ff       	call   0x1800012c0
   1800036da:	85 c0                	test   eax,eax
   1800036dc:	48 8b 54 24 38       	mov    rdx,QWORD PTR [rsp+0x38]
   1800036e1:	89 d1                	mov    ecx,edx
   1800036e3:	44 8b 44 24 30       	mov    r8d,DWORD PTR [rsp+0x30]
   1800036e8:	41 0f 44 c8          	cmove  ecx,r8d
   1800036ec:	ff c1                	inc    ecx
   1800036ee:	85 c0                	test   eax,eax
   1800036f0:	4c 89 e0             	mov    rax,r12
   1800036f3:	48 0f 44 c3          	cmove  rax,rbx
   1800036f7:	0f 45 d1             	cmovne edx,ecx
   1800036fa:	48 89 54 24 38       	mov    QWORD PTR [rsp+0x38],rdx
   1800036ff:	44 0f 44 c1          	cmove  r8d,ecx
   180003703:	44 89 44 24 30       	mov    DWORD PTR [rsp+0x30],r8d
   180003708:	81 fd 62 01 00 00    	cmp    ebp,0x162
   18000370e:	77 a0                	ja     0x1800036b0
   180003710:	48 69 cd 40 04 00 00 	imul   rcx,rbp,0x440
   180003717:	48 01 f1             	add    rcx,rsi
   18000371a:	ba 30 00 00 00       	mov    edx,0x30
   18000371f:	90                   	nop
   180003720:	66 0f 6f 44 10 d0    	movdqa xmm0,XMMWORD PTR [rax+rdx*1-0x30]
   180003726:	66 0f 6f 4c 10 e0    	movdqa xmm1,XMMWORD PTR [rax+rdx*1-0x20]
   18000372c:	66 0f 6f 54 10 f0    	movdqa xmm2,XMMWORD PTR [rax+rdx*1-0x10]
   180003732:	66 0f 6f 1c 10       	movdqa xmm3,XMMWORD PTR [rax+rdx*1]
   180003737:	66 0f fc 44 11 d0    	paddb  xmm0,XMMWORD PTR [rcx+rdx*1-0x30]
   18000373d:	66 0f fc 4c 11 e0    	paddb  xmm1,XMMWORD PTR [rcx+rdx*1-0x20]
   180003743:	66 0f 7f 44 10 d0    	movdqa XMMWORD PTR [rax+rdx*1-0x30],xmm0
   180003749:	66 0f 7f 4c 10 e0    	movdqa XMMWORD PTR [rax+rdx*1-0x20],xmm1
   18000374f:	66 0f fc 54 11 f0    	paddb  xmm2,XMMWORD PTR [rcx+rdx*1-0x10]
   180003755:	66 0f fc 1c 11       	paddb  xmm3,XMMWORD PTR [rcx+rdx*1]
   18000375a:	66 0f 7f 54 10 f0    	movdqa XMMWORD PTR [rax+rdx*1-0x10],xmm2
   180003760:	66 0f 7f 1c 10       	movdqa XMMWORD PTR [rax+rdx*1],xmm3
   180003765:	48 83 c2 40          	add    rdx,0x40
   180003769:	48 81 fa 70 04 00 00 	cmp    rdx,0x470
   180003770:	75 ae                	jne    0x180003720
   180003772:	e9 39 ff ff ff       	jmp    0x1800036b0
   180003777:	ba c8 22 00 00       	mov    edx,0x22c8
   18000377c:	4c 89 f1             	mov    rcx,r14
   18000377f:	e8 ec d9 ff ff       	call   0x180001170
   180003784:	44 89 fa             	mov    edx,r15d
   180003787:	45 88 be 68 22 00 00 	mov    BYTE PTR [r14+0x2268],r15b
   18000378e:	0f b6 84 24 f0 2c 00 	movzx  eax,BYTE PTR [rsp+0x2cf0]
   180003795:	00 
   180003796:	41 88 86 69 22 00 00 	mov    BYTE PTR [r14+0x2269],al
   18000379d:	0f b7 45 02          	movzx  eax,WORD PTR [rbp+0x2]
   1800037a1:	48 3d 62 01 00 00    	cmp    rax,0x162
   1800037a7:	77 18                	ja     0x1800037c1
   1800037a9:	c1 e0 04             	shl    eax,0x4
   1800037ac:	31 c9                	xor    ecx,ecx
   1800037ae:	4c 8d 05 1b 58 03 00 	lea    r8,[rip+0x3581b]        # 0x180038fd0
   1800037b5:	42 80 3c 00 03       	cmp    BYTE PTR [rax+r8*1],0x3
   1800037ba:	0f 94 c1             	sete   cl
   1800037bd:	89 4c 24 20          	mov    DWORD PTR [rsp+0x20],ecx
   1800037c1:	0f b7 45 04          	movzx  eax,WORD PTR [rbp+0x4]
   1800037c5:	48 3d 62 01 00 00    	cmp    rax,0x162
   1800037cb:	77 18                	ja     0x1800037e5
   1800037cd:	c1 e0 04             	shl    eax,0x4
   1800037d0:	31 c9                	xor    ecx,ecx
   1800037d2:	4c 8d 05 f7 57 03 00 	lea    r8,[rip+0x357f7]        # 0x180038fd0
   1800037d9:	42 80 3c 00 03       	cmp    BYTE PTR [rax+r8*1],0x3
   1800037de:	0f 94 c1             	sete   cl
   1800037e1:	01 4c 24 20          	add    DWORD PTR [rsp+0x20],ecx
   1800037e5:	0f b7 45 06          	movzx  eax,WORD PTR [rbp+0x6]
   1800037e9:	48 3d 62 01 00 00    	cmp    rax,0x162
   1800037ef:	77 18                	ja     0x180003809
   1800037f1:	c1 e0 04             	shl    eax,0x4
   1800037f4:	31 c9                	xor    ecx,ecx
   1800037f6:	4c 8d 05 d3 57 03 00 	lea    r8,[rip+0x357d3]        # 0x180038fd0
   1800037fd:	42 80 3c 00 03       	cmp    BYTE PTR [rax+r8*1],0x3
   180003802:	0f 94 c1             	sete   cl
   180003805:	01 4c 24 20          	add    DWORD PTR [rsp+0x20],ecx
   180003809:	0f b7 45 08          	movzx  eax,WORD PTR [rbp+0x8]
   18000380d:	48 3d 62 01 00 00    	cmp    rax,0x162
   180003813:	77 18                	ja     0x18000382d
   180003815:	c1 e0 04             	shl    eax,0x4
   180003818:	31 c9                	xor    ecx,ecx
   18000381a:	4c 8d 05 af 57 03 00 	lea    r8,[rip+0x357af]        # 0x180038fd0
   180003821:	42 80 3c 00 03       	cmp    BYTE PTR [rax+r8*1],0x3
   180003826:	0f 94 c1             	sete   cl
   180003829:	01 4c 24 20          	add    DWORD PTR [rsp+0x20],ecx
   18000382d:	0f b7 45 0a          	movzx  eax,WORD PTR [rbp+0xa]
   180003831:	48 3d 62 01 00 00    	cmp    rax,0x162
   180003837:	77 18                	ja     0x180003851
   180003839:	c1 e0 04             	shl    eax,0x4
   18000383c:	31 c9                	xor    ecx,ecx
   18000383e:	4c 8d 05 8b 57 03 00 	lea    r8,[rip+0x3578b]        # 0x180038fd0
   180003845:	42 80 3c 00 03       	cmp    BYTE PTR [rax+r8*1],0x3
   18000384a:	0f 94 c1             	sete   cl
   18000384d:	01 4c 24 20          	add    DWORD PTR [rsp+0x20],ecx
   180003851:	0f b7 45 0c          	movzx  eax,WORD PTR [rbp+0xc]
   180003855:	48 3d 62 01 00 00    	cmp    rax,0x162
   18000385b:	77 18                	ja     0x180003875
   18000385d:	c1 e0 04             	shl    eax,0x4
   180003860:	31 c9                	xor    ecx,ecx
   180003862:	4c 8d 05 67 57 03 00 	lea    r8,[rip+0x35767]        # 0x180038fd0
   180003869:	42 80 3c 00 03       	cmp    BYTE PTR [rax+r8*1],0x3
   18000386e:	0f 94 c1             	sete   cl
   180003871:	01 4c 24 20          	add    DWORD PTR [rsp+0x20],ecx
   180003875:	0f b7 45 0e          	movzx  eax,WORD PTR [rbp+0xe]
   180003879:	48 3d 62 01 00 00    	cmp    rax,0x162
   18000387f:	77 18                	ja     0x180003899
   180003881:	c1 e0 04             	shl    eax,0x4
   180003884:	31 c9                	xor    ecx,ecx
   180003886:	4c 8d 05 43 57 03 00 	lea    r8,[rip+0x35743]        # 0x180038fd0
   18000388d:	42 80 3c 00 03       	cmp    BYTE PTR [rax+r8*1],0x3
   180003892:	0f 94 c1             	sete   cl
   180003895:	01 4c 24 20          	add    DWORD PTR [rsp+0x20],ecx
   180003899:	0f b7 45 10          	movzx  eax,WORD PTR [rbp+0x10]
   18000389d:	48 3d 62 01 00 00    	cmp    rax,0x162
   1800038a3:	77 18                	ja     0x1800038bd
   1800038a5:	c1 e0 04             	shl    eax,0x4
   1800038a8:	31 c9                	xor    ecx,ecx
   1800038aa:	4c 8d 05 1f 57 03 00 	lea    r8,[rip+0x3571f]        # 0x180038fd0
   1800038b1:	42 80 3c 00 03       	cmp    BYTE PTR [rax+r8*1],0x3
   1800038b6:	0f 94 c1             	sete   cl
   1800038b9:	01 4c 24 20          	add    DWORD PTR [rsp+0x20],ecx
   1800038bd:	0f b7 45 12          	movzx  eax,WORD PTR [rbp+0x12]
   1800038c1:	48 3d 62 01 00 00    	cmp    rax,0x162
   1800038c7:	88 54 24 27          	mov    BYTE PTR [rsp+0x27],dl
   1800038cb:	4c 89 6c 24 78       	mov    QWORD PTR [rsp+0x78],r13
   1800038d0:	77 17                	ja     0x1800038e9
   1800038d2:	c1 e0 04             	shl    eax,0x4
   1800038d5:	31 c9                	xor    ecx,ecx
   1800038d7:	48 8d 15 f2 56 03 00 	lea    rdx,[rip+0x356f2]        # 0x180038fd0
   1800038de:	80 3c 10 03          	cmp    BYTE PTR [rax+rdx*1],0x3
   1800038e2:	0f 94 c1             	sete   cl
   1800038e5:	01 4c 24 20          	add    DWORD PTR [rsp+0x20],ecx
   1800038e9:	8b 44 24 38          	mov    eax,DWORD PTR [rsp+0x38]
   1800038ed:	f2 4c 0f 2a c0       	cvtsi2sd xmm8,rax
   1800038f2:	8b 44 24 30          	mov    eax,DWORD PTR [rsp+0x30]
   1800038f6:	f2 4c 0f 2a e0       	cvtsi2sd xmm12,rax
   1800038fb:	4d 8d 66 08          	lea    r12,[r14+0x8]
   1800038ff:	4d 8d 7e 18          	lea    r15,[r14+0x18]
   180003903:	49 8d 46 10          	lea    rax,[r14+0x10]
   180003907:	48 89 84 24 18 01 00 	mov    QWORD PTR [rsp+0x118],rax
   18000390e:	00 
   18000390f:	4d 8d 46 28          	lea    r8,[r14+0x28]
   180003913:	49 8d 46 20          	lea    rax,[r14+0x20]
   180003917:	48 89 84 24 20 01 00 	mov    QWORD PTR [rsp+0x120],rax
   18000391e:	00 
   18000391f:	4d 8d 5e 38          	lea    r11,[r14+0x38]
   180003923:	49 8d 46 30          	lea    rax,[r14+0x30]
   180003927:	48 89 84 24 28 01 00 	mov    QWORD PTR [rsp+0x128],rax
   18000392e:	00 
   18000392f:	4d 8d 56 48          	lea    r10,[r14+0x48]
   180003933:	49 8d 46 40          	lea    rax,[r14+0x40]
   180003937:	48 89 84 24 30 01 00 	mov    QWORD PTR [rsp+0x130],rax
   18000393e:	00 
   18000393f:	49 8d 56 58          	lea    rdx,[r14+0x58]
   180003943:	49 8d 46 50          	lea    rax,[r14+0x50]
   180003947:	48 89 84 24 40 01 00 	mov    QWORD PTR [rsp+0x140],rax
   18000394e:	00 
   18000394f:	49 8d 46 68          	lea    rax,[r14+0x68]
   180003953:	48 89 84 24 50 01 00 	mov    QWORD PTR [rsp+0x150],rax
   18000395a:	00 
   18000395b:	49 8d 46 60          	lea    rax,[r14+0x60]
   18000395f:	48 89 44 24 50       	mov    QWORD PTR [rsp+0x50],rax
   180003964:	49 8d 46 78          	lea    rax,[r14+0x78]
   180003968:	48 89 84 24 48 01 00 	mov    QWORD PTR [rsp+0x148],rax
   18000396f:	00 
   180003970:	49 8d 46 70          	lea    rax,[r14+0x70]
   180003974:	48 89 44 24 70       	mov    QWORD PTR [rsp+0x70],rax
   180003979:	66 45 0f 57 ed       	xorpd  xmm13,xmm13
   18000397e:	f2 44 0f 10 0d e1 83 	movsd  xmm9,QWORD PTR [rip+0x383e1]        # 0x18003bd68
   180003985:	03 00 
   180003987:	f2 44 0f 10 35 e0 83 	movsd  xmm14,QWORD PTR [rip+0x383e0]        # 0x18003bd70
   18000398e:	03 00 
   180003990:	f2 44 0f 10 1d ef 83 	movsd  xmm11,QWORD PTR [rip+0x383ef]        # 0x18003bd88
   180003997:	03 00 
   180003999:	f2 44 0f 10 3d de 83 	movsd  xmm15,QWORD PTR [rip+0x383de]        # 0x18003bd80
   1800039a0:	03 00 
   1800039a2:	66 45 0f 57 d2       	xorpd  xmm10,xmm10
   1800039a7:	31 f6                	xor    esi,esi
   1800039a9:	48 8d 1d 20 ac 02 00 	lea    rbx,[rip+0x2ac20]        # 0x18002e5d0
   1800039b0:	4c 89 5c 24 60       	mov    QWORD PTR [rsp+0x60],r11
   1800039b5:	4c 89 54 24 48       	mov    QWORD PTR [rsp+0x48],r10
   1800039ba:	48 89 54 24 58       	mov    QWORD PTR [rsp+0x58],rdx
   1800039bf:	4c 89 a4 24 58 01 00 	mov    QWORD PTR [rsp+0x158],r12
   1800039c6:	00 
   1800039c7:	4c 89 bc 24 38 01 00 	mov    QWORD PTR [rsp+0x138],r15
   1800039ce:	00 
   1800039cf:	4c 89 44 24 68       	mov    QWORD PTR [rsp+0x68],r8
   1800039d4:	eb 1f                	jmp    0x1800039f5
   1800039d6:	66 2e 0f 1f 84 00 00 	cs nop WORD PTR [rax+rax*1+0x0]
   1800039dd:	00 00 00 
   1800039e0:	f2 44 0f 58 d0       	addsd  xmm10,xmm0
   1800039e5:	48 ff c6             	inc    rsi
   1800039e8:	48 81 fe 40 04 00 00 	cmp    rsi,0x440
   1800039ef:	0f 84 e8 06 00 00    	je     0x1800040dd
   1800039f5:	4c 8d 2c b6          	lea    r13,[rsi+rsi*4]
   1800039f9:	0f b6 45 14          	movzx  eax,BYTE PTR [rbp+0x14]
   1800039fd:	f2 0f 2a d0          	cvtsi2sd xmm2,eax
   180003a01:	42 0f b6 44 eb 06    	movzx  eax,BYTE PTR [rbx+r13*8+0x6]
   180003a07:	f2 0f 2a c0          	cvtsi2sd xmm0,eax
   180003a0b:	f2 0f 5c d0          	subsd  xmm2,xmm0
   180003a0f:	f2 0f 59 d2          	mulsd  xmm2,xmm2
   180003a13:	0f b6 45 15          	movzx  eax,BYTE PTR [rbp+0x15]
   180003a17:	0f 57 c9             	xorps  xmm1,xmm1
   180003a1a:	f2 0f 2a c8          	cvtsi2sd xmm1,eax
   180003a1e:	f2 41 0f 58 d5       	addsd  xmm2,xmm13
   180003a23:	0f b6 45 16          	movzx  eax,BYTE PTR [rbp+0x16]
   180003a27:	0f 57 c0             	xorps  xmm0,xmm0
   180003a2a:	f2 0f 2a c0          	cvtsi2sd xmm0,eax
   180003a2e:	0f b6 45 17          	movzx  eax,BYTE PTR [rbp+0x17]
   180003a32:	f2 0f 2a f8          	cvtsi2sd xmm7,eax
   180003a36:	83 bc 24 08 01 00 00 	cmp    DWORD PTR [rsp+0x108],0x2
   180003a3d:	02 
   180003a3e:	0f 85 dc 00 00 00    	jne    0x180003b20
   180003a44:	4a 8d 04 eb          	lea    rax,[rbx+r13*8]
   180003a48:	48 83 c0 06          	add    rax,0x6
   180003a4c:	0f b6 48 01          	movzx  ecx,BYTE PTR [rax+0x1]
   180003a50:	f2 0f 2a d9          	cvtsi2sd xmm3,ecx
   180003a54:	f2 0f 5c cb          	subsd  xmm1,xmm3
   180003a58:	f2 0f 59 c9          	mulsd  xmm1,xmm1
   180003a5c:	f2 0f 58 ca          	addsd  xmm1,xmm2
   180003a60:	0f b6 48 02          	movzx  ecx,BYTE PTR [rax+0x2]
   180003a64:	0f 57 d2             	xorps  xmm2,xmm2
   180003a67:	f2 0f 2a d1          	cvtsi2sd xmm2,ecx
   180003a6b:	f2 0f 5c c2          	subsd  xmm0,xmm2
   180003a6f:	f2 0f 59 c0          	mulsd  xmm0,xmm0
   180003a73:	0f b6 40 03          	movzx  eax,BYTE PTR [rax+0x3]
   180003a77:	0f 57 d2             	xorps  xmm2,xmm2
   180003a7a:	f2 0f 2a d0          	cvtsi2sd xmm2,eax
   180003a7e:	f2 0f 58 c1          	addsd  xmm0,xmm1
   180003a82:	f2 0f 5c fa          	subsd  xmm7,xmm2
   180003a86:	f2 0f 59 ff          	mulsd  xmm7,xmm7
   180003a8a:	f2 0f 58 f8          	addsd  xmm7,xmm0
   180003a8e:	f2 41 0f 5e f9       	divsd  xmm7,xmm9
   180003a93:	66 0f 57 c9          	xorpd  xmm1,xmm1
   180003a97:	66 0f 57 c0          	xorpd  xmm0,xmm0
   180003a9b:	83 7c 24 38 00       	cmp    DWORD PTR [rsp+0x38],0x0
   180003aa0:	74 1c                	je     0x180003abe
   180003aa2:	0f b6 84 34 a0 27 00 	movzx  eax,BYTE PTR [rsp+rsi*1+0x27a0]
   180003aa9:	00 
   180003aaa:	0f 57 c0             	xorps  xmm0,xmm0
   180003aad:	f2 0f 2a c0          	cvtsi2sd xmm0,eax
   180003ab1:	f2 0f 59 05 bf 82 03 	mulsd  xmm0,QWORD PTR [rip+0x382bf]        # 0x18003bd78
   180003ab8:	00 
   180003ab9:	f2 41 0f 5e c0       	divsd  xmm0,xmm8
   180003abe:	f2 41 0f 58 c6       	addsd  xmm0,xmm14
   180003ac3:	83 7c 24 30 00       	cmp    DWORD PTR [rsp+0x30],0x0
   180003ac8:	74 19                	je     0x180003ae3
   180003aca:	0f b6 84 34 60 23 00 	movzx  eax,BYTE PTR [rsp+rsi*1+0x2360]
   180003ad1:	00 
   180003ad2:	0f 57 c9             	xorps  xmm1,xmm1
   180003ad5:	f2 0f 2a c8          	cvtsi2sd xmm1,eax
   180003ad9:	f2 41 0f 59 cf       	mulsd  xmm1,xmm15
   180003ade:	f2 41 0f 5e cc       	divsd  xmm1,xmm12
   180003ae3:	49 89 d1             	mov    r9,rdx
   180003ae6:	f2 0f 58 c1          	addsd  xmm0,xmm1
   180003aea:	f2 41 0f 58 fe       	addsd  xmm7,xmm14
   180003aef:	66 0f 28 d7          	movapd xmm2,xmm7
   180003af3:	f2 0f 59 d7          	mulsd  xmm2,xmm7
   180003af7:	f2 0f 59 d7          	mulsd  xmm2,xmm7
   180003afb:	66 41 0f 28 ce       	movapd xmm1,xmm14
   180003b00:	f2 0f 5e ca          	divsd  xmm1,xmm2
   180003b04:	f2 0f 59 c8          	mulsd  xmm1,xmm0
   180003b08:	80 7d 19 00          	cmp    BYTE PTR [rbp+0x19],0x0
   180003b0c:	0f 85 89 01 00 00    	jne    0x180003c9b
   180003b12:	e9 9e 01 00 00       	jmp    0x180003cb5
   180003b17:	66 0f 1f 84 00 00 00 	nop    WORD PTR [rax+rax*1+0x0]
   180003b1e:	00 00 
   180003b20:	48 89 74 24 28       	mov    QWORD PTR [rsp+0x28],rsi
   180003b25:	42 0f b6 44 eb 07    	movzx  eax,BYTE PTR [rbx+r13*8+0x7]
   180003b2b:	f2 0f 2a d8          	cvtsi2sd xmm3,eax
   180003b2f:	f2 0f 5c cb          	subsd  xmm1,xmm3
   180003b33:	f2 0f 59 c9          	mulsd  xmm1,xmm1
   180003b37:	42 0f b6 44 eb 08    	movzx  eax,BYTE PTR [rbx+r13*8+0x8]
   180003b3d:	0f 57 db             	xorps  xmm3,xmm3
   180003b40:	f2 0f 2a d8          	cvtsi2sd xmm3,eax
   180003b44:	f2 0f 58 ca          	addsd  xmm1,xmm2
   180003b48:	f2 0f 5c c3          	subsd  xmm0,xmm3
   180003b4c:	f2 0f 59 c0          	mulsd  xmm0,xmm0
   180003b50:	f2 0f 58 c1          	addsd  xmm0,xmm1
   180003b54:	42 0f b6 44 eb 09    	movzx  eax,BYTE PTR [rbx+r13*8+0x9]
   180003b5a:	0f 57 c9             	xorps  xmm1,xmm1
   180003b5d:	f2 0f 2a c8          	cvtsi2sd xmm1,eax
   180003b61:	f2 0f 5c f9          	subsd  xmm7,xmm1
   180003b65:	f2 0f 59 ff          	mulsd  xmm7,xmm7
   180003b69:	f2 0f 58 f8          	addsd  xmm7,xmm0
   180003b6d:	f2 41 0f 5e f9       	divsd  xmm7,xmm9
   180003b72:	f2 41 0f 58 fe       	addsd  xmm7,xmm14
   180003b77:	66 0f 28 f7          	movapd xmm6,xmm7
   180003b7b:	f2 0f 59 f7          	mulsd  xmm6,xmm7
   180003b7f:	4e 8d 24 eb          	lea    r12,[rbx+r13*8]
   180003b83:	49 83 c4 0e          	add    r12,0xe
   180003b87:	45 31 ff             	xor    r15d,r15d
   180003b8a:	bf 01 00 00 00       	mov    edi,0x1
   180003b8f:	31 db                	xor    ebx,ebx
   180003b91:	31 f6                	xor    esi,esi
   180003b93:	45 31 f6             	xor    r14d,r14d
   180003b96:	eb 13                	jmp    0x180003bab
   180003b98:	41 ff c7             	inc    r15d
   180003b9b:	01 ce                	add    esi,ecx
   180003b9d:	48 8b 6c 24 40       	mov    rbp,QWORD PTR [rsp+0x40]
   180003ba2:	48 ff c7             	inc    rdi
   180003ba5:	48 83 ff 0a          	cmp    rdi,0xa
   180003ba9:	74 45                	je     0x180003bf0
   180003bab:	0f b7 54 7d 00       	movzx  edx,WORD PTR [rbp+rdi*2+0x0]
   180003bb0:	85 d2                	test   edx,edx
   180003bb2:	74 ee                	je     0x180003ba2
   180003bb4:	81 fa ff ff 00 00    	cmp    edx,0xffff
   180003bba:	74 e6                	je     0x180003ba2
   180003bbc:	48 8b 4c 24 78       	mov    rcx,QWORD PTR [rsp+0x78]
   180003bc1:	e8 fa d6 ff ff       	call   0x1800012c0
   180003bc6:	48 89 e9             	mov    rcx,rbp
   180003bc9:	89 c5                	mov    ebp,eax
   180003bcb:	0f b7 14 79          	movzx  edx,WORD PTR [rcx+rdi*2]
   180003bcf:	4c 89 e1             	mov    rcx,r12
   180003bd2:	e8 e9 d6 ff ff       	call   0x1800012c0
   180003bd7:	31 c9                	xor    ecx,ecx
   180003bd9:	85 c0                	test   eax,eax
   180003bdb:	0f 95 c1             	setne  cl
   180003bde:	85 ed                	test   ebp,ebp
   180003be0:	74 b6                	je     0x180003b98
   180003be2:	ff c3                	inc    ebx
   180003be4:	41 01 ce             	add    r14d,ecx
   180003be7:	eb b4                	jmp    0x180003b9d
   180003be9:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
   180003bf0:	66 41 0f 28 c6       	movapd xmm0,xmm14
   180003bf5:	85 db                	test   ebx,ebx
   180003bf7:	74 26                	je     0x180003c1f
   180003bf9:	44 89 f0             	mov    eax,r14d
   180003bfc:	0f 57 c0             	xorps  xmm0,xmm0
   180003bff:	f2 48 0f 2a c0       	cvtsi2sd xmm0,rax
   180003c04:	f2 0f 59 05 6c 81 03 	mulsd  xmm0,QWORD PTR [rip+0x3816c]        # 0x18003bd78
   180003c0b:	00 
   180003c0c:	89 d8                	mov    eax,ebx
   180003c0e:	0f 57 c9             	xorps  xmm1,xmm1
   180003c11:	f2 48 0f 2a c8       	cvtsi2sd xmm1,rax
   180003c16:	f2 0f 5e c1          	divsd  xmm0,xmm1
   180003c1a:	f2 41 0f 58 c6       	addsd  xmm0,xmm14
   180003c1f:	89 f0                	mov    eax,esi
   180003c21:	0f 57 d2             	xorps  xmm2,xmm2
   180003c24:	f2 48 0f 2a d0       	cvtsi2sd xmm2,rax
   180003c29:	f2 41 0f 59 d7       	mulsd  xmm2,xmm15
   180003c2e:	45 85 ff             	test   r15d,r15d
   180003c31:	4c 8b a4 24 58 01 00 	mov    r12,QWORD PTR [rsp+0x158]
   180003c38:	00 
   180003c39:	4c 8b 44 24 68       	mov    r8,QWORD PTR [rsp+0x68]
   180003c3e:	74 11                	je     0x180003c51
   180003c40:	44 89 f8             	mov    eax,r15d
   180003c43:	0f 57 c9             	xorps  xmm1,xmm1
   180003c46:	f2 48 0f 2a c8       	cvtsi2sd xmm1,rax
   180003c4b:	f2 0f 5e d1          	divsd  xmm2,xmm1
   180003c4f:	eb 04                	jmp    0x180003c55
   180003c51:	66 0f 57 d2          	xorpd  xmm2,xmm2
   180003c55:	4c 8b b4 24 f8 2c 00 	mov    r14,QWORD PTR [rsp+0x2cf8]
   180003c5c:	00 
   180003c5d:	48 8d 1d 6c a9 02 00 	lea    rbx,[rip+0x2a96c]        # 0x18002e5d0
   180003c64:	4c 8b bc 24 38 01 00 	mov    r15,QWORD PTR [rsp+0x138]
   180003c6b:	00 
   180003c6c:	4c 8b 5c 24 60       	mov    r11,QWORD PTR [rsp+0x60]
   180003c71:	4c 8b 54 24 48       	mov    r10,QWORD PTR [rsp+0x48]
   180003c76:	4c 8b 4c 24 58       	mov    r9,QWORD PTR [rsp+0x58]
   180003c7b:	48 8b 74 24 28       	mov    rsi,QWORD PTR [rsp+0x28]
   180003c80:	f2 0f 58 d0          	addsd  xmm2,xmm0
   180003c84:	f2 0f 59 fe          	mulsd  xmm7,xmm6
   180003c88:	66 41 0f 28 ce       	movapd xmm1,xmm14
   180003c8d:	f2 0f 5e cf          	divsd  xmm1,xmm7
   180003c91:	f2 0f 59 ca          	mulsd  xmm1,xmm2
   180003c95:	80 7d 19 00          	cmp    BYTE PTR [rbp+0x19],0x0
   180003c99:	74 1a                	je     0x180003cb5
   180003c9b:	66 41 0f 28 c6       	movapd xmm0,xmm14
   180003ca0:	f2 0f 5d c1          	minsd  xmm0,xmm1
   180003ca4:	f2 41 0f 59 c3       	mulsd  xmm0,xmm11
   180003ca9:	f2 0f 58 05 df 80 03 	addsd  xmm0,QWORD PTR [rip+0x380df]        # 0x18003bd90
   180003cb0:	00 
   180003cb1:	f2 0f 59 c8          	mulsd  xmm1,xmm0
   180003cb5:	4a 8d 04 eb          	lea    rax,[rbx+r13*8]
   180003cb9:	31 c9                	xor    ecx,ecx
   180003cbb:	42 80 7c eb 0c 02    	cmp    BYTE PTR [rbx+r13*8+0xc],0x2
   180003cc1:	0f 94 c2             	sete   dl
   180003cc4:	0f b7 00             	movzx  eax,WORD PTR [rax]
   180003cc7:	66 0f 57 c0          	xorpd  xmm0,xmm0
   180003ccb:	66 3b 45 00          	cmp    ax,WORD PTR [rbp+0x0]
   180003ccf:	74 12                	je     0x180003ce3
   180003cd1:	88 d1                	mov    cl,dl
   180003cd3:	48 8d 15 c6 80 03 00 	lea    rdx,[rip+0x380c6]        # 0x18003bda0
   180003cda:	f2 0f 59 0c ca       	mulsd  xmm1,QWORD PTR [rdx+rcx*8]
   180003cdf:	66 0f 28 c1          	movapd xmm0,xmm1
   180003ce3:	f2 0f 11 84 f4 60 01 	movsd  QWORD PTR [rsp+rsi*8+0x160],xmm0
   180003cea:	00 00 
   180003cec:	f2 41 0f 10 0c 24    	movsd  xmm1,QWORD PTR [r12]
   180003cf2:	b2 01                	mov    dl,0x1
   180003cf4:	66 0f 2e c1          	ucomisd xmm0,xmm1
   180003cf8:	76 26                	jbe    0x180003d20
   180003cfa:	49 89 f1             	mov    r9,rsi
   180003cfd:	40 b6 01             	mov    sil,0x1
   180003d00:	41 b3 01             	mov    r11b,0x1
   180003d03:	41 b2 01             	mov    r10b,0x1
   180003d06:	b1 01                	mov    cl,0x1
   180003d08:	89 4c 24 28          	mov    DWORD PTR [rsp+0x28],ecx
   180003d0c:	31 c9                	xor    ecx,ecx
   180003d0e:	4c 89 e7             	mov    rdi,r12
   180003d11:	4d 89 e0             	mov    r8,r12
   180003d14:	e9 e7 00 00 00       	jmp    0x180003e00
   180003d19:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
   180003d20:	b9 01 00 00 00       	mov    ecx,0x1
   180003d25:	66 41 0f 2e 07       	ucomisd xmm0,QWORD PTR [r15]
   180003d2a:	76 24                	jbe    0x180003d50
   180003d2c:	4c 89 e7             	mov    rdi,r12
   180003d2f:	4d 89 f8             	mov    r8,r15
   180003d32:	49 89 f1             	mov    r9,rsi
   180003d35:	40 b6 01             	mov    sil,0x1
   180003d38:	41 b3 01             	mov    r11b,0x1
   180003d3b:	41 b2 01             	mov    r10b,0x1
   180003d3e:	41 b7 01             	mov    r15b,0x1
   180003d41:	44 89 7c 24 28       	mov    DWORD PTR [rsp+0x28],r15d
   180003d46:	4d 89 c7             	mov    r15,r8
   180003d49:	e9 b2 00 00 00       	jmp    0x180003e00
   180003d4e:	66 90                	xchg   ax,ax
   180003d50:	66 41 0f 2e 00       	ucomisd xmm0,QWORD PTR [r8]
   180003d55:	76 21                	jbe    0x180003d78
   180003d57:	4c 89 e7             	mov    rdi,r12
   180003d5a:	49 89 f1             	mov    r9,rsi
   180003d5d:	b9 02 00 00 00       	mov    ecx,0x2
   180003d62:	c7 44 24 28 00 00 00 	mov    DWORD PTR [rsp+0x28],0x0
   180003d69:	00 
   180003d6a:	40 b6 01             	mov    sil,0x1
   180003d6d:	41 b3 01             	mov    r11b,0x1
   180003d70:	41 b2 01             	mov    r10b,0x1
   180003d73:	e9 88 00 00 00       	jmp    0x180003e00
   180003d78:	66 41 0f 2e 03       	ucomisd xmm0,QWORD PTR [r11]
   180003d7d:	76 19                	jbe    0x180003d98
   180003d7f:	4c 89 e7             	mov    rdi,r12
   180003d82:	49 89 f1             	mov    r9,rsi
   180003d85:	b9 03 00 00 00       	mov    ecx,0x3
   180003d8a:	45 31 d2             	xor    r10d,r10d
   180003d8d:	4d 89 d8             	mov    r8,r11
   180003d90:	40 b6 01             	mov    sil,0x1
   180003d93:	41 b3 01             	mov    r11b,0x1
   180003d96:	eb 59                	jmp    0x180003df1
   180003d98:	66 41 0f 2e 02       	ucomisd xmm0,QWORD PTR [r10]
   180003d9d:	76 16                	jbe    0x180003db5
   180003d9f:	4c 89 e7             	mov    rdi,r12
   180003da2:	49 89 f1             	mov    r9,rsi
   180003da5:	b9 04 00 00 00       	mov    ecx,0x4
   180003daa:	45 31 db             	xor    r11d,r11d
   180003dad:	4d 89 d0             	mov    r8,r10
   180003db0:	40 b6 01             	mov    sil,0x1
   180003db3:	eb 39                	jmp    0x180003dee
   180003db5:	4c 89 e7             	mov    rdi,r12
   180003db8:	66 41 0f 2e 01       	ucomisd xmm0,QWORD PTR [r9]
   180003dbd:	76 0d                	jbe    0x180003dcc
   180003dbf:	4d 89 c8             	mov    r8,r9
   180003dc2:	49 89 f1             	mov    r9,rsi
   180003dc5:	b9 05 00 00 00       	mov    ecx,0x5
   180003dca:	eb 1d                	jmp    0x180003de9
   180003dcc:	4c 8b 84 24 50 01 00 	mov    r8,QWORD PTR [rsp+0x150]
   180003dd3:	00 
   180003dd4:	66 41 0f 2e 00       	ucomisd xmm0,QWORD PTR [r8]
   180003dd9:	0f 86 db 02 00 00    	jbe    0x1800040ba
   180003ddf:	49 89 f1             	mov    r9,rsi
   180003de2:	b9 06 00 00 00       	mov    ecx,0x6
   180003de7:	31 d2                	xor    edx,edx
   180003de9:	31 f6                	xor    esi,esi
   180003deb:	45 31 db             	xor    r11d,r11d
   180003dee:	45 31 d2             	xor    r10d,r10d
   180003df1:	c7 44 24 28 00 00 00 	mov    DWORD PTR [rsp+0x28],0x0
   180003df8:	00 
   180003df9:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
   180003e00:	4c 8b 64 24 50       	mov    r12,QWORD PTR [rsp+0x50]
   180003e05:	66 41 0f 10 14 24    	movupd xmm2,XMMWORD PTR [r12]
   180003e0b:	4c 8b 64 24 70       	mov    r12,QWORD PTR [rsp+0x70]
   180003e10:	66 41 0f 11 14 24    	movupd XMMWORD PTR [r12],xmm2
   180003e16:	84 d2                	test   dl,dl
   180003e18:	0f 84 b2 00 00 00    	je     0x180003ed0
   180003e1e:	48 8b 94 24 40 01 00 	mov    rdx,QWORD PTR [rsp+0x140]
   180003e25:	00 
   180003e26:	66 0f 10 12          	movupd xmm2,XMMWORD PTR [rdx]
   180003e2a:	48 8b 54 24 50       	mov    rdx,QWORD PTR [rsp+0x50]
   180003e2f:	66 0f 11 12          	movupd XMMWORD PTR [rdx],xmm2
   180003e33:	40 84 f6             	test   sil,sil
   180003e36:	0f 84 94 00 00 00    	je     0x180003ed0
   180003e3c:	48 8b 94 24 30 01 00 	mov    rdx,QWORD PTR [rsp+0x130]
   180003e43:	00 
   180003e44:	66 0f 10 12          	movupd xmm2,XMMWORD PTR [rdx]
   180003e48:	48 8b 94 24 40 01 00 	mov    rdx,QWORD PTR [rsp+0x140]
   180003e4f:	00 
   180003e50:	66 0f 11 12          	movupd XMMWORD PTR [rdx],xmm2
   180003e54:	45 84 db             	test   r11b,r11b
   180003e57:	74 77                	je     0x180003ed0
   180003e59:	48 8b 94 24 28 01 00 	mov    rdx,QWORD PTR [rsp+0x128]
   180003e60:	00 
   180003e61:	66 0f 10 12          	movupd xmm2,XMMWORD PTR [rdx]
   180003e65:	48 8b 94 24 30 01 00 	mov    rdx,QWORD PTR [rsp+0x130]
   180003e6c:	00 
   180003e6d:	66 0f 11 12          	movupd XMMWORD PTR [rdx],xmm2
   180003e71:	45 84 d2             	test   r10b,r10b
   180003e74:	74 5a                	je     0x180003ed0
   180003e76:	48 8b 94 24 20 01 00 	mov    rdx,QWORD PTR [rsp+0x120]
   180003e7d:	00 
   180003e7e:	66 0f 10 12          	movupd xmm2,XMMWORD PTR [rdx]
   180003e82:	48 8b 94 24 28 01 00 	mov    rdx,QWORD PTR [rsp+0x128]
   180003e89:	00 
   180003e8a:	66 0f 11 12          	movupd XMMWORD PTR [rdx],xmm2
   180003e8e:	80 7c 24 28 00       	cmp    BYTE PTR [rsp+0x28],0x0
   180003e93:	74 3b                	je     0x180003ed0
   180003e95:	66 0f 2e c1          	ucomisd xmm0,xmm1
   180003e99:	48 8b 94 24 18 01 00 	mov    rdx,QWORD PTR [rsp+0x118]
   180003ea0:	00 
   180003ea1:	66 0f 10 0a          	movupd xmm1,XMMWORD PTR [rdx]
   180003ea5:	48 8b 94 24 20 01 00 	mov    rdx,QWORD PTR [rsp+0x120]
   180003eac:	00 
   180003ead:	66 0f 11 0a          	movupd XMMWORD PTR [rdx],xmm1
   180003eb1:	76 1d                	jbe    0x180003ed0
   180003eb3:	66 41 0f 10 0e       	movupd xmm1,XMMWORD PTR [r14]
   180003eb8:	48 8b 94 24 18 01 00 	mov    rdx,QWORD PTR [rsp+0x118]
   180003ebf:	00 
   180003ec0:	66 0f 11 0a          	movupd XMMWORD PTR [rdx],xmm1
   180003ec4:	66 66 66 2e 0f 1f 84 	data16 data16 cs nop WORD PTR [rax+rax*1+0x0]
   180003ecb:	00 00 00 00 00 
   180003ed0:	c1 e1 04             	shl    ecx,0x4
   180003ed3:	4c 01 f1             	add    rcx,r14
   180003ed6:	4c 8b 5c 24 60       	mov    r11,QWORD PTR [rsp+0x60]
   180003edb:	4c 8b 54 24 48       	mov    r10,QWORD PTR [rsp+0x48]
   180003ee0:	48 8b 54 24 58       	mov    rdx,QWORD PTR [rsp+0x58]
   180003ee5:	4c 89 ce             	mov    rsi,r9
   180003ee8:	49 89 fc             	mov    r12,rdi
   180003eeb:	66 89 01             	mov    WORD PTR [rcx],ax
   180003eee:	f2 41 0f 11 00       	movsd  QWORD PTR [r8],xmm0
   180003ef3:	42 0f b7 44 eb 0e    	movzx  eax,WORD PTR [rbx+r13*8+0xe]
   180003ef9:	8d 48 ff             	lea    ecx,[rax-0x1]
   180003efc:	0f b7 c9             	movzx  ecx,cx
   180003eff:	81 f9 61 01 00 00    	cmp    ecx,0x161
   180003f05:	77 1d                	ja     0x180003f24
   180003f07:	f2 0f 10 8c f4 60 01 	movsd  xmm1,QWORD PTR [rsp+rsi*8+0x160]
   180003f0e:	00 00 
   180003f10:	f2 41 0f 58 8c c6 00 	addsd  xmm1,QWORD PTR [r14+rax*8+0x100]
   180003f17:	01 00 00 
   180003f1a:	f2 41 0f 11 8c c6 00 	movsd  QWORD PTR [r14+rax*8+0x100],xmm1
   180003f21:	01 00 00 
   180003f24:	42 0f b7 44 eb 10    	movzx  eax,WORD PTR [rbx+r13*8+0x10]
   180003f2a:	8d 48 ff             	lea    ecx,[rax-0x1]
   180003f2d:	0f b7 c9             	movzx  ecx,cx
   180003f30:	81 f9 61 01 00 00    	cmp    ecx,0x161
   180003f36:	4c 8b 44 24 68       	mov    r8,QWORD PTR [rsp+0x68]
   180003f3b:	77 1d                	ja     0x180003f5a
   180003f3d:	f2 0f 10 8c f4 60 01 	movsd  xmm1,QWORD PTR [rsp+rsi*8+0x160]
   180003f44:	00 00 
   180003f46:	f2 41 0f 58 8c c6 00 	addsd  xmm1,QWORD PTR [r14+rax*8+0x100]
   180003f4d:	01 00 00 
   180003f50:	f2 41 0f 11 8c c6 00 	movsd  QWORD PTR [r14+rax*8+0x100],xmm1
   180003f57:	01 00 00 
   180003f5a:	42 0f b7 44 eb 12    	movzx  eax,WORD PTR [rbx+r13*8+0x12]
   180003f60:	8d 48 ff             	lea    ecx,[rax-0x1]
   180003f63:	0f b7 c9             	movzx  ecx,cx
   180003f66:	81 f9 61 01 00 00    	cmp    ecx,0x161
   180003f6c:	77 1d                	ja     0x180003f8b
   180003f6e:	f2 0f 10 8c f4 60 01 	movsd  xmm1,QWORD PTR [rsp+rsi*8+0x160]
   180003f75:	00 00 
   180003f77:	f2 41 0f 58 8c c6 00 	addsd  xmm1,QWORD PTR [r14+rax*8+0x100]
   180003f7e:	01 00 00 
   180003f81:	f2 41 0f 11 8c c6 00 	movsd  QWORD PTR [r14+rax*8+0x100],xmm1
   180003f88:	01 00 00 
   180003f8b:	42 0f b7 44 eb 14    	movzx  eax,WORD PTR [rbx+r13*8+0x14]
   180003f91:	8d 48 ff             	lea    ecx,[rax-0x1]
   180003f94:	0f b7 c9             	movzx  ecx,cx
   180003f97:	81 f9 61 01 00 00    	cmp    ecx,0x161
   180003f9d:	77 1d                	ja     0x180003fbc
   180003f9f:	f2 0f 10 8c f4 60 01 	movsd  xmm1,QWORD PTR [rsp+rsi*8+0x160]
   180003fa6:	00 00 
   180003fa8:	f2 41 0f 58 8c c6 00 	addsd  xmm1,QWORD PTR [r14+rax*8+0x100]
   180003faf:	01 00 00 
   180003fb2:	f2 41 0f 11 8c c6 00 	movsd  QWORD PTR [r14+rax*8+0x100],xmm1
   180003fb9:	01 00 00 
   180003fbc:	42 0f b7 44 eb 16    	movzx  eax,WORD PTR [rbx+r13*8+0x16]
   180003fc2:	8d 48 ff             	lea    ecx,[rax-0x1]
   180003fc5:	0f b7 c9             	movzx  ecx,cx
   180003fc8:	81 f9 61 01 00 00    	cmp    ecx,0x161
   180003fce:	77 1d                	ja     0x180003fed
   180003fd0:	f2 0f 10 8c f4 60 01 	movsd  xmm1,QWORD PTR [rsp+rsi*8+0x160]
   180003fd7:	00 00 
   180003fd9:	f2 41 0f 58 8c c6 00 	addsd  xmm1,QWORD PTR [r14+rax*8+0x100]
   180003fe0:	01 00 00 
   180003fe3:	f2 41 0f 11 8c c6 00 	movsd  QWORD PTR [r14+rax*8+0x100],xmm1
   180003fea:	01 00 00 
   180003fed:	42 0f b7 44 eb 18    	movzx  eax,WORD PTR [rbx+r13*8+0x18]
   180003ff3:	8d 48 ff             	lea    ecx,[rax-0x1]
   180003ff6:	0f b7 c9             	movzx  ecx,cx
   180003ff9:	81 f9 61 01 00 00    	cmp    ecx,0x161
   180003fff:	77 1d                	ja     0x18000401e
   180004001:	f2 0f 10 8c f4 60 01 	movsd  xmm1,QWORD PTR [rsp+rsi*8+0x160]
   180004008:	00 00 
   18000400a:	f2 41 0f 58 8c c6 00 	addsd  xmm1,QWORD PTR [r14+rax*8+0x100]
   180004011:	01 00 00 
   180004014:	f2 41 0f 11 8c c6 00 	movsd  QWORD PTR [r14+rax*8+0x100],xmm1
   18000401b:	01 00 00 
   18000401e:	42 0f b7 44 eb 1a    	movzx  eax,WORD PTR [rbx+r13*8+0x1a]
   180004024:	8d 48 ff             	lea    ecx,[rax-0x1]
   180004027:	0f b7 c9             	movzx  ecx,cx
   18000402a:	81 f9 61 01 00 00    	cmp    ecx,0x161
   180004030:	77 1d                	ja     0x18000404f
   180004032:	f2 0f 10 8c f4 60 01 	movsd  xmm1,QWORD PTR [rsp+rsi*8+0x160]
   180004039:	00 00 
   18000403b:	f2 41 0f 58 8c c6 00 	addsd  xmm1,QWORD PTR [r14+rax*8+0x100]
   180004042:	01 00 00 
   180004045:	f2 41 0f 11 8c c6 00 	movsd  QWORD PTR [r14+rax*8+0x100],xmm1
   18000404c:	01 00 00 
   18000404f:	42 0f b7 44 eb 1c    	movzx  eax,WORD PTR [rbx+r13*8+0x1c]
   180004055:	8d 48 ff             	lea    ecx,[rax-0x1]
   180004058:	0f b7 c9             	movzx  ecx,cx
   18000405b:	81 f9 61 01 00 00    	cmp    ecx,0x161
   180004061:	77 1d                	ja     0x180004080
   180004063:	f2 0f 10 8c f4 60 01 	movsd  xmm1,QWORD PTR [rsp+rsi*8+0x160]
   18000406a:	00 00 
   18000406c:	f2 41 0f 58 8c c6 00 	addsd  xmm1,QWORD PTR [r14+rax*8+0x100]
   180004073:	01 00 00 
   180004076:	f2 41 0f 11 8c c6 00 	movsd  QWORD PTR [r14+rax*8+0x100],xmm1
   18000407d:	01 00 00 
   180004080:	42 0f b7 44 eb 1e    	movzx  eax,WORD PTR [rbx+r13*8+0x1e]
   180004086:	8d 48 ff             	lea    ecx,[rax-0x1]
   180004089:	0f b7 c9             	movzx  ecx,cx
   18000408c:	81 f9 61 01 00 00    	cmp    ecx,0x161
   180004092:	0f 87 48 f9 ff ff    	ja     0x1800039e0
   180004098:	f2 0f 10 8c f4 60 01 	movsd  xmm1,QWORD PTR [rsp+rsi*8+0x160]
   18000409f:	00 00 
   1800040a1:	f2 41 0f 58 8c c6 00 	addsd  xmm1,QWORD PTR [r14+rax*8+0x100]
   1800040a8:	01 00 00 
   1800040ab:	f2 41 0f 11 8c c6 00 	movsd  QWORD PTR [r14+rax*8+0x100],xmm1
   1800040b2:	01 00 00 
   1800040b5:	e9 26 f9 ff ff       	jmp    0x1800039e0
   1800040ba:	4c 8b 84 24 48 01 00 	mov    r8,QWORD PTR [rsp+0x148]
   1800040c1:	00 
   1800040c2:	66 41 0f 2e 00       	ucomisd xmm0,QWORD PTR [r8]
   1800040c7:	48 8b 4c 24 70       	mov    rcx,QWORD PTR [rsp+0x70]
   1800040cc:	49 89 fc             	mov    r12,rdi
   1800040cf:	4c 89 ca             	mov    rdx,r9
   1800040d2:	0f 87 13 fe ff ff    	ja     0x180003eeb
   1800040d8:	e9 16 fe ff ff       	jmp    0x180003ef3
   1800040dd:	41 0f b7 06          	movzx  eax,WORD PTR [r14]
   1800040e1:	66 85 c0             	test   ax,ax
   1800040e4:	0f 84 6e 01 00 00    	je     0x180004258
   1800040ea:	ff c8                	dec    eax
   1800040ec:	0f b7 c0             	movzx  eax,ax
   1800040ef:	48 8d 04 80          	lea    rax,[rax+rax*4]
   1800040f3:	48 8d 14 c3          	lea    rdx,[rbx+rax*8]
   1800040f7:	48 83 c2 06          	add    rdx,0x6
   1800040fb:	49 8d 8e a0 22 00 00 	lea    rcx,[r14+0x22a0]
   180004102:	41 b8 05 00 00 00    	mov    r8d,0x5
   180004108:	e8 f3 ce ff ff       	call   0x180001000
   18000410d:	41 0f b7 46 10       	movzx  eax,WORD PTR [r14+0x10]
   180004112:	66 85 c0             	test   ax,ax
   180004115:	0f 84 3d 01 00 00    	je     0x180004258
   18000411b:	ff c8                	dec    eax
   18000411d:	0f b7 c0             	movzx  eax,ax
   180004120:	48 8d 04 80          	lea    rax,[rax+rax*4]
   180004124:	48 8d 14 c3          	lea    rdx,[rbx+rax*8]
   180004128:	48 83 c2 06          	add    rdx,0x6
   18000412c:	49 8d 8e a5 22 00 00 	lea    rcx,[r14+0x22a5]
   180004133:	41 b8 05 00 00 00    	mov    r8d,0x5
   180004139:	e8 c2 ce ff ff       	call   0x180001000
   18000413e:	41 0f b7 46 20       	movzx  eax,WORD PTR [r14+0x20]
   180004143:	66 85 c0             	test   ax,ax
   180004146:	0f 84 0c 01 00 00    	je     0x180004258
   18000414c:	ff c8                	dec    eax
   18000414e:	0f b7 c0             	movzx  eax,ax
   180004151:	48 8d 04 80          	lea    rax,[rax+rax*4]
   180004155:	48 8d 14 c3          	lea    rdx,[rbx+rax*8]
   180004159:	48 83 c2 06          	add    rdx,0x6
   18000415d:	49 8d 8e aa 22 00 00 	lea    rcx,[r14+0x22aa]
   180004164:	41 b8 05 00 00 00    	mov    r8d,0x5
   18000416a:	e8 91 ce ff ff       	call   0x180001000
   18000416f:	41 0f b7 46 30       	movzx  eax,WORD PTR [r14+0x30]
   180004174:	66 85 c0             	test   ax,ax
   180004177:	0f 84 db 00 00 00    	je     0x180004258
   18000417d:	ff c8                	dec    eax
   18000417f:	0f b7 c0             	movzx  eax,ax
   180004182:	48 8d 04 80          	lea    rax,[rax+rax*4]
   180004186:	48 8d 14 c3          	lea    rdx,[rbx+rax*8]
   18000418a:	48 83 c2 06          	add    rdx,0x6
   18000418e:	49 8d 8e af 22 00 00 	lea    rcx,[r14+0x22af]
   180004195:	41 b8 05 00 00 00    	mov    r8d,0x5
   18000419b:	e8 60 ce ff ff       	call   0x180001000
   1800041a0:	41 0f b7 46 40       	movzx  eax,WORD PTR [r14+0x40]
   1800041a5:	66 85 c0             	test   ax,ax
   1800041a8:	0f 84 aa 00 00 00    	je     0x180004258
   1800041ae:	ff c8                	dec    eax
   1800041b0:	0f b7 c0             	movzx  eax,ax
   1800041b3:	48 8d 04 80          	lea    rax,[rax+rax*4]
   1800041b7:	48 8d 14 c3          	lea    rdx,[rbx+rax*8]
   1800041bb:	48 83 c2 06          	add    rdx,0x6
   1800041bf:	49 8d 8e b4 22 00 00 	lea    rcx,[r14+0x22b4]
   1800041c6:	41 b8 05 00 00 00    	mov    r8d,0x5
   1800041cc:	e8 2f ce ff ff       	call   0x180001000
   1800041d1:	41 0f b7 46 50       	movzx  eax,WORD PTR [r14+0x50]
   1800041d6:	66 85 c0             	test   ax,ax
   1800041d9:	74 7d                	je     0x180004258
   1800041db:	ff c8                	dec    eax
   1800041dd:	0f b7 c0             	movzx  eax,ax
   1800041e0:	48 8d 04 80          	lea    rax,[rax+rax*4]
   1800041e4:	48 8d 14 c3          	lea    rdx,[rbx+rax*8]
   1800041e8:	48 83 c2 06          	add    rdx,0x6
   1800041ec:	49 8d 8e b9 22 00 00 	lea    rcx,[r14+0x22b9]
   1800041f3:	41 b8 05 00 00 00    	mov    r8d,0x5
   1800041f9:	e8 02 ce ff ff       	call   0x180001000
   1800041fe:	41 0f b7 46 60       	movzx  eax,WORD PTR [r14+0x60]
   180004203:	66 85 c0             	test   ax,ax
   180004206:	74 50                	je     0x180004258
   180004208:	ff c8                	dec    eax
   18000420a:	0f b7 c0             	movzx  eax,ax
   18000420d:	48 8d 04 80          	lea    rax,[rax+rax*4]
   180004211:	48 8d 14 c3          	lea    rdx,[rbx+rax*8]
   180004215:	48 83 c2 06          	add    rdx,0x6
   180004219:	49 8d 8e be 22 00 00 	lea    rcx,[r14+0x22be]
   180004220:	41 b8 05 00 00 00    	mov    r8d,0x5
   180004226:	e8 d5 cd ff ff       	call   0x180001000
   18000422b:	41 0f b7 46 70       	movzx  eax,WORD PTR [r14+0x70]
   180004230:	66 85 c0             	test   ax,ax
   180004233:	74 23                	je     0x180004258
   180004235:	ff c8                	dec    eax
   180004237:	0f b7 c0             	movzx  eax,ax
   18000423a:	48 8d 04 80          	lea    rax,[rax+rax*4]
   18000423e:	48 8d 14 c3          	lea    rdx,[rbx+rax*8]
   180004242:	48 83 c2 06          	add    rdx,0x6
   180004246:	49 8d 8e c3 22 00 00 	lea    rcx,[r14+0x22c3]
   18000424d:	41 b8 05 00 00 00    	mov    r8d,0x5
   180004253:	e8 a8 cd ff ff       	call   0x180001000
   180004258:	b8 fe ff ff ff       	mov    eax,0xfffffffe
   18000425d:	0f 57 c0             	xorps  xmm0,xmm0
   180004260:	66 41 0f 2e c2       	ucomisd xmm0,xmm10
   180004265:	0f 83 3e f3 ff ff    	jae    0x1800035a9
   18000426b:	b8 05 00 00 00       	mov    eax,0x5
   180004270:	48 8d 0d 59 65 03 00 	lea    rcx,[rip+0x36559]        # 0x18003a7d0
   180004277:	f2 44 0f 10 2d f0 7a 	movsd  xmm13,QWORD PTR [rip+0x37af0]        # 0x18003bd70
   18000427e:	03 00 
   180004280:	eb 1e                	jmp    0x1800042a0
   180004282:	66 66 66 66 66 2e 0f 	data16 data16 data16 data16 cs nop WORD PTR [rax+rax*1+0x0]
   180004289:	1f 84 00 00 00 00 00 
   180004290:	48 83 c0 10          	add    rax,0x10
   180004294:	48 3d c5 02 00 00    	cmp    rax,0x2c5
   18000429a:	0f 84 ee 00 00 00    	je     0x18000438e
   1800042a0:	0f b7 54 08 fb       	movzx  edx,WORD PTR [rax+rcx*1-0x5]
   1800042a5:	44 8d 42 ff          	lea    r8d,[rdx-0x1]
   1800042a9:	45 0f b7 c0          	movzx  r8d,r8w
   1800042ad:	41 81 f8 61 01 00 00 	cmp    r8d,0x161
   1800042b4:	77 da                	ja     0x180004290
   1800042b6:	44 0f b6 45 17       	movzx  r8d,BYTE PTR [rbp+0x17]
   1800042bb:	0f 57 c9             	xorps  xmm1,xmm1
   1800042be:	f2 41 0f 2a c8       	cvtsi2sd xmm1,r8d
   1800042c3:	44 0f b6 04 08       	movzx  r8d,BYTE PTR [rax+rcx*1]
   1800042c8:	0f 57 db             	xorps  xmm3,xmm3
   1800042cb:	f2 41 0f 2a d8       	cvtsi2sd xmm3,r8d
   1800042d0:	44 0f b6 45 16       	movzx  r8d,BYTE PTR [rbp+0x16]
   1800042d5:	0f 57 d2             	xorps  xmm2,xmm2
   1800042d8:	f2 41 0f 2a d0       	cvtsi2sd xmm2,r8d
   1800042dd:	44 0f b6 44 08 ff    	movzx  r8d,BYTE PTR [rax+rcx*1-0x1]
   1800042e3:	0f 57 e4             	xorps  xmm4,xmm4
   1800042e6:	f2 41 0f 2a e0       	cvtsi2sd xmm4,r8d
   1800042eb:	44 0f b6 45 15       	movzx  r8d,BYTE PTR [rbp+0x15]
   1800042f0:	0f 57 ed             	xorps  xmm5,xmm5
   1800042f3:	f2 41 0f 2a e8       	cvtsi2sd xmm5,r8d
   1800042f8:	f2 0f 5c cb          	subsd  xmm1,xmm3
   1800042fc:	44 0f b6 44 08 fe    	movzx  r8d,BYTE PTR [rax+rcx*1-0x2]
   180004302:	0f 57 db             	xorps  xmm3,xmm3
   180004305:	f2 41 0f 2a d8       	cvtsi2sd xmm3,r8d
   18000430a:	f2 0f 5c d4          	subsd  xmm2,xmm4
   18000430e:	44 0f b6 45 14       	movzx  r8d,BYTE PTR [rbp+0x14]
   180004313:	0f 57 e4             	xorps  xmm4,xmm4
   180004316:	f2 41 0f 2a e0       	cvtsi2sd xmm4,r8d
   18000431b:	f2 0f 5c eb          	subsd  xmm5,xmm3
   18000431f:	44 0f b6 44 08 fd    	movzx  r8d,BYTE PTR [rax+rcx*1-0x3]
   180004325:	0f 57 db             	xorps  xmm3,xmm3
   180004328:	f2 41 0f 2a d8       	cvtsi2sd xmm3,r8d
   18000432d:	f2 0f 5c e3          	subsd  xmm4,xmm3
   180004331:	f2 0f 59 e4          	mulsd  xmm4,xmm4
   180004335:	f2 0f 58 e0          	addsd  xmm4,xmm0
   180004339:	f2 0f 59 ed          	mulsd  xmm5,xmm5
   18000433d:	f2 0f 58 ec          	addsd  xmm5,xmm4
   180004341:	f2 0f 59 d2          	mulsd  xmm2,xmm2
   180004345:	f2 0f 58 d5          	addsd  xmm2,xmm5
   180004349:	f2 0f 59 c9          	mulsd  xmm1,xmm1
   18000434d:	f2 0f 58 ca          	addsd  xmm1,xmm2
   180004351:	f2 41 0f 5e c9       	divsd  xmm1,xmm9
   180004356:	f2 41 0f 58 cd       	addsd  xmm1,xmm13
   18000435b:	66 0f 28 d1          	movapd xmm2,xmm1
   18000435f:	f2 0f 59 d1          	mulsd  xmm2,xmm1
   180004363:	f2 0f 59 d1          	mulsd  xmm2,xmm1
   180004367:	66 41 0f 28 cd       	movapd xmm1,xmm13
   18000436c:	f2 0f 5e ca          	divsd  xmm1,xmm2
   180004370:	f2 41 0f 59 cb       	mulsd  xmm1,xmm11
   180004375:	f2 41 0f 58 8c d6 00 	addsd  xmm1,QWORD PTR [r14+rdx*8+0x100]
   18000437c:	01 00 00 
   18000437f:	f2 41 0f 11 8c d6 00 	movsd  QWORD PTR [r14+rdx*8+0x100],xmm1
   180004386:	01 00 00 
   180004389:	e9 02 ff ff ff       	jmp    0x180004290
   18000438e:	66 41 0f 28 c2       	movapd xmm0,xmm10
   180004393:	66 41 0f 14 c2       	unpcklpd xmm0,xmm10
   180004398:	b8 25 00 00 00       	mov    eax,0x25
   18000439d:	0f 1f 00             	nop    DWORD PTR [rax]
   1800043a0:	66 41 0f 10 4c c6 e0 	movupd xmm1,XMMWORD PTR [r14+rax*8-0x20]
   1800043a7:	66 41 0f 10 54 c6 f0 	movupd xmm2,XMMWORD PTR [r14+rax*8-0x10]
   1800043ae:	66 41 0f 10 1c c6    	movupd xmm3,XMMWORD PTR [r14+rax*8]
   1800043b4:	66 0f 5e c8          	divpd  xmm1,xmm0
   1800043b8:	66 41 0f 11 4c c6 e0 	movupd XMMWORD PTR [r14+rax*8-0x20],xmm1
   1800043bf:	66 0f 5e d0          	divpd  xmm2,xmm0
   1800043c3:	66 41 0f 11 54 c6 f0 	movupd XMMWORD PTR [r14+rax*8-0x10],xmm2
   1800043ca:	66 0f 5e d8          	divpd  xmm3,xmm0
   1800043ce:	66 41 0f 11 1c c6    	movupd XMMWORD PTR [r14+rax*8],xmm3
   1800043d4:	48 83 c0 06          	add    rax,0x6
   1800043d8:	48 3d 87 01 00 00    	cmp    rax,0x187
   1800043de:	75 c0                	jne    0x1800043a0
   1800043e0:	4c 8d ac 24 e0 00 00 	lea    r13,[rsp+0xe0]
   1800043e7:	00 
   1800043e8:	f2 44 0f 10 1d bf 79 	movsd  xmm11,QWORD PTR [rip+0x379bf]        # 0x18003bdb0
   1800043ef:	03 00 
   1800043f1:	66 45 0f 57 e4       	xorpd  xmm12,xmm12
   1800043f6:	41 bf 01 00 00 00    	mov    r15d,0x1
   1800043fc:	4c 8d 25 4d 45 0a 00 	lea    r12,[rip+0xa454d]        # 0x1800a8950
   180004403:	eb 43                	jmp    0x180004448
   180004405:	66 66 2e 0f 1f 84 00 	data16 cs nop WORD PTR [rax+rax*1+0x0]
   18000440c:	00 00 00 00 
   180004410:	f2 41 0f 59 d3       	mulsd  xmm2,xmm11
   180004415:	48 8b 84 24 f8 2c 00 	mov    rax,QWORD PTR [rsp+0x2cf8]
   18000441c:	00 
   18000441d:	f2 42 0f 58 94 f8 00 	addsd  xmm2,QWORD PTR [rax+r15*8+0x100]
   180004424:	01 00 00 
   180004427:	f2 42 0f 11 94 f8 00 	movsd  QWORD PTR [rax+r15*8+0x100],xmm2
   18000442e:	01 00 00 
   180004431:	49 ff c7             	inc    r15
   180004434:	49 81 c4 90 00 00 00 	add    r12,0x90
   18000443b:	49 81 ff 63 01 00 00 	cmp    r15,0x163
   180004442:	0f 84 67 05 00 00    	je     0x1800049af
   180004448:	4c 89 f8             	mov    rax,r15
   18000444b:	48 c1 e0 04          	shl    rax,0x4
   18000444f:	48 8d 0d 7a 4b 03 00 	lea    rcx,[rip+0x34b7a]        # 0x180038fd0
   180004456:	66 83 7c 08 02 01    	cmp    WORD PTR [rax+rcx*1+0x2],0x1
   18000445c:	75 d3                	jne    0x180004431
   18000445e:	48 8d 0d 6b 4b 03 00 	lea    rcx,[rip+0x34b6b]        # 0x180038fd0
   180004465:	0f b7 74 08 04       	movzx  esi,WORD PTR [rax+rcx*1+0x4]
   18000446a:	48 85 f6             	test   rsi,rsi
   18000446d:	74 c2                	je     0x180004431
   18000446f:	ba 80 00 00 00       	mov    edx,0x80
   180004474:	48 8d 8c 24 80 00 00 	lea    rcx,[rsp+0x80]
   18000447b:	00 
   18000447c:	e8 ef cc ff ff       	call   0x180001170
   180004481:	8b 05 6d 5f 04 00    	mov    eax,DWORD PTR [rip+0x45f6d]        # 0x18004a3f4
   180004487:	83 f8 02             	cmp    eax,0x2
   18000448a:	0f 85 87 00 00 00    	jne    0x180004517
   180004490:	31 c0                	xor    eax,eax
   180004492:	4c 89 e1             	mov    rcx,r12
   180004495:	31 d2                	xor    edx,edx
   180004497:	66 0f 1f 84 00 00 00 	nop    WORD PTR [rax+rax*1+0x0]
   18000449e:	00 00 
   1800044a0:	44 0f b7 01          	movzx  r8d,WORD PTR [rcx]
   1800044a4:	66 45 85 c0          	test   r8w,r8w
   1800044a8:	74 1e                	je     0x1800044c8
   1800044aa:	66 44 3b 45 00       	cmp    r8w,WORD PTR [rbp+0x0]
   1800044af:	74 17                	je     0x1800044c8
   1800044b1:	41 89 d0             	mov    r8d,edx
   1800044b4:	ff c2                	inc    edx
   1800044b6:	49 c1 e0 04          	shl    r8,0x4
   1800044ba:	66 0f 28 01          	movapd xmm0,XMMWORD PTR [rcx]
   1800044be:	66 42 0f 29 84 04 80 	movapd XMMWORD PTR [rsp+r8*1+0x80],xmm0
   1800044c5:	00 00 00 
   1800044c8:	48 83 f8 07          	cmp    rax,0x7
   1800044cc:	77 0c                	ja     0x1800044da
   1800044ce:	48 ff c0             	inc    rax
   1800044d1:	48 83 c1 10          	add    rcx,0x10
   1800044d5:	83 fa 08             	cmp    edx,0x8
   1800044d8:	72 c6                	jb     0x1800044a0
   1800044da:	0f b7 84 24 80 00 00 	movzx  eax,WORD PTR [rsp+0x80]
   1800044e1:	00 
   1800044e2:	48 85 c0             	test   rax,rax
   1800044e5:	0f 84 77 03 00 00    	je     0x180004862
   1800044eb:	f2 0f 10 84 24 88 00 	movsd  xmm0,QWORD PTR [rsp+0x88]
   1800044f2:	00 00 
   1800044f4:	b9 ff ff ff ff       	mov    ecx,0xffffffff
   1800044f9:	01 c8                	add    eax,ecx
   1800044fb:	f2 0f 10 8c c4 60 01 	movsd  xmm1,QWORD PTR [rsp+rax*8+0x160]
   180004502:	00 00 
   180004504:	f2 0f 59 c8          	mulsd  xmm1,xmm0
   180004508:	f2 41 0f 58 cc       	addsd  xmm1,xmm12
   18000450d:	f2 41 0f 58 c4       	addsd  xmm0,xmm12
   180004512:	e9 57 03 00 00       	jmp    0x18000486e
   180004517:	81 fe 41 04 00 00    	cmp    esi,0x441
   18000451d:	48 8d 04 b6          	lea    rax,[rsi+rsi*4]
   180004521:	48 8d 44 c3 d8       	lea    rax,[rbx+rax*8-0x28]
   180004526:	b9 00 00 00 00       	mov    ecx,0x0
   18000452b:	48 0f 43 c1          	cmovae rax,rcx
   18000452f:	0f b7 08             	movzx  ecx,WORD PTR [rax]
   180004532:	ba 09 00 00 00       	mov    edx,0x9
   180004537:	eb 3b                	jmp    0x180004574
   180004539:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
   180004540:	41 c1 e1 04          	shl    r9d,0x4
   180004544:	49 01 e1             	add    r9,rsp
   180004547:	49 81 c1 80 00 00 00 	add    r9,0x80
   18000454e:	48 8b 6c 24 40       	mov    rbp,QWORD PTR [rsp+0x40]
   180004553:	48 8d 1d 76 a0 02 00 	lea    rbx,[rip+0x2a076]        # 0x18002e5d0
   18000455a:	66 45 89 01          	mov    WORD PTR [r9],r8w
   18000455e:	f2 41 0f 11 03       	movsd  QWORD PTR [r11],xmm0
   180004563:	48 83 c2 28          	add    rdx,0x28
   180004567:	48 81 fa 49 9c 00 00 	cmp    rdx,0x9c49
   18000456e:	0f 84 66 ff ff ff    	je     0x1800044da
   180004574:	44 0f b7 44 1a f7    	movzx  r8d,WORD PTR [rdx+rbx*1-0x9]
   18000457a:	66 41 39 c8          	cmp    r8w,cx
   18000457e:	74 e3                	je     0x180004563
   180004580:	66 44 3b 45 00       	cmp    r8w,WORD PTR [rbp+0x0]
   180004585:	74 dc                	je     0x180004563
   180004587:	44 0f b6 48 06       	movzx  r9d,BYTE PTR [rax+0x6]
   18000458c:	0f 57 c0             	xorps  xmm0,xmm0
   18000458f:	f2 41 0f 2a c1       	cvtsi2sd xmm0,r9d
   180004594:	44 0f b6 4c 1a fd    	movzx  r9d,BYTE PTR [rdx+rbx*1-0x3]
   18000459a:	0f 57 c9             	xorps  xmm1,xmm1
   18000459d:	f2 41 0f 2a c9       	cvtsi2sd xmm1,r9d
   1800045a2:	f2 0f 5c c1          	subsd  xmm0,xmm1
   1800045a6:	f2 0f 59 c0          	mulsd  xmm0,xmm0
   1800045aa:	44 0f b6 48 07       	movzx  r9d,BYTE PTR [rax+0x7]
   1800045af:	0f 57 c9             	xorps  xmm1,xmm1
   1800045b2:	f2 41 0f 2a c9       	cvtsi2sd xmm1,r9d
   1800045b7:	f2 41 0f 58 c4       	addsd  xmm0,xmm12
   1800045bc:	44 0f b6 4c 1a fe    	movzx  r9d,BYTE PTR [rdx+rbx*1-0x2]
   1800045c2:	0f 57 d2             	xorps  xmm2,xmm2
   1800045c5:	f2 41 0f 2a d1       	cvtsi2sd xmm2,r9d
   1800045ca:	f2 0f 5c ca          	subsd  xmm1,xmm2
   1800045ce:	44 0f b6 48 08       	movzx  r9d,BYTE PTR [rax+0x8]
   1800045d3:	0f 57 d2             	xorps  xmm2,xmm2
   1800045d6:	f2 41 0f 2a d1       	cvtsi2sd xmm2,r9d
   1800045db:	f2 0f 59 c9          	mulsd  xmm1,xmm1
   1800045df:	44 0f b6 4c 1a ff    	movzx  r9d,BYTE PTR [rdx+rbx*1-0x1]
   1800045e5:	0f 57 db             	xorps  xmm3,xmm3
   1800045e8:	f2 41 0f 2a d9       	cvtsi2sd xmm3,r9d
   1800045ed:	f2 0f 58 c8          	addsd  xmm1,xmm0
   1800045f1:	f2 0f 5c d3          	subsd  xmm2,xmm3
   1800045f5:	f2 0f 59 d2          	mulsd  xmm2,xmm2
   1800045f9:	f2 0f 58 d1          	addsd  xmm2,xmm1
   1800045fd:	44 0f b6 48 09       	movzx  r9d,BYTE PTR [rax+0x9]
   180004602:	0f 57 c0             	xorps  xmm0,xmm0
   180004605:	f2 41 0f 2a c1       	cvtsi2sd xmm0,r9d
   18000460a:	44 0f b6 0c 1a       	movzx  r9d,BYTE PTR [rdx+rbx*1]
   18000460f:	0f 57 c9             	xorps  xmm1,xmm1
   180004612:	f2 41 0f 2a c9       	cvtsi2sd xmm1,r9d
   180004617:	f2 0f 5c c1          	subsd  xmm0,xmm1
   18000461b:	f2 0f 59 c0          	mulsd  xmm0,xmm0
   18000461f:	f2 0f 58 c2          	addsd  xmm0,xmm2
   180004623:	f2 41 0f 5e c1       	divsd  xmm0,xmm9
   180004628:	f2 41 0f 58 c5       	addsd  xmm0,xmm13
   18000462d:	66 0f 28 c8          	movapd xmm1,xmm0
   180004631:	f2 0f 59 c8          	mulsd  xmm1,xmm0
   180004635:	f2 0f 59 c8          	mulsd  xmm1,xmm0
   180004639:	66 41 0f 28 c5       	movapd xmm0,xmm13
   18000463e:	f2 0f 5e c1          	divsd  xmm0,xmm1
   180004642:	f2 0f 10 8c 24 88 00 	movsd  xmm1,QWORD PTR [rsp+0x88]
   180004649:	00 00 
   18000464b:	41 b2 01             	mov    r10b,0x1
   18000464e:	66 0f 2e c1          	ucomisd xmm0,xmm1
   180004652:	76 1b                	jbe    0x18000466f
   180004654:	40 b5 01             	mov    bpl,0x1
   180004657:	41 b6 01             	mov    r14b,0x1
   18000465a:	b3 01                	mov    bl,0x1
   18000465c:	40 b6 01             	mov    sil,0x1
   18000465f:	45 31 c9             	xor    r9d,r9d
   180004662:	4c 8d 9c 24 88 00 00 	lea    r11,[rsp+0x88]
   180004669:	00 
   18000466a:	e9 f1 00 00 00       	jmp    0x180004760
   18000466f:	41 b9 01 00 00 00    	mov    r9d,0x1
   180004675:	66 0f 2e 84 24 98 00 	ucomisd xmm0,QWORD PTR [rsp+0x98]
   18000467c:	00 00 
   18000467e:	76 18                	jbe    0x180004698
   180004680:	40 b5 01             	mov    bpl,0x1
   180004683:	41 b6 01             	mov    r14b,0x1
   180004686:	b3 01                	mov    bl,0x1
   180004688:	40 b6 01             	mov    sil,0x1
   18000468b:	4c 8d 9c 24 98 00 00 	lea    r11,[rsp+0x98]
   180004692:	00 
   180004693:	e9 c8 00 00 00       	jmp    0x180004760
   180004698:	66 0f 2e 84 24 a8 00 	ucomisd xmm0,QWORD PTR [rsp+0xa8]
   18000469f:	00 00 
   1800046a1:	76 1d                	jbe    0x1800046c0
   1800046a3:	41 b9 02 00 00 00    	mov    r9d,0x2
   1800046a9:	31 f6                	xor    esi,esi
   1800046ab:	40 b5 01             	mov    bpl,0x1
   1800046ae:	41 b6 01             	mov    r14b,0x1
   1800046b1:	b3 01                	mov    bl,0x1
   1800046b3:	4c 8d 9c 24 a8 00 00 	lea    r11,[rsp+0xa8]
   1800046ba:	00 
   1800046bb:	e9 a0 00 00 00       	jmp    0x180004760
   1800046c0:	66 0f 2e 84 24 b8 00 	ucomisd xmm0,QWORD PTR [rsp+0xb8]
   1800046c7:	00 00 
   1800046c9:	76 1a                	jbe    0x1800046e5
   1800046cb:	41 b9 03 00 00 00    	mov    r9d,0x3
   1800046d1:	31 db                	xor    ebx,ebx
   1800046d3:	40 b5 01             	mov    bpl,0x1
   1800046d6:	41 b6 01             	mov    r14b,0x1
   1800046d9:	31 f6                	xor    esi,esi
   1800046db:	4c 8d 9c 24 b8 00 00 	lea    r11,[rsp+0xb8]
   1800046e2:	00 
   1800046e3:	eb 7b                	jmp    0x180004760
   1800046e5:	66 0f 2e 84 24 c8 00 	ucomisd xmm0,QWORD PTR [rsp+0xc8]
   1800046ec:	00 00 
   1800046ee:	76 1a                	jbe    0x18000470a
   1800046f0:	41 b9 04 00 00 00    	mov    r9d,0x4
   1800046f6:	45 31 f6             	xor    r14d,r14d
   1800046f9:	40 b5 01             	mov    bpl,0x1
   1800046fc:	31 db                	xor    ebx,ebx
   1800046fe:	31 f6                	xor    esi,esi
   180004700:	4c 8d 9c 24 c8 00 00 	lea    r11,[rsp+0xc8]
   180004707:	00 
   180004708:	eb 56                	jmp    0x180004760
   18000470a:	66 0f 2e 84 24 d8 00 	ucomisd xmm0,QWORD PTR [rsp+0xd8]
   180004711:	00 00 
   180004713:	76 19                	jbe    0x18000472e
   180004715:	41 b9 05 00 00 00    	mov    r9d,0x5
   18000471b:	31 ed                	xor    ebp,ebp
   18000471d:	45 31 f6             	xor    r14d,r14d
   180004720:	31 db                	xor    ebx,ebx
   180004722:	31 f6                	xor    esi,esi
   180004724:	4c 8d 9c 24 d8 00 00 	lea    r11,[rsp+0xd8]
   18000472b:	00 
   18000472c:	eb 32                	jmp    0x180004760
   18000472e:	66 0f 2e 84 24 e8 00 	ucomisd xmm0,QWORD PTR [rsp+0xe8]
   180004735:	00 00 
   180004737:	0f 86 01 01 00 00    	jbe    0x18000483e
   18000473d:	41 b9 06 00 00 00    	mov    r9d,0x6
   180004743:	45 31 d2             	xor    r10d,r10d
   180004746:	31 ed                	xor    ebp,ebp
   180004748:	45 31 f6             	xor    r14d,r14d
   18000474b:	31 db                	xor    ebx,ebx
   18000474d:	31 f6                	xor    esi,esi
   18000474f:	4c 8d 9c 24 e8 00 00 	lea    r11,[rsp+0xe8]
   180004756:	00 
   180004757:	66 0f 1f 84 00 00 00 	nop    WORD PTR [rax+rax*1+0x0]
   18000475e:	00 00 
   180004760:	66 41 0f 28 55 00    	movapd xmm2,XMMWORD PTR [r13+0x0]
   180004766:	48 8d bc 24 f0 00 00 	lea    rdi,[rsp+0xf0]
   18000476d:	00 
   18000476e:	66 0f 29 17          	movapd XMMWORD PTR [rdi],xmm2
   180004772:	45 84 d2             	test   r10b,r10b
   180004775:	0f 84 c5 fd ff ff    	je     0x180004540
   18000477b:	4c 8d 94 24 d0 00 00 	lea    r10,[rsp+0xd0]
   180004782:	00 
   180004783:	66 41 0f 28 12       	movapd xmm2,XMMWORD PTR [r10]
   180004788:	66 41 0f 29 55 00    	movapd XMMWORD PTR [r13+0x0],xmm2
   18000478e:	40 84 ed             	test   bpl,bpl
   180004791:	0f 84 a9 fd ff ff    	je     0x180004540
   180004797:	4c 8d 94 24 c0 00 00 	lea    r10,[rsp+0xc0]
   18000479e:	00 
   18000479f:	66 41 0f 28 12       	movapd xmm2,XMMWORD PTR [r10]
   1800047a4:	4c 8d 94 24 d0 00 00 	lea    r10,[rsp+0xd0]
   1800047ab:	00 
   1800047ac:	66 41 0f 29 12       	movapd XMMWORD PTR [r10],xmm2
   1800047b1:	45 84 f6             	test   r14b,r14b
   1800047b4:	0f 84 86 fd ff ff    	je     0x180004540
   1800047ba:	4c 8d 94 24 b0 00 00 	lea    r10,[rsp+0xb0]
   1800047c1:	00 
   1800047c2:	66 41 0f 28 12       	movapd xmm2,XMMWORD PTR [r10]
   1800047c7:	4c 8d 94 24 c0 00 00 	lea    r10,[rsp+0xc0]
   1800047ce:	00 
   1800047cf:	66 41 0f 29 12       	movapd XMMWORD PTR [r10],xmm2
   1800047d4:	84 db                	test   bl,bl
   1800047d6:	0f 84 64 fd ff ff    	je     0x180004540
   1800047dc:	4c 8d 94 24 a0 00 00 	lea    r10,[rsp+0xa0]
   1800047e3:	00 
   1800047e4:	66 41 0f 28 12       	movapd xmm2,XMMWORD PTR [r10]
   1800047e9:	4c 8d 94 24 b0 00 00 	lea    r10,[rsp+0xb0]
   1800047f0:	00 
   1800047f1:	66 41 0f 29 12       	movapd XMMWORD PTR [r10],xmm2
   1800047f6:	40 84 f6             	test   sil,sil
   1800047f9:	0f 84 41 fd ff ff    	je     0x180004540
   1800047ff:	66 0f 2e c1          	ucomisd xmm0,xmm1
   180004803:	4c 8d 94 24 90 00 00 	lea    r10,[rsp+0x90]
   18000480a:	00 
   18000480b:	66 41 0f 28 0a       	movapd xmm1,XMMWORD PTR [r10]
   180004810:	4c 8d 94 24 a0 00 00 	lea    r10,[rsp+0xa0]
   180004817:	00 
   180004818:	66 41 0f 29 0a       	movapd XMMWORD PTR [r10],xmm1
   18000481d:	0f 86 1d fd ff ff    	jbe    0x180004540
   180004823:	66 0f 28 8c 24 80 00 	movapd xmm1,XMMWORD PTR [rsp+0x80]
   18000482a:	00 00 
   18000482c:	4c 8d 94 24 90 00 00 	lea    r10,[rsp+0x90]
   180004833:	00 
   180004834:	66 41 0f 29 0a       	movapd XMMWORD PTR [r10],xmm1
   180004839:	e9 02 fd ff ff       	jmp    0x180004540
   18000483e:	66 0f 2e 84 24 f8 00 	ucomisd xmm0,QWORD PTR [rsp+0xf8]
   180004845:	00 00 
   180004847:	4c 8d 8c 24 f0 00 00 	lea    r9,[rsp+0xf0]
   18000484e:	00 
   18000484f:	4c 8d 9c 24 f8 00 00 	lea    r11,[rsp+0xf8]
   180004856:	00 
   180004857:	0f 87 fd fc ff ff    	ja     0x18000455a
   18000485d:	e9 01 fd ff ff       	jmp    0x180004563
   180004862:	0f 57 c9             	xorps  xmm1,xmm1
   180004865:	66 0f 57 c0          	xorpd  xmm0,xmm0
   180004869:	b9 ff ff ff ff       	mov    ecx,0xffffffff
   18000486e:	0f b7 84 24 90 00 00 	movzx  eax,WORD PTR [rsp+0x90]
   180004875:	00 
   180004876:	48 85 c0             	test   rax,rax
   180004879:	74 1c                	je     0x180004897
   18000487b:	f2 0f 10 94 24 98 00 	movsd  xmm2,QWORD PTR [rsp+0x98]
   180004882:	00 00 
   180004884:	01 c8                	add    eax,ecx
   180004886:	f2 0f 58 c2          	addsd  xmm0,xmm2
   18000488a:	f2 0f 59 94 c4 60 01 	mulsd  xmm2,QWORD PTR [rsp+rax*8+0x160]
   180004891:	00 00 
   180004893:	f2 0f 58 ca          	addsd  xmm1,xmm2
   180004897:	0f b7 84 24 a0 00 00 	movzx  eax,WORD PTR [rsp+0xa0]
   18000489e:	00 
   18000489f:	48 85 c0             	test   rax,rax
   1800048a2:	74 1c                	je     0x1800048c0
   1800048a4:	f2 0f 10 94 24 a8 00 	movsd  xmm2,QWORD PTR [rsp+0xa8]
   1800048ab:	00 00 
   1800048ad:	01 c8                	add    eax,ecx
   1800048af:	f2 0f 58 c2          	addsd  xmm0,xmm2
   1800048b3:	f2 0f 59 94 c4 60 01 	mulsd  xmm2,QWORD PTR [rsp+rax*8+0x160]
   1800048ba:	00 00 
   1800048bc:	f2 0f 58 ca          	addsd  xmm1,xmm2
   1800048c0:	0f b7 84 24 b0 00 00 	movzx  eax,WORD PTR [rsp+0xb0]
   1800048c7:	00 
   1800048c8:	48 85 c0             	test   rax,rax
   1800048cb:	74 1c                	je     0x1800048e9
   1800048cd:	f2 0f 10 94 24 b8 00 	movsd  xmm2,QWORD PTR [rsp+0xb8]
   1800048d4:	00 00 
   1800048d6:	01 c8                	add    eax,ecx
   1800048d8:	f2 0f 58 c2          	addsd  xmm0,xmm2
   1800048dc:	f2 0f 59 94 c4 60 01 	mulsd  xmm2,QWORD PTR [rsp+rax*8+0x160]
   1800048e3:	00 00 
   1800048e5:	f2 0f 58 ca          	addsd  xmm1,xmm2
   1800048e9:	0f b7 84 24 c0 00 00 	movzx  eax,WORD PTR [rsp+0xc0]
   1800048f0:	00 
   1800048f1:	48 85 c0             	test   rax,rax
   1800048f4:	74 1c                	je     0x180004912
   1800048f6:	f2 0f 10 94 24 c8 00 	movsd  xmm2,QWORD PTR [rsp+0xc8]
   1800048fd:	00 00 
   1800048ff:	01 c8                	add    eax,ecx
   180004901:	f2 0f 58 c2          	addsd  xmm0,xmm2
   180004905:	f2 0f 59 94 c4 60 01 	mulsd  xmm2,QWORD PTR [rsp+rax*8+0x160]
   18000490c:	00 00 
   18000490e:	f2 0f 58 ca          	addsd  xmm1,xmm2
   180004912:	0f b7 84 24 d0 00 00 	movzx  eax,WORD PTR [rsp+0xd0]
   180004919:	00 
   18000491a:	48 85 c0             	test   rax,rax
   18000491d:	74 1c                	je     0x18000493b
   18000491f:	f2 0f 10 94 24 d8 00 	movsd  xmm2,QWORD PTR [rsp+0xd8]
   180004926:	00 00 
   180004928:	01 c8                	add    eax,ecx
   18000492a:	f2 0f 58 c2          	addsd  xmm0,xmm2
   18000492e:	f2 0f 59 94 c4 60 01 	mulsd  xmm2,QWORD PTR [rsp+rax*8+0x160]
   180004935:	00 00 
   180004937:	f2 0f 58 ca          	addsd  xmm1,xmm2
   18000493b:	0f b7 84 24 e0 00 00 	movzx  eax,WORD PTR [rsp+0xe0]
   180004942:	00 
   180004943:	48 85 c0             	test   rax,rax
   180004946:	74 1c                	je     0x180004964
   180004948:	f2 0f 10 94 24 e8 00 	movsd  xmm2,QWORD PTR [rsp+0xe8]
   18000494f:	00 00 
   180004951:	01 c8                	add    eax,ecx
   180004953:	f2 0f 58 c2          	addsd  xmm0,xmm2
   180004957:	f2 0f 59 94 c4 60 01 	mulsd  xmm2,QWORD PTR [rsp+rax*8+0x160]
   18000495e:	00 00 
   180004960:	f2 0f 58 ca          	addsd  xmm1,xmm2
   180004964:	0f b7 84 24 f0 00 00 	movzx  eax,WORD PTR [rsp+0xf0]
   18000496b:	00 
   18000496c:	48 85 c0             	test   rax,rax
   18000496f:	74 1c                	je     0x18000498d
   180004971:	f2 0f 10 94 24 f8 00 	movsd  xmm2,QWORD PTR [rsp+0xf8]
   180004978:	00 00 
   18000497a:	01 c8                	add    eax,ecx
   18000497c:	f2 0f 58 c2          	addsd  xmm0,xmm2
   180004980:	f2 0f 59 94 c4 60 01 	mulsd  xmm2,QWORD PTR [rsp+rax*8+0x160]
   180004987:	00 00 
   180004989:	f2 0f 58 ca          	addsd  xmm1,xmm2
   18000498d:	66 0f 57 d2          	xorpd  xmm2,xmm2
   180004991:	66 0f 2e c2          	ucomisd xmm0,xmm2
   180004995:	75 06                	jne    0x18000499d
   180004997:	0f 8b 73 fa ff ff    	jnp    0x180004410
   18000499d:	f2 0f 5e c8          	divsd  xmm1,xmm0
   1800049a1:	f2 41 0f 5e ca       	divsd  xmm1,xmm10
   1800049a6:	66 0f 28 d1          	movapd xmm2,xmm1
   1800049aa:	e9 61 fa ff ff       	jmp    0x180004410
   1800049af:	0f b7 55 1a          	movzx  edx,WORD PTR [rbp+0x1a]
   1800049b3:	0f b7 4d 1c          	movzx  ecx,WORD PTR [rbp+0x1c]
   1800049b7:	8d 82 bf fb ff ff    	lea    eax,[rdx-0x441]
   1800049bd:	44 0f b7 c0          	movzx  r8d,ax
   1800049c1:	31 c0                	xor    eax,eax
   1800049c3:	66 0f 57 c0          	xorpd  xmm0,xmm0
   1800049c7:	41 81 f8 c0 fb 00 00 	cmp    r8d,0xfbc0
   1800049ce:	73 18                	jae    0x1800049e8
   1800049d0:	49 89 d9             	mov    r9,rbx
   1800049d3:	48 8b 9c 24 f8 2c 00 	mov    rbx,QWORD PTR [rsp+0x2cf8]
   1800049da:	00 
   1800049db:	48 8b bc 24 10 01 00 	mov    rdi,QWORD PTR [rsp+0x110]
   1800049e2:	00 
   1800049e3:	e9 24 01 00 00       	jmp    0x180004b0c
   1800049e8:	48 ff ca             	dec    rdx
   1800049eb:	48 8d 14 92          	lea    rdx,[rdx+rdx*4]
   1800049ef:	44 0f b7 04 d3       	movzx  r8d,WORD PTR [rbx+rdx*8]
   1800049f4:	66 44 3b 45 00       	cmp    r8w,WORD PTR [rbp+0x0]
   1800049f9:	49 89 d9             	mov    r9,rbx
   1800049fc:	48 8b 9c 24 f8 2c 00 	mov    rbx,QWORD PTR [rsp+0x2cf8]
   180004a03:	00 
   180004a04:	48 8b bc 24 10 01 00 	mov    rdi,QWORD PTR [rsp+0x110]
   180004a0b:	00 
   180004a0c:	0f 84 fa 00 00 00    	je     0x180004b0c
   180004a12:	66 44 89 83 48 22 00 	mov    WORD PTR [rbx+0x2248],r8w
   180004a19:	00 
   180004a1a:	48 b8 ec 51 b8 1e 85 	movabs rax,0x3fa1eb851eb851ec
   180004a21:	eb a1 3f 
   180004a24:	48 89 84 24 80 00 00 	mov    QWORD PTR [rsp+0x80],rax
   180004a2b:	00 
   180004a2c:	41 8b 54 d1 02       	mov    edx,DWORD PTR [r9+rdx*8+0x2]
   180004a31:	66 0f 6e ca          	movd   xmm1,edx
   180004a35:	8d 82 bf fb ff ff    	lea    eax,[rdx-0x441]
   180004a3b:	44 0f b7 c0          	movzx  r8d,ax
   180004a3f:	b8 01 00 00 00       	mov    eax,0x1
   180004a44:	f2 0f 10 05 6c 73 03 	movsd  xmm0,QWORD PTR [rip+0x3736c]        # 0x18003bdb8
   180004a4b:	00 
   180004a4c:	41 81 f8 c0 fb 00 00 	cmp    r8d,0xfbc0
   180004a53:	72 4b                	jb     0x180004aa0
   180004a55:	66 3b 55 00          	cmp    dx,WORD PTR [rbp+0x0]
   180004a59:	74 45                	je     0x180004aa0
   180004a5b:	45 31 c0             	xor    r8d,r8d
   180004a5e:	66 42 39 94 43 48 22 	cmp    WORD PTR [rbx+r8*2+0x2248],dx
   180004a65:	00 00 
   180004a67:	74 0b                	je     0x180004a74
   180004a69:	49 ff c0             	inc    r8
   180004a6c:	49 83 f8 01          	cmp    r8,0x1
   180004a70:	75 ec                	jne    0x180004a5e
   180004a72:	eb 06                	jmp    0x180004a7a
   180004a74:	41 83 f8 01          	cmp    r8d,0x1
   180004a78:	75 26                	jne    0x180004aa0
   180004a7a:	66 89 93 4a 22 00 00 	mov    WORD PTR [rbx+0x224a],dx
   180004a81:	48 b8 7b 14 ae 47 e1 	movabs rax,0x3f847ae147ae147b
   180004a88:	7a 84 3f 
   180004a8b:	48 89 84 24 88 00 00 	mov    QWORD PTR [rsp+0x88],rax
   180004a92:	00 
   180004a93:	b8 02 00 00 00       	mov    eax,0x2
   180004a98:	f2 0f 10 05 20 73 03 	movsd  xmm0,QWORD PTR [rip+0x37320]        # 0x18003bdc0
   180004a9f:	00 
   180004aa0:	66 44 0f c5 c1 01    	pextrw r8d,xmm1,0x1
   180004aa6:	89 c2                	mov    edx,eax
   180004aa8:	41 8d 80 bf fb ff ff 	lea    eax,[r8-0x441]
   180004aaf:	0f b7 c0             	movzx  eax,ax
   180004ab2:	3d bf fb 00 00       	cmp    eax,0xfbbf
   180004ab7:	76 4a                	jbe    0x180004b03
   180004ab9:	66 44 3b 45 00       	cmp    r8w,WORD PTR [rbp+0x0]
   180004abe:	74 43                	je     0x180004b03
   180004ac0:	31 c0                	xor    eax,eax
   180004ac2:	66 44 39 84 43 48 22 	cmp    WORD PTR [rbx+rax*2+0x2248],r8w
   180004ac9:	00 00 
   180004acb:	74 0a                	je     0x180004ad7
   180004acd:	48 ff c0             	inc    rax
   180004ad0:	48 39 c2             	cmp    rdx,rax
   180004ad3:	75 ed                	jne    0x180004ac2
   180004ad5:	eb 04                	jmp    0x180004adb
   180004ad7:	39 c2                	cmp    edx,eax
   180004ad9:	75 28                	jne    0x180004b03
   180004adb:	66 44 89 84 53 48 22 	mov    WORD PTR [rbx+rdx*2+0x2248],r8w
   180004ae2:	00 00 
   180004ae4:	8d 42 01             	lea    eax,[rdx+0x1]
   180004ae7:	49 b8 7b 14 ae 47 e1 	movabs r8,0x3f847ae147ae147b
   180004aee:	7a 84 3f 
   180004af1:	4c 89 84 d4 80 00 00 	mov    QWORD PTR [rsp+rdx*8+0x80],r8
   180004af8:	00 
   180004af9:	f2 0f 58 05 c7 72 03 	addsd  xmm0,QWORD PTR [rip+0x372c7]        # 0x18003bdc8
   180004b00:	00 
   180004b01:	eb 02                	jmp    0x180004b05
   180004b03:	89 d0                	mov    eax,edx
   180004b05:	4c 8d 0d c4 9a 02 00 	lea    r9,[rip+0x29ac4]        # 0x18002e5d0
   180004b0c:	8d 91 bf fb ff ff    	lea    edx,[rcx-0x441]
   180004b12:	44 0f b7 c2          	movzx  r8d,dx
   180004b16:	31 d2                	xor    edx,edx
   180004b18:	48 8d 0c 89          	lea    rcx,[rcx+rcx*4]
   180004b1c:	49 8d 0c c9          	lea    rcx,[r9+rcx*8]
   180004b20:	48 83 c1 d8          	add    rcx,0xffffffffffffffd8
   180004b24:	41 81 f8 c0 fb 00 00 	cmp    r8d,0xfbc0
   180004b2b:	48 0f 43 d1          	cmovae rdx,rcx
   180004b2f:	0f 82 6f 01 00 00    	jb     0x180004ca4
   180004b35:	0f b7 09             	movzx  ecx,WORD PTR [rcx]
   180004b38:	66 3b 4d 00          	cmp    cx,WORD PTR [rbp+0x0]
   180004b3c:	0f 84 62 01 00 00    	je     0x180004ca4
   180004b42:	45 31 c0             	xor    r8d,r8d
   180004b45:	85 c0                	test   eax,eax
   180004b47:	74 09                	je     0x180004b52
   180004b49:	66 39 8b 48 22 00 00 	cmp    WORD PTR [rbx+0x2248],cx
   180004b50:	75 07                	jne    0x180004b59
   180004b52:	41 39 c0             	cmp    r8d,eax
   180004b55:	74 07                	je     0x180004b5e
   180004b57:	eb 2d                	jmp    0x180004b86
   180004b59:	83 f8 01             	cmp    eax,0x1
   180004b5c:	75 77                	jne    0x180004bd5
   180004b5e:	41 89 c0             	mov    r8d,eax
   180004b61:	66 42 89 8c 43 48 22 	mov    WORD PTR [rbx+r8*2+0x2248],cx
   180004b68:	00 00 
   180004b6a:	ff c0                	inc    eax
   180004b6c:	48 b9 ec 51 b8 1e 85 	movabs rcx,0x3fa1eb851eb851ec
   180004b73:	eb a1 3f 
   180004b76:	4a 89 8c c4 80 00 00 	mov    QWORD PTR [rsp+r8*8+0x80],rcx
   180004b7d:	00 
   180004b7e:	f2 0f 58 05 32 72 03 	addsd  xmm0,QWORD PTR [rip+0x37232]        # 0x18003bdb8
   180004b85:	00 
   180004b86:	83 f8 05             	cmp    eax,0x5
   180004b89:	0f 87 15 01 00 00    	ja     0x180004ca4
   180004b8f:	66 0f 6e 4a 02       	movd   xmm1,DWORD PTR [rdx+0x2]
   180004b94:	66 0f 7e c9          	movd   ecx,xmm1
   180004b98:	8d 91 bf fb ff ff    	lea    edx,[rcx-0x441]
   180004b9e:	0f b7 d2             	movzx  edx,dx
   180004ba1:	81 fa c0 fb 00 00    	cmp    edx,0xfbc0
   180004ba7:	0f 82 8a 00 00 00    	jb     0x180004c37
   180004bad:	66 3b 4d 00          	cmp    cx,WORD PTR [rbp+0x0]
   180004bb1:	0f 84 80 00 00 00    	je     0x180004c37
   180004bb7:	85 c0                	test   eax,eax
   180004bb9:	74 4e                	je     0x180004c09
   180004bbb:	89 c2                	mov    edx,eax
   180004bbd:	45 31 c0             	xor    r8d,r8d
   180004bc0:	66 42 39 8c 43 48 22 	cmp    WORD PTR [rbx+r8*2+0x2248],cx
   180004bc7:	00 00 
   180004bc9:	74 41                	je     0x180004c0c
   180004bcb:	49 ff c0             	inc    r8
   180004bce:	4c 39 c2             	cmp    rdx,r8
   180004bd1:	75 ed                	jne    0x180004bc0
   180004bd3:	eb 3e                	jmp    0x180004c13
   180004bd5:	41 b8 01 00 00 00    	mov    r8d,0x1
   180004bdb:	66 39 8b 4a 22 00 00 	cmp    WORD PTR [rbx+0x224a],cx
   180004be2:	0f 84 6a ff ff ff    	je     0x180004b52
   180004be8:	83 f8 02             	cmp    eax,0x2
   180004beb:	0f 84 6d ff ff ff    	je     0x180004b5e
   180004bf1:	41 b8 02 00 00 00    	mov    r8d,0x2
   180004bf7:	66 39 8b 4c 22 00 00 	cmp    WORD PTR [rbx+0x224c],cx
   180004bfe:	0f 84 4e ff ff ff    	je     0x180004b52
   180004c04:	e9 55 ff ff ff       	jmp    0x180004b5e
   180004c09:	45 31 c0             	xor    r8d,r8d
   180004c0c:	41 39 c0             	cmp    r8d,eax
   180004c0f:	75 26                	jne    0x180004c37
   180004c11:	89 c2                	mov    edx,eax
   180004c13:	66 89 8c 53 48 22 00 	mov    WORD PTR [rbx+rdx*2+0x2248],cx
   180004c1a:	00 
   180004c1b:	ff c0                	inc    eax
   180004c1d:	48 b9 7b 14 ae 47 e1 	movabs rcx,0x3f847ae147ae147b
   180004c24:	7a 84 3f 
   180004c27:	48 89 8c d4 80 00 00 	mov    QWORD PTR [rsp+rdx*8+0x80],rcx
   180004c2e:	00 
   180004c2f:	f2 0f 58 05 91 71 03 	addsd  xmm0,QWORD PTR [rip+0x37191]        # 0x18003bdc8
   180004c36:	00 
   180004c37:	83 f8 05             	cmp    eax,0x5
   180004c3a:	77 68                	ja     0x180004ca4
   180004c3c:	66 0f c5 c9 01       	pextrw ecx,xmm1,0x1
   180004c41:	8d 91 bf fb ff ff    	lea    edx,[rcx-0x441]
   180004c47:	0f b7 d2             	movzx  edx,dx
   180004c4a:	81 fa c0 fb 00 00    	cmp    edx,0xfbc0
   180004c50:	72 52                	jb     0x180004ca4
   180004c52:	66 3b 4d 00          	cmp    cx,WORD PTR [rbp+0x0]
   180004c56:	74 4c                	je     0x180004ca4
   180004c58:	85 c0                	test   eax,eax
   180004c5a:	74 1a                	je     0x180004c76
   180004c5c:	89 c2                	mov    edx,eax
   180004c5e:	45 31 c0             	xor    r8d,r8d
   180004c61:	66 42 39 8c 43 48 22 	cmp    WORD PTR [rbx+r8*2+0x2248],cx
   180004c68:	00 00 
   180004c6a:	74 0d                	je     0x180004c79
   180004c6c:	49 ff c0             	inc    r8
   180004c6f:	4c 39 c2             	cmp    rdx,r8
   180004c72:	75 ed                	jne    0x180004c61
   180004c74:	eb 0a                	jmp    0x180004c80
   180004c76:	45 31 c0             	xor    r8d,r8d
   180004c79:	41 39 c0             	cmp    r8d,eax
   180004c7c:	75 26                	jne    0x180004ca4
   180004c7e:	89 c2                	mov    edx,eax
   180004c80:	66 89 8c 53 48 22 00 	mov    WORD PTR [rbx+rdx*2+0x2248],cx
   180004c87:	00 
   180004c88:	ff c0                	inc    eax
   180004c8a:	48 b9 7b 14 ae 47 e1 	movabs rcx,0x3f847ae147ae147b
   180004c91:	7a 84 3f 
   180004c94:	48 89 8c d4 80 00 00 	mov    QWORD PTR [rsp+rdx*8+0x80],rcx
   180004c9b:	00 
   180004c9c:	f2 0f 58 05 24 71 03 	addsd  xmm0,QWORD PTR [rip+0x37124]        # 0x18003bdc8
   180004ca3:	00 
   180004ca4:	66 0f 2e 05 24 71 03 	ucomisd xmm0,QWORD PTR [rip+0x37124]        # 0x18003bdd0
   180004cab:	00 
   180004cac:	41 89 c0             	mov    r8d,eax
   180004caf:	0f 86 79 00 00 00    	jbe    0x180004d2e
   180004cb5:	85 c0                	test   eax,eax
   180004cb7:	74 75                	je     0x180004d2e
   180004cb9:	f2 0f 10 0d 0f 71 03 	movsd  xmm1,QWORD PTR [rip+0x3710f]        # 0x18003bdd0
   180004cc0:	00 
   180004cc1:	f2 0f 5e c8          	divsd  xmm1,xmm0
   180004cc5:	83 f8 04             	cmp    eax,0x4
   180004cc8:	73 04                	jae    0x180004cce
   180004cca:	31 c9                	xor    ecx,ecx
   180004ccc:	eb 42                	jmp    0x180004d10
   180004cce:	66 0f 28 c1          	movapd xmm0,xmm1
   180004cd2:	66 0f 14 c1          	unpcklpd xmm0,xmm1
   180004cd6:	44 89 c1             	mov    ecx,r8d
   180004cd9:	66 0f 28 94 24 80 00 	movapd xmm2,XMMWORD PTR [rsp+0x80]
   180004ce0:	00 00 
   180004ce2:	66 0f 59 d0          	mulpd  xmm2,xmm0
   180004ce6:	66 0f 59 84 24 90 00 	mulpd  xmm0,XMMWORD PTR [rsp+0x90]
   180004ced:	00 00 
   180004cef:	83 e1 fc             	and    ecx,0xfffffffc
   180004cf2:	66 0f 29 94 24 80 00 	movapd XMMWORD PTR [rsp+0x80],xmm2
   180004cf9:	00 00 
   180004cfb:	66 0f 29 84 24 90 00 	movapd XMMWORD PTR [rsp+0x90],xmm0
   180004d02:	00 00 
   180004d04:	4c 39 c1             	cmp    rcx,r8
   180004d07:	74 25                	je     0x180004d2e
   180004d09:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
   180004d10:	f2 0f 10 84 cc 80 00 	movsd  xmm0,QWORD PTR [rsp+rcx*8+0x80]
   180004d17:	00 00 
   180004d19:	f2 0f 59 c1          	mulsd  xmm0,xmm1
   180004d1d:	f2 0f 11 84 cc 80 00 	movsd  QWORD PTR [rsp+rcx*8+0x80],xmm0
   180004d24:	00 00 
   180004d26:	48 ff c1             	inc    rcx
   180004d29:	49 39 c8             	cmp    r8,rcx
   180004d2c:	75 e2                	jne    0x180004d10
   180004d2e:	89 83 54 22 00 00    	mov    DWORD PTR [rbx+0x2254],eax
   180004d34:	41 c1 e0 03          	shl    r8d,0x3
   180004d38:	48 8d 8b 70 22 00 00 	lea    rcx,[rbx+0x2270]
   180004d3f:	48 8d 94 24 80 00 00 	lea    rdx,[rsp+0x80]
   180004d46:	00 
   180004d47:	e8 b4 c2 ff ff       	call   0x180001000
   180004d4c:	8b 83 54 22 00 00    	mov    eax,DWORD PTR [rbx+0x2254]
   180004d52:	48 85 c0             	test   rax,rax
   180004d55:	0f 84 45 02 00 00    	je     0x180004fa0
   180004d5b:	31 c9                	xor    ecx,ecx
   180004d5d:	f2 0f 10 05 73 70 03 	movsd  xmm0,QWORD PTR [rip+0x37073]        # 0x18003bdd8
   180004d64:	00 
   180004d65:	31 d2                	xor    edx,edx
   180004d67:	eb 13                	jmp    0x180004d7c
   180004d69:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
   180004d70:	48 ff c2             	inc    rdx
   180004d73:	48 39 d0             	cmp    rax,rdx
   180004d76:	0f 84 24 02 00 00    	je     0x180004fa0
   180004d7c:	44 0f b7 84 53 48 22 	movzx  r8d,WORD PTR [rbx+rdx*2+0x2248]
   180004d83:	00 00 
   180004d85:	45 8d 48 ff          	lea    r9d,[r8-0x1]
   180004d89:	45 0f b7 c9          	movzx  r9d,r9w
   180004d8d:	41 81 f9 40 04 00 00 	cmp    r9d,0x440
   180004d94:	4f 8d 04 80          	lea    r8,[r8+r8*4]
   180004d98:	4c 8d 0d 31 98 02 00 	lea    r9,[rip+0x29831]        # 0x18002e5d0
   180004d9f:	4f 8d 44 c1 d8       	lea    r8,[r9+r8*8-0x28]
   180004da4:	4c 0f 43 c1          	cmovae r8,rcx
   180004da8:	45 0f b7 48 0e       	movzx  r9d,WORD PTR [r8+0xe]
   180004dad:	45 8d 51 ff          	lea    r10d,[r9-0x1]
   180004db1:	45 0f b7 d2          	movzx  r10d,r10w
   180004db5:	41 81 fa 61 01 00 00 	cmp    r10d,0x161
   180004dbc:	77 21                	ja     0x180004ddf
   180004dbe:	f2 0f 10 8c d4 80 00 	movsd  xmm1,QWORD PTR [rsp+rdx*8+0x80]
   180004dc5:	00 00 
   180004dc7:	f2 0f 5e c8          	divsd  xmm1,xmm0
   180004dcb:	f2 42 0f 58 8c cb 00 	addsd  xmm1,QWORD PTR [rbx+r9*8+0x100]
   180004dd2:	01 00 00 
   180004dd5:	f2 42 0f 11 8c cb 00 	movsd  QWORD PTR [rbx+r9*8+0x100],xmm1
   180004ddc:	01 00 00 
   180004ddf:	45 0f b7 48 10       	movzx  r9d,WORD PTR [r8+0x10]
   180004de4:	45 8d 51 ff          	lea    r10d,[r9-0x1]
   180004de8:	45 0f b7 d2          	movzx  r10d,r10w
   180004dec:	41 81 fa 61 01 00 00 	cmp    r10d,0x161
   180004df3:	77 21                	ja     0x180004e16
   180004df5:	f2 0f 10 8c d4 80 00 	movsd  xmm1,QWORD PTR [rsp+rdx*8+0x80]
   180004dfc:	00 00 
   180004dfe:	f2 0f 5e c8          	divsd  xmm1,xmm0
   180004e02:	f2 42 0f 58 8c cb 00 	addsd  xmm1,QWORD PTR [rbx+r9*8+0x100]
   180004e09:	01 00 00 
   180004e0c:	f2 42 0f 11 8c cb 00 	movsd  QWORD PTR [rbx+r9*8+0x100],xmm1
   180004e13:	01 00 00 
   180004e16:	45 0f b7 48 12       	movzx  r9d,WORD PTR [r8+0x12]
   180004e1b:	45 8d 51 ff          	lea    r10d,[r9-0x1]
   180004e1f:	45 0f b7 d2          	movzx  r10d,r10w
   180004e23:	41 81 fa 61 01 00 00 	cmp    r10d,0x161
   180004e2a:	77 21                	ja     0x180004e4d
   180004e2c:	f2 0f 10 8c d4 80 00 	movsd  xmm1,QWORD PTR [rsp+rdx*8+0x80]
   180004e33:	00 00 
   180004e35:	f2 0f 5e c8          	divsd  xmm1,xmm0
   180004e39:	f2 42 0f 58 8c cb 00 	addsd  xmm1,QWORD PTR [rbx+r9*8+0x100]
   180004e40:	01 00 00 
   180004e43:	f2 42 0f 11 8c cb 00 	movsd  QWORD PTR [rbx+r9*8+0x100],xmm1
   180004e4a:	01 00 00 
   180004e4d:	45 0f b7 48 14       	movzx  r9d,WORD PTR [r8+0x14]
   180004e52:	45 8d 51 ff          	lea    r10d,[r9-0x1]
   180004e56:	45 0f b7 d2          	movzx  r10d,r10w
   180004e5a:	41 81 fa 61 01 00 00 	cmp    r10d,0x161
   180004e61:	77 21                	ja     0x180004e84
   180004e63:	f2 0f 10 8c d4 80 00 	movsd  xmm1,QWORD PTR [rsp+rdx*8+0x80]
   180004e6a:	00 00 
   180004e6c:	f2 0f 5e c8          	divsd  xmm1,xmm0
   180004e70:	f2 42 0f 58 8c cb 00 	addsd  xmm1,QWORD PTR [rbx+r9*8+0x100]
   180004e77:	01 00 00 
   180004e7a:	f2 42 0f 11 8c cb 00 	movsd  QWORD PTR [rbx+r9*8+0x100],xmm1
   180004e81:	01 00 00 
   180004e84:	45 0f b7 48 16       	movzx  r9d,WORD PTR [r8+0x16]
   180004e89:	45 8d 51 ff          	lea    r10d,[r9-0x1]
   180004e8d:	45 0f b7 d2          	movzx  r10d,r10w
   180004e91:	41 81 fa 61 01 00 00 	cmp    r10d,0x161
   180004e98:	77 21                	ja     0x180004ebb
   180004e9a:	f2 0f 10 8c d4 80 00 	movsd  xmm1,QWORD PTR [rsp+rdx*8+0x80]
   180004ea1:	00 00 
   180004ea3:	f2 0f 5e c8          	divsd  xmm1,xmm0
   180004ea7:	f2 42 0f 58 8c cb 00 	addsd  xmm1,QWORD PTR [rbx+r9*8+0x100]
   180004eae:	01 00 00 
   180004eb1:	f2 42 0f 11 8c cb 00 	movsd  QWORD PTR [rbx+r9*8+0x100],xmm1
   180004eb8:	01 00 00 
   180004ebb:	45 0f b7 48 18       	movzx  r9d,WORD PTR [r8+0x18]
   180004ec0:	45 8d 51 ff          	lea    r10d,[r9-0x1]
   180004ec4:	45 0f b7 d2          	movzx  r10d,r10w
   180004ec8:	41 81 fa 61 01 00 00 	cmp    r10d,0x161
   180004ecf:	77 21                	ja     0x180004ef2
   180004ed1:	f2 0f 10 8c d4 80 00 	movsd  xmm1,QWORD PTR [rsp+rdx*8+0x80]
   180004ed8:	00 00 
   180004eda:	f2 0f 5e c8          	divsd  xmm1,xmm0
   180004ede:	f2 42 0f 58 8c cb 00 	addsd  xmm1,QWORD PTR [rbx+r9*8+0x100]
   180004ee5:	01 00 00 
   180004ee8:	f2 42 0f 11 8c cb 00 	movsd  QWORD PTR [rbx+r9*8+0x100],xmm1
   180004eef:	01 00 00 
   180004ef2:	45 0f b7 48 1a       	movzx  r9d,WORD PTR [r8+0x1a]
   180004ef7:	45 8d 51 ff          	lea    r10d,[r9-0x1]
   180004efb:	45 0f b7 d2          	movzx  r10d,r10w
   180004eff:	41 81 fa 61 01 00 00 	cmp    r10d,0x161
   180004f06:	77 21                	ja     0x180004f29
   180004f08:	f2 0f 10 8c d4 80 00 	movsd  xmm1,QWORD PTR [rsp+rdx*8+0x80]
   180004f0f:	00 00 
   180004f11:	f2 0f 5e c8          	divsd  xmm1,xmm0
   180004f15:	f2 42 0f 58 8c cb 00 	addsd  xmm1,QWORD PTR [rbx+r9*8+0x100]
   180004f1c:	01 00 00 
   180004f1f:	f2 42 0f 11 8c cb 00 	movsd  QWORD PTR [rbx+r9*8+0x100],xmm1
   180004f26:	01 00 00 
   180004f29:	45 0f b7 48 1c       	movzx  r9d,WORD PTR [r8+0x1c]
   180004f2e:	45 8d 51 ff          	lea    r10d,[r9-0x1]
   180004f32:	45 0f b7 d2          	movzx  r10d,r10w
   180004f36:	41 81 fa 61 01 00 00 	cmp    r10d,0x161
   180004f3d:	77 21                	ja     0x180004f60
   180004f3f:	f2 0f 10 8c d4 80 00 	movsd  xmm1,QWORD PTR [rsp+rdx*8+0x80]
   180004f46:	00 00 
   180004f48:	f2 0f 5e c8          	divsd  xmm1,xmm0
   180004f4c:	f2 42 0f 58 8c cb 00 	addsd  xmm1,QWORD PTR [rbx+r9*8+0x100]
   180004f53:	01 00 00 
   180004f56:	f2 42 0f 11 8c cb 00 	movsd  QWORD PTR [rbx+r9*8+0x100],xmm1
   180004f5d:	01 00 00 
   180004f60:	45 0f b7 40 1e       	movzx  r8d,WORD PTR [r8+0x1e]
   180004f65:	45 8d 48 ff          	lea    r9d,[r8-0x1]
   180004f69:	45 0f b7 c9          	movzx  r9d,r9w
   180004f6d:	41 81 f9 61 01 00 00 	cmp    r9d,0x161
   180004f74:	0f 87 f6 fd ff ff    	ja     0x180004d70
   180004f7a:	f2 0f 10 8c d4 80 00 	movsd  xmm1,QWORD PTR [rsp+rdx*8+0x80]
   180004f81:	00 00 
   180004f83:	f2 0f 5e c8          	divsd  xmm1,xmm0
   180004f87:	f2 42 0f 58 8c c3 00 	addsd  xmm1,QWORD PTR [rbx+r8*8+0x100]
   180004f8e:	01 00 00 
   180004f91:	f2 42 0f 11 8c c3 00 	movsd  QWORD PTR [rbx+r8*8+0x100],xmm1
   180004f98:	01 00 00 
   180004f9b:	e9 d0 fd ff ff       	jmp    0x180004d70
   180004fa0:	66 0f 57 f6          	xorpd  xmm6,xmm6
   180004fa4:	41 bc 01 00 00 00    	mov    r12d,0x1
   180004faa:	31 f6                	xor    esi,esi
   180004fac:	66 45 0f 57 c9       	xorpd  xmm9,xmm9
   180004fb1:	eb 1d                	jmp    0x180004fd0
   180004fb3:	66 66 66 66 2e 0f 1f 	data16 data16 data16 cs nop WORD PTR [rax+rax*1+0x0]
   180004fba:	84 00 00 00 00 00 
   180004fc0:	49 ff c4             	inc    r12
   180004fc3:	48 83 c6 08          	add    rsi,0x8
   180004fc7:	48 81 fe 10 0b 00 00 	cmp    rsi,0xb10
   180004fce:	74 67                	je     0x180005037
   180004fd0:	42 0f b6 04 27       	movzx  eax,BYTE PTR [rdi+r12*1]
   180004fd5:	84 c0                	test   al,al
   180004fd7:	74 e7                	je     0x180004fc0
   180004fd9:	48 8d 0d f0 3f 03 00 	lea    rcx,[rip+0x33ff0]        # 0x180038fd0
   180004fe0:	0f b6 4c 71 10       	movzx  ecx,BYTE PTR [rcx+rsi*2+0x10]
   180004fe5:	89 ca                	mov    edx,ecx
   180004fe7:	83 ca 02             	or     edx,0x2
   180004fea:	83 fa 03             	cmp    edx,0x3
   180004fed:	75 d1                	jne    0x180004fc0
   180004fef:	38 c8                	cmp    al,cl
   180004ff1:	75 cd                	jne    0x180004fc0
   180004ff3:	f2 0f 10 84 33 08 01 	movsd  xmm0,QWORD PTR [rbx+rsi*1+0x108]
   180004ffa:	00 00 
   180004ffc:	66 0f 2e c6          	ucomisd xmm0,xmm6
   180005000:	75 02                	jne    0x180005004
   180005002:	7b bc                	jnp    0x180004fc0
   180005004:	48 89 e9             	mov    rcx,rbp
   180005007:	44 89 e2             	mov    edx,r12d
   18000500a:	e8 f1 d7 ff ff       	call   0x180002800
   18000500f:	85 c0                	test   eax,eax
   180005011:	74 ad                	je     0x180004fc0
   180005013:	f2 0f 10 84 33 08 01 	movsd  xmm0,QWORD PTR [rbx+rsi*1+0x108]
   18000501a:	00 00 
   18000501c:	f2 0f 11 84 33 20 0c 	movsd  QWORD PTR [rbx+rsi*1+0xc20],xmm0
   180005023:	00 00 
   180005025:	f2 41 0f 5f c1       	maxsd  xmm0,xmm9
   18000502a:	ff 83 58 22 00 00    	inc    DWORD PTR [rbx+0x2258]
   180005030:	66 44 0f 28 c8       	movapd xmm9,xmm0
   180005035:	eb 89                	jmp    0x180004fc0
   180005037:	83 bb 58 22 00 00 00 	cmp    DWORD PTR [rbx+0x2258],0x0
   18000503e:	b8 fe ff ff ff       	mov    eax,0xfffffffe
   180005043:	0f 84 60 e5 ff ff    	je     0x1800035a9
   180005049:	0f 57 c0             	xorps  xmm0,xmm0
   18000504c:	f2 0f 2a 44 24 20    	cvtsi2sd xmm0,DWORD PTR [rsp+0x20]
   180005052:	f2 0f 59 05 5e 6d 03 	mulsd  xmm0,QWORD PTR [rip+0x36d5e]        # 0x18003bdb8
   180005059:	00 
   18000505a:	f2 0f 58 05 0e 6d 03 	addsd  xmm0,QWORD PTR [rip+0x36d0e]        # 0x18003bd70
   180005061:	00 
   180005062:	48 8d 8b 80 00 00 00 	lea    rcx,[rbx+0x80]
   180005069:	48 8d 93 88 00 00 00 	lea    rdx,[rbx+0x88]
   180005070:	4c 8d 83 98 00 00 00 	lea    r8,[rbx+0x98]
   180005077:	48 8d 83 90 00 00 00 	lea    rax,[rbx+0x90]
   18000507e:	48 89 44 24 30       	mov    QWORD PTR [rsp+0x30],rax
   180005083:	48 8d 83 a8 00 00 00 	lea    rax,[rbx+0xa8]
   18000508a:	48 89 44 24 60       	mov    QWORD PTR [rsp+0x60],rax
   18000508f:	48 8d 83 a0 00 00 00 	lea    rax,[rbx+0xa0]
   180005096:	48 89 44 24 50       	mov    QWORD PTR [rsp+0x50],rax
   18000509b:	48 8d 83 b8 00 00 00 	lea    rax,[rbx+0xb8]
   1800050a2:	48 89 44 24 58       	mov    QWORD PTR [rsp+0x58],rax
   1800050a7:	48 8d 83 b0 00 00 00 	lea    rax,[rbx+0xb0]
   1800050ae:	48 89 44 24 48       	mov    QWORD PTR [rsp+0x48],rax
   1800050b3:	48 8d 83 c8 00 00 00 	lea    rax,[rbx+0xc8]
   1800050ba:	48 89 44 24 38       	mov    QWORD PTR [rsp+0x38],rax
   1800050bf:	48 8d 83 c0 00 00 00 	lea    rax,[rbx+0xc0]
   1800050c6:	48 89 44 24 68       	mov    QWORD PTR [rsp+0x68],rax
   1800050cb:	48 8d 83 d8 00 00 00 	lea    rax,[rbx+0xd8]
   1800050d2:	48 89 44 24 28       	mov    QWORD PTR [rsp+0x28],rax
   1800050d7:	48 8d ab d0 00 00 00 	lea    rbp,[rbx+0xd0]
   1800050de:	48 8d 83 e8 00 00 00 	lea    rax,[rbx+0xe8]
   1800050e5:	48 89 84 24 08 01 00 	mov    QWORD PTR [rsp+0x108],rax
   1800050ec:	00 
   1800050ed:	4c 8d 8b e0 00 00 00 	lea    r9,[rbx+0xe0]
   1800050f4:	48 8d 83 f8 00 00 00 	lea    rax,[rbx+0xf8]
   1800050fb:	48 89 44 24 70       	mov    QWORD PTR [rsp+0x70],rax
   180005100:	66 0f ef c9          	pxor   xmm1,xmm1
   180005104:	f2 0f 10 15 d4 6c 03 	movsd  xmm2,QWORD PTR [rip+0x36cd4]        # 0x18003bde0
   18000510b:	00 
   18000510c:	f2 0f 10 1d d4 6c 03 	movsd  xmm3,QWORD PTR [rip+0x36cd4]        # 0x18003bde8
   180005113:	00 
   180005114:	f2 0f 10 25 e4 6c 03 	movsd  xmm4,QWORD PTR [rip+0x36ce4]        # 0x18003be00
   18000511b:	00 
   18000511c:	f2 44 0f 10 15 eb 6c 	movsd  xmm10,QWORD PTR [rip+0x36ceb]        # 0x18003be10
   180005123:	03 00 
   180005125:	f2 44 0f 10 1d ca 6c 	movsd  xmm11,QWORD PTR [rip+0x36cca]        # 0x18003bdf8
   18000512c:	03 00 
   18000512e:	f2 44 0f 10 25 b9 6c 	movsd  xmm12,QWORD PTR [rip+0x36cb9]        # 0x18003bdf0
   180005135:	03 00 
   180005137:	f2 0f 10 2d 91 6c 03 	movsd  xmm5,QWORD PTR [rip+0x36c91]        # 0x18003bdd0
   18000513e:	00 
   18000513f:	48 8d bb f0 00 00 00 	lea    rdi,[rbx+0xf0]
   180005146:	4c 8d 2d 95 3e 03 00 	lea    r13,[rip+0x33e95]        # 0x180038fe2
   18000514d:	45 31 db             	xor    r11d,r11d
   180005150:	eb 3d                	jmp    0x18000518f
   180005152:	66 66 66 66 66 2e 0f 	data16 data16 data16 data16 cs nop WORD PTR [rax+rax*1+0x0]
   180005159:	1f 84 00 00 00 00 00 
   180005160:	c1 e0 04             	shl    eax,0x4
   180005163:	48 01 c8             	add    rax,rcx
   180005166:	48 8b 9c 24 f8 2c 00 	mov    rbx,QWORD PTR [rsp+0x2cf8]
   18000516d:	00 
   18000516e:	4d 8d 53 01          	lea    r10,[r11+0x1]
   180005172:	66 44 89 10          	mov    WORD PTR [rax],r10w
   180005176:	f2 45 0f 11 3e       	movsd  QWORD PTR [r14],xmm15
   18000517b:	49 ff c3             	inc    r11
   18000517e:	49 83 c5 10          	add    r13,0x10
   180005182:	49 81 fb 62 01 00 00 	cmp    r11,0x162
   180005189:	0f 84 61 03 00 00    	je     0x1800054f0
   18000518f:	f2 42 0f 10 b4 db 20 	movsd  xmm6,QWORD PTR [rbx+r11*8+0xc20]
   180005196:	0c 00 00 
   180005199:	66 0f 2e f1          	ucomisd xmm6,xmm1
   18000519d:	76 dc                	jbe    0x18000517b
   18000519f:	66 41 83 fb ff       	cmp    r11w,0xffff
   1800051a4:	74 14                	je     0x1800051ba
   1800051a6:	48 8d 05 f3 69 03 00 	lea    rax,[rip+0x369f3]        # 0x18003bba0
   1800051ad:	41 0f b6 44 03 01    	movzx  eax,BYTE PTR [r11+rax*1+0x1]
   1800051b3:	66 41 03 45 00       	add    ax,WORD PTR [r13+0x0]
   1800051b8:	eb 02                	jmp    0x1800051bc
   1800051ba:	31 c0                	xor    eax,eax
   1800051bc:	0f b7 c0             	movzx  eax,ax
   1800051bf:	0f 57 f6             	xorps  xmm6,xmm6
   1800051c2:	f2 0f 2a f0          	cvtsi2sd xmm6,eax
   1800051c6:	66 44 0f 28 fe       	movapd xmm15,xmm6
   1800051cb:	f2 44 0f 59 fa       	mulsd  xmm15,xmm2
   1800051d0:	f2 0f 58 f3          	addsd  xmm6,xmm3
   1800051d4:	f2 44 0f 5e fe       	divsd  xmm15,xmm6
   1800051d9:	f2 44 0f 58 3d ae 6b 	addsd  xmm15,QWORD PTR [rip+0x36bae]        # 0x18003bd90
   1800051e0:	03 00 
   1800051e2:	f2 46 0f 10 84 db 08 	movsd  xmm8,QWORD PTR [rbx+r11*8+0x108]
   1800051e9:	01 00 00 
   1800051ec:	66 41 0f 28 f0       	movapd xmm6,xmm8
   1800051f1:	80 7c 24 27 00       	cmp    BYTE PTR [rsp+0x27],0x0
   1800051f6:	74 24                	je     0x18000521c
   1800051f8:	8b 83 58 22 00 00    	mov    eax,DWORD PTR [rbx+0x2258]
   1800051fe:	0f 57 f6             	xorps  xmm6,xmm6
   180005201:	f2 48 0f 2a f0       	cvtsi2sd xmm6,rax
   180005206:	66 41 0f 28 fc       	movapd xmm7,xmm12
   18000520b:	f2 0f 5e fe          	divsd  xmm7,xmm6
   18000520f:	66 41 0f 28 f0       	movapd xmm6,xmm8
   180005214:	f2 0f 59 f5          	mulsd  xmm6,xmm5
   180005218:	f2 0f 58 f7          	addsd  xmm6,xmm7
   18000521c:	f2 45 0f 5e c1       	divsd  xmm8,xmm9
   180005221:	66 41 0f 28 fd       	movapd xmm7,xmm13
   180005226:	f2 41 0f 5d f8       	minsd  xmm7,xmm8
   18000522b:	f2 42 0f 11 bc db 38 	movsd  QWORD PTR [rbx+r11*8+0x1738],xmm7
   180005232:	17 00 00 
   180005235:	41 80 7d fe 03       	cmp    BYTE PTR [r13-0x2],0x3
   18000523a:	0f 85 c0 00 00 00    	jne    0x180005300
   180005240:	66 45 0f 28 c5       	movapd xmm8,xmm13
   180005245:	80 bc 24 f0 2c 00 00 	cmp    BYTE PTR [rsp+0x2cf0],0x0
   18000524c:	00 
   18000524d:	75 28                	jne    0x180005277
   18000524f:	48 8b 44 24 78       	mov    rax,QWORD PTR [rsp+0x78]
   180005254:	f2 44 0f 10 40 18    	movsd  xmm8,QWORD PTR [rax+0x18]
   18000525a:	f2 44 0f 59 c4       	mulsd  xmm8,xmm4
   18000525f:	66 45 0f 28 f5       	movapd xmm14,xmm13
   180005264:	f2 45 0f 5d f0       	minsd  xmm14,xmm8
   180005269:	f2 44 0f 10 05 96 6b 	movsd  xmm8,QWORD PTR [rip+0x36b96]        # 0x18003be08
   180005270:	03 00 
   180005272:	f2 45 0f 5f c6       	maxsd  xmm8,xmm14
   180005277:	31 c0                	xor    eax,eax
   180005279:	4c 8b 54 24 40       	mov    r10,QWORD PTR [rsp+0x40]
   18000527e:	41 80 7a 19 00       	cmp    BYTE PTR [r10+0x19],0x0
   180005283:	f2 45 0f 59 c2       	mulsd  xmm8,xmm10
   180005288:	0f 94 c0             	sete   al
   18000528b:	4c 8d 15 8e 6b 03 00 	lea    r10,[rip+0x36b8e]        # 0x18003be20
   180005292:	f2 45 0f 59 04 c2    	mulsd  xmm8,QWORD PTR [r10+rax*8]
   180005298:	48 8b 44 24 78       	mov    rax,QWORD PTR [rsp+0x78]
   18000529d:	f2 44 0f 10 70 20    	movsd  xmm14,QWORD PTR [rax+0x20]
   1800052a3:	f2 44 0f 59 35 cc 6a 	mulsd  xmm14,QWORD PTR [rip+0x36acc]        # 0x18003bd78
   1800052aa:	03 00 
   1800052ac:	f2 45 0f 58 f5       	addsd  xmm14,xmm13
   1800052b1:	f2 45 0f 59 f0       	mulsd  xmm14,xmm8
   1800052b6:	f2 44 0f 5e f0       	divsd  xmm14,xmm0
   1800052bb:	f2 41 0f 59 f6       	mulsd  xmm6,xmm14
   1800052c0:	80 78 16 00          	cmp    BYTE PTR [rax+0x16],0x0
   1800052c4:	74 69                	je     0x18000532f
   1800052c6:	66 44 0f 28 c6       	movapd xmm8,xmm6
   1800052cb:	f2 45 0f 59 c3       	mulsd  xmm8,xmm11
   1800052d0:	66 44 0f 28 f4       	movapd xmm14,xmm4
   1800052d5:	f2 44 0f c2 f7 06    	cmpnlesd xmm14,xmm7
   1800052db:	66 41 0f 54 f6       	andpd  xmm6,xmm14
   1800052e0:	66 45 0f 55 f0       	andnpd xmm14,xmm8
   1800052e5:	66 44 0f 56 f6       	orpd   xmm14,xmm6
   1800052ea:	f2 44 0f 59 35 3d 6b 	mulsd  xmm14,QWORD PTR [rip+0x36b3d]        # 0x18003be30
   1800052f1:	03 00 
   1800052f3:	66 41 0f 28 f6       	movapd xmm6,xmm14
   1800052f8:	eb 35                	jmp    0x18000532f
   1800052fa:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
   180005300:	48 8b 44 24 78       	mov    rax,QWORD PTR [rsp+0x78]
   180005305:	80 78 16 00          	cmp    BYTE PTR [rax+0x16],0x0
   180005309:	74 24                	je     0x18000532f
   18000530b:	66 44 0f 28 c6       	movapd xmm8,xmm6
   180005310:	f2 45 0f 59 c3       	mulsd  xmm8,xmm11
   180005315:	66 44 0f 28 f4       	movapd xmm14,xmm4
   18000531a:	f2 44 0f c2 f7 06    	cmpnlesd xmm14,xmm7
   180005320:	66 41 0f 54 f6       	andpd  xmm6,xmm14
   180005325:	66 45 0f 55 f0       	andnpd xmm14,xmm8
   18000532a:	66 41 0f 56 f6       	orpd   xmm6,xmm14
   18000532f:	f2 44 0f 59 fe       	mulsd  xmm15,xmm6
   180005334:	f2 46 0f 11 bc db 20 	movsd  QWORD PTR [rbx+r11*8+0xc20],xmm15
   18000533b:	0c 00 00 
   18000533e:	f2 0f 10 b3 60 22 00 	movsd  xmm6,QWORD PTR [rbx+0x2260]
   180005345:	00 
   180005346:	f2 41 0f 58 f7       	addsd  xmm6,xmm15
   18000534b:	f2 0f 11 b3 60 22 00 	movsd  QWORD PTR [rbx+0x2260],xmm6
   180005352:	00 
   180005353:	f2 0f 10 bb 88 00 00 	movsd  xmm7,QWORD PTR [rbx+0x88]
   18000535a:	00 
   18000535b:	40 b6 01             	mov    sil,0x1
   18000535e:	66 44 0f 2e ff       	ucomisd xmm15,xmm7
   180005363:	76 1b                	jbe    0x180005380
   180005365:	41 b2 01             	mov    r10b,0x1
   180005368:	41 b4 01             	mov    r12b,0x1
   18000536b:	b3 01                	mov    bl,0x1
   18000536d:	41 b7 01             	mov    r15b,0x1
   180005370:	31 c0                	xor    eax,eax
   180005372:	49 89 d6             	mov    r14,rdx
   180005375:	e9 b6 00 00 00       	jmp    0x180005430
   18000537a:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
   180005380:	b8 01 00 00 00       	mov    eax,0x1
   180005385:	66 45 0f 2e 38       	ucomisd xmm15,QWORD PTR [r8]
   18000538a:	76 13                	jbe    0x18000539f
   18000538c:	41 b2 01             	mov    r10b,0x1
   18000538f:	41 b4 01             	mov    r12b,0x1
   180005392:	b3 01                	mov    bl,0x1
   180005394:	41 b7 01             	mov    r15b,0x1
   180005397:	4d 89 c6             	mov    r14,r8
   18000539a:	e9 91 00 00 00       	jmp    0x180005430
   18000539f:	4c 8b 74 24 60       	mov    r14,QWORD PTR [rsp+0x60]
   1800053a4:	66 45 0f 2e 3e       	ucomisd xmm15,QWORD PTR [r14]
   1800053a9:	76 12                	jbe    0x1800053bd
   1800053ab:	b8 02 00 00 00       	mov    eax,0x2
   1800053b0:	45 31 ff             	xor    r15d,r15d
   1800053b3:	41 b2 01             	mov    r10b,0x1
   1800053b6:	41 b4 01             	mov    r12b,0x1
   1800053b9:	b3 01                	mov    bl,0x1
   1800053bb:	eb 73                	jmp    0x180005430
   1800053bd:	4c 8b 74 24 58       	mov    r14,QWORD PTR [rsp+0x58]
   1800053c2:	66 45 0f 2e 3e       	ucomisd xmm15,QWORD PTR [r14]
   1800053c7:	76 0f                	jbe    0x1800053d8
   1800053c9:	b8 03 00 00 00       	mov    eax,0x3
   1800053ce:	31 db                	xor    ebx,ebx
   1800053d0:	41 b2 01             	mov    r10b,0x1
   1800053d3:	41 b4 01             	mov    r12b,0x1
   1800053d6:	eb 4e                	jmp    0x180005426
   1800053d8:	4c 8b 74 24 38       	mov    r14,QWORD PTR [rsp+0x38]
   1800053dd:	66 45 0f 2e 3e       	ucomisd xmm15,QWORD PTR [r14]
   1800053e2:	76 0d                	jbe    0x1800053f1
   1800053e4:	b8 04 00 00 00       	mov    eax,0x4
   1800053e9:	45 31 e4             	xor    r12d,r12d
   1800053ec:	41 b2 01             	mov    r10b,0x1
   1800053ef:	eb 33                	jmp    0x180005424
   1800053f1:	4c 8b 74 24 28       	mov    r14,QWORD PTR [rsp+0x28]
   1800053f6:	66 45 0f 2e 3e       	ucomisd xmm15,QWORD PTR [r14]
   1800053fb:	76 07                	jbe    0x180005404
   1800053fd:	b8 05 00 00 00       	mov    eax,0x5
   180005402:	eb 1a                	jmp    0x18000541e
   180005404:	4c 8b b4 24 08 01 00 	mov    r14,QWORD PTR [rsp+0x108]
   18000540b:	00 
   18000540c:	66 45 0f 2e 3e       	ucomisd xmm15,QWORD PTR [r14]
   180005411:	0f 86 c1 00 00 00    	jbe    0x1800054d8
   180005417:	b8 06 00 00 00       	mov    eax,0x6
   18000541c:	31 f6                	xor    esi,esi
   18000541e:	45 31 d2             	xor    r10d,r10d
   180005421:	45 31 e4             	xor    r12d,r12d
   180005424:	31 db                	xor    ebx,ebx
   180005426:	45 31 ff             	xor    r15d,r15d
   180005429:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
   180005430:	66 41 0f 10 31       	movupd xmm6,XMMWORD PTR [r9]
   180005435:	66 0f 11 37          	movupd XMMWORD PTR [rdi],xmm6
   180005439:	40 84 f6             	test   sil,sil
   18000543c:	0f 84 1e fd ff ff    	je     0x180005160
   180005442:	66 0f 10 75 00       	movupd xmm6,XMMWORD PTR [rbp+0x0]
   180005447:	66 41 0f 11 31       	movupd XMMWORD PTR [r9],xmm6
   18000544c:	45 84 d2             	test   r10b,r10b
   18000544f:	0f 84 0b fd ff ff    	je     0x180005160
   180005455:	4c 8b 54 24 68       	mov    r10,QWORD PTR [rsp+0x68]
   18000545a:	66 41 0f 10 32       	movupd xmm6,XMMWORD PTR [r10]
   18000545f:	66 0f 11 75 00       	movupd XMMWORD PTR [rbp+0x0],xmm6
   180005464:	45 84 e4             	test   r12b,r12b
   180005467:	0f 84 f3 fc ff ff    	je     0x180005160
   18000546d:	4c 8b 54 24 48       	mov    r10,QWORD PTR [rsp+0x48]
   180005472:	66 41 0f 10 32       	movupd xmm6,XMMWORD PTR [r10]
   180005477:	4c 8b 54 24 68       	mov    r10,QWORD PTR [rsp+0x68]
   18000547c:	66 41 0f 11 32       	movupd XMMWORD PTR [r10],xmm6
   180005481:	84 db                	test   bl,bl
   180005483:	0f 84 d7 fc ff ff    	je     0x180005160
   180005489:	4c 8b 54 24 50       	mov    r10,QWORD PTR [rsp+0x50]
   18000548e:	66 41 0f 10 32       	movupd xmm6,XMMWORD PTR [r10]
   180005493:	4c 8b 54 24 48       	mov    r10,QWORD PTR [rsp+0x48]
   180005498:	66 41 0f 11 32       	movupd XMMWORD PTR [r10],xmm6
   18000549d:	45 84 ff             	test   r15b,r15b
   1800054a0:	0f 84 ba fc ff ff    	je     0x180005160
   1800054a6:	66 44 0f 2e ff       	ucomisd xmm15,xmm7
   1800054ab:	4c 8b 54 24 30       	mov    r10,QWORD PTR [rsp+0x30]
   1800054b0:	66 41 0f 10 32       	movupd xmm6,XMMWORD PTR [r10]
   1800054b5:	4c 8b 54 24 50       	mov    r10,QWORD PTR [rsp+0x50]
   1800054ba:	66 41 0f 11 32       	movupd XMMWORD PTR [r10],xmm6
   1800054bf:	0f 86 9b fc ff ff    	jbe    0x180005160
   1800054c5:	66 0f 10 31          	movupd xmm6,XMMWORD PTR [rcx]
   1800054c9:	4c 8b 54 24 30       	mov    r10,QWORD PTR [rsp+0x30]
   1800054ce:	66 41 0f 11 32       	movupd XMMWORD PTR [r10],xmm6
   1800054d3:	e9 88 fc ff ff       	jmp    0x180005160
   1800054d8:	4c 8b 74 24 70       	mov    r14,QWORD PTR [rsp+0x70]
   1800054dd:	66 45 0f 2e 3e       	ucomisd xmm15,QWORD PTR [r14]
   1800054e2:	48 89 f8             	mov    rax,rdi
   1800054e5:	0f 87 83 fc ff ff    	ja     0x18000516e
   1800054eb:	e9 8b fc ff ff       	jmp    0x18000517b
   1800054f0:	66 83 39 00          	cmp    WORD PTR [rcx],0x0
   1800054f4:	74 18                	je     0x18000550e
   1800054f6:	f2 0f 10 83 88 00 00 	movsd  xmm0,QWORD PTR [rbx+0x88]
   1800054fd:	00 
   1800054fe:	f2 0f 5e 83 60 22 00 	divsd  xmm0,QWORD PTR [rbx+0x2260]
   180005505:	00 
   180005506:	f2 0f 11 83 88 00 00 	movsd  QWORD PTR [rbx+0x88],xmm0
   18000550d:	00 
   18000550e:	66 83 bb 90 00 00 00 	cmp    WORD PTR [rbx+0x90],0x0
   180005515:	00 
   180005516:	74 18                	je     0x180005530
   180005518:	f2 0f 10 83 98 00 00 	movsd  xmm0,QWORD PTR [rbx+0x98]
   18000551f:	00 
   180005520:	f2 0f 5e 83 60 22 00 	divsd  xmm0,QWORD PTR [rbx+0x2260]
   180005527:	00 
   180005528:	f2 0f 11 83 98 00 00 	movsd  QWORD PTR [rbx+0x98],xmm0
   18000552f:	00 
   180005530:	66 83 bb a0 00 00 00 	cmp    WORD PTR [rbx+0xa0],0x0
   180005537:	00 
   180005538:	74 18                	je     0x180005552
   18000553a:	f2 0f 10 83 a8 00 00 	movsd  xmm0,QWORD PTR [rbx+0xa8]
   180005541:	00 
   180005542:	f2 0f 5e 83 60 22 00 	divsd  xmm0,QWORD PTR [rbx+0x2260]
   180005549:	00 
   18000554a:	f2 0f 11 83 a8 00 00 	movsd  QWORD PTR [rbx+0xa8],xmm0
   180005551:	00 
   180005552:	66 83 bb b0 00 00 00 	cmp    WORD PTR [rbx+0xb0],0x0
   180005559:	00 
   18000555a:	74 18                	je     0x180005574
   18000555c:	f2 0f 10 83 b8 00 00 	movsd  xmm0,QWORD PTR [rbx+0xb8]
   180005563:	00 
   180005564:	f2 0f 5e 83 60 22 00 	divsd  xmm0,QWORD PTR [rbx+0x2260]
   18000556b:	00 
   18000556c:	f2 0f 11 83 b8 00 00 	movsd  QWORD PTR [rbx+0xb8],xmm0
   180005573:	00 
   180005574:	66 83 bb c0 00 00 00 	cmp    WORD PTR [rbx+0xc0],0x0
   18000557b:	00 
   18000557c:	74 18                	je     0x180005596
   18000557e:	f2 0f 10 83 c8 00 00 	movsd  xmm0,QWORD PTR [rbx+0xc8]
   180005585:	00 
   180005586:	f2 0f 5e 83 60 22 00 	divsd  xmm0,QWORD PTR [rbx+0x2260]
   18000558d:	00 
   18000558e:	f2 0f 11 83 c8 00 00 	movsd  QWORD PTR [rbx+0xc8],xmm0
   180005595:	00 
   180005596:	66 83 bb d0 00 00 00 	cmp    WORD PTR [rbx+0xd0],0x0
   18000559d:	00 
   18000559e:	74 18                	je     0x1800055b8
   1800055a0:	f2 0f 10 83 d8 00 00 	movsd  xmm0,QWORD PTR [rbx+0xd8]
   1800055a7:	00 
   1800055a8:	f2 0f 5e 83 60 22 00 	divsd  xmm0,QWORD PTR [rbx+0x2260]
   1800055af:	00 
   1800055b0:	f2 0f 11 83 d8 00 00 	movsd  QWORD PTR [rbx+0xd8],xmm0
   1800055b7:	00 
   1800055b8:	66 83 bb e0 00 00 00 	cmp    WORD PTR [rbx+0xe0],0x0
   1800055bf:	00 
   1800055c0:	74 18                	je     0x1800055da
   1800055c2:	f2 0f 10 83 e8 00 00 	movsd  xmm0,QWORD PTR [rbx+0xe8]
   1800055c9:	00 
   1800055ca:	f2 0f 5e 83 60 22 00 	divsd  xmm0,QWORD PTR [rbx+0x2260]
   1800055d1:	00 
   1800055d2:	f2 0f 11 83 e8 00 00 	movsd  QWORD PTR [rbx+0xe8],xmm0
   1800055d9:	00 
   1800055da:	b8 01 00 00 00       	mov    eax,0x1
   1800055df:	66 83 3f 00          	cmp    WORD PTR [rdi],0x0
   1800055e3:	0f 84 c0 df ff ff    	je     0x1800035a9
   1800055e9:	f2 0f 10 83 f8 00 00 	movsd  xmm0,QWORD PTR [rbx+0xf8]
   1800055f0:	00 
   1800055f1:	f2 0f 5e 83 60 22 00 	divsd  xmm0,QWORD PTR [rbx+0x2260]
   1800055f8:	00 
   1800055f9:	f2 0f 11 83 f8 00 00 	movsd  QWORD PTR [rbx+0xf8],xmm0
   180005600:	00 
   180005601:	e9 a3 df ff ff       	jmp    0x1800035a9
