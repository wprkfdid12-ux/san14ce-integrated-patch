
game_static_only.bin:     file format pei-x86-64


Disassembly of section .text:

0000000140781810 <.text+0x780810>:
   140781810:	48 89 5c 24 18       	mov    QWORD PTR [rsp+0x18],rbx
   140781815:	55                   	push   rbp
   140781816:	56                   	push   rsi
   140781817:	57                   	push   rdi
   140781818:	41 54                	push   r12
   14078181a:	41 55                	push   r13
   14078181c:	41 56                	push   r14
   14078181e:	41 57                	push   r15
   140781820:	48 83 ec 40          	sub    rsp,0x40
   140781824:	48 8b f1             	mov    rsi,rcx
   140781827:	e8 74 57 c7 ff       	call   0x1403f6fa0
   14078182c:	48 8b c8             	mov    rcx,rax
   14078182f:	e8 4c 09 a5 ff       	call   0x1401d2180
   140781834:	48 8b 86 90 01 00 00 	mov    rax,QWORD PTR [rsi+0x190]
   14078183b:	45 33 e4             	xor    r12d,r12d
   14078183e:	44 89 a4 24 80 00 00 	mov    DWORD PTR [rsp+0x80],r12d
   140781845:	00 
   140781846:	0f 57 c0             	xorps  xmm0,xmm0
   140781849:	45 8b f4             	mov    r14d,r12d
   14078184c:	45 8b fc             	mov    r15d,r12d
   14078184f:	f3 0f 7f 44 24 30    	movdqu XMMWORD PTR [rsp+0x30],xmm0
   140781855:	48 85 c0             	test   rax,rax
   140781858:	74 25                	je     0x14078187f
   14078185a:	4c 39 25 87 f8 04 02 	cmp    QWORD PTR [rip+0x204f887],r12        # 0x1427d10e8
   140781861:	74 1c                	je     0x14078187f
   140781863:	48 8b 0d 86 f8 04 02 	mov    rcx,QWORD PTR [rip+0x204f886]        # 0x1427d10f0
   14078186a:	8b 00                	mov    eax,DWORD PTR [rax]
   14078186c:	48 85 c9             	test   rcx,rcx
   14078186f:	74 0e                	je     0x14078187f
   140781871:	3b 05 a9 f8 04 02    	cmp    eax,DWORD PTR [rip+0x204f8a9]        # 0x1427d1120
   140781877:	73 06                	jae    0x14078187f
   140781879:	48 8b 3c c1          	mov    rdi,QWORD PTR [rcx+rax*8]
   14078187d:	eb 03                	jmp    0x140781882
   14078187f:	49 8b fc             	mov    rdi,r12
   140781882:	48 85 ff             	test   rdi,rdi
   140781885:	0f 84 18 01 00 00    	je     0x1407819a3
   14078188b:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
   140781890:	48 8b 1f             	mov    rbx,QWORD PTR [rdi]
   140781893:	48 8b cb             	mov    rcx,rbx
   140781896:	e8 25 cc ba ff       	call   0x14032e4c0
   14078189b:	85 c0                	test   eax,eax
   14078189d:	0f 84 eb 00 00 00    	je     0x14078198e
   1407818a3:	41 8b c6             	mov    eax,r14d
   1407818a6:	99                   	cdq
   1407818a7:	2b c2                	sub    eax,edx
   1407818a9:	d1 f8                	sar    eax,1
   1407818ab:	39 86 a8 01 00 00    	cmp    DWORD PTR [rsi+0x1a8],eax
   1407818b1:	75 18                	jne    0x1407818cb
   1407818b3:	41 8b c6             	mov    eax,r14d
   1407818b6:	25 01 00 00 80       	and    eax,0x80000001
   1407818bb:	7d 07                	jge    0x1407818c4
   1407818bd:	ff c8                	dec    eax
   1407818bf:	83 c8 fe             	or     eax,0xfffffffe
   1407818c2:	ff c0                	inc    eax
   1407818c4:	48 98                	cdqe
   1407818c6:	48 89 5c c4 30       	mov    QWORD PTR [rsp+rax*8+0x30],rbx
   1407818cb:	ba 01 00 00 00       	mov    edx,0x1
   1407818d0:	48 8b cb             	mov    rcx,rbx
   1407818d3:	41 ff c6             	inc    r14d
   1407818d6:	e8 95 12 ab ff       	call   0x140232b70
   1407818db:	85 c0                	test   eax,eax
   1407818dd:	75 24                	jne    0x140781903
   1407818df:	33 d2                	xor    edx,edx
   1407818e1:	48 8b cb             	mov    rcx,rbx
   1407818e4:	e8 57 0d ab ff       	call   0x140232640
   1407818e9:	85 c0                	test   eax,eax
   1407818eb:	75 16                	jne    0x140781903
   1407818ed:	0f b7 83 5e 01 00 00 	movzx  eax,WORD PTR [rbx+0x15e]
   1407818f4:	0f ba e0 0e          	bt     eax,0xe
   1407818f8:	72 09                	jb     0x140781903
   1407818fa:	0f b6 ab bc 01 00 00 	movzx  ebp,BYTE PTR [rbx+0x1bc]
   140781901:	eb 03                	jmp    0x140781906
   140781903:	41 8b ec             	mov    ebp,r12d
   140781906:	48 8b cb             	mov    rcx,rbx
   140781909:	e8 d2 e1 8b ff       	call   0x14003fae0
   14078190e:	48 8b 8e 98 01 00 00 	mov    rcx,QWORD PTR [rsi+0x198]
   140781915:	3b c5                	cmp    eax,ebp
   140781917:	44 0f b7 43 10       	movzx  r8d,WORD PTR [rbx+0x10]
   14078191c:	45 8b cc             	mov    r9d,r12d
   14078191f:	41 0f 9f c1          	setg   r9b
   140781923:	48 8b c1             	mov    rax,rcx
   140781926:	48 8b 51 08          	mov    rdx,QWORD PTR [rcx+0x8]
   14078192a:	44 38 62 19          	cmp    BYTE PTR [rdx+0x19],r12b
   14078192e:	75 18                	jne    0x140781948
   140781930:	44 39 42 1c          	cmp    DWORD PTR [rdx+0x1c],r8d
   140781934:	7d 06                	jge    0x14078193c
   140781936:	48 8b 52 10          	mov    rdx,QWORD PTR [rdx+0x10]
   14078193a:	eb 06                	jmp    0x140781942
   14078193c:	48 8b c2             	mov    rax,rdx
   14078193f:	48 8b 12             	mov    rdx,QWORD PTR [rdx]
   140781942:	44 38 62 19          	cmp    BYTE PTR [rdx+0x19],r12b
   140781946:	74 e8                	je     0x140781930
   140781948:	48 3b c1             	cmp    rax,rcx
   14078194b:	74 06                	je     0x140781953
   14078194d:	44 3b 40 1c          	cmp    r8d,DWORD PTR [rax+0x1c]
   140781951:	7d 03                	jge    0x140781956
   140781953:	48 8b c1             	mov    rax,rcx
   140781956:	48 3b c1             	cmp    rax,rcx
   140781959:	74 07                	je     0x140781962
   14078195b:	44 0f b6 40 20       	movzx  r8d,BYTE PTR [rax+0x20]
   140781960:	eb 03                	jmp    0x140781965
   140781962:	45 32 c0             	xor    r8b,r8b
   140781965:	45 85 c9             	test   r9d,r9d
   140781968:	75 21                	jne    0x14078198b
   14078196a:	0f b7 83 c2 01 00 00 	movzx  eax,WORD PTR [rbx+0x1c2]
   140781971:	0f b7 c8             	movzx  ecx,ax
   140781974:	66 c1 e9 08          	shr    cx,0x8
   140781978:	0f b6 d1             	movzx  edx,cl
   14078197b:	c1 e2 04             	shl    edx,0x4
   14078197e:	0f b6 c8             	movzx  ecx,al
   140781981:	03 d1                	add    edx,ecx
   140781983:	41 0f b6 c0          	movzx  eax,r8b
   140781987:	3b d0                	cmp    edx,eax
   140781989:	74 03                	je     0x14078198e
   14078198b:	41 ff c7             	inc    r15d
   14078198e:	48 8b 7f 08          	mov    rdi,QWORD PTR [rdi+0x8]
   140781992:	48 85 ff             	test   rdi,rdi
   140781995:	0f 85 f5 fe ff ff    	jne    0x140781890
   14078199b:	44 89 bc 24 80 00 00 	mov    DWORD PTR [rsp+0x80],r15d
   1407819a2:	00 
   1407819a3:	48 8b 54 24 30       	mov    rdx,QWORD PTR [rsp+0x30]
   1407819a8:	48 8b 8e f8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1f8]
   1407819af:	e8 ec fc 11 00       	call   0x1408a16a0
   1407819b4:	48 8b 54 24 38       	mov    rdx,QWORD PTR [rsp+0x38]
   1407819b9:	48 8b 8e 00 02 00 00 	mov    rcx,QWORD PTR [rsi+0x200]
   1407819c0:	e8 db fc 11 00       	call   0x1408a16a0
   1407819c5:	e8 d6 55 c7 ff       	call   0x1403f6fa0
   1407819ca:	48 8b d8             	mov    rbx,rax
   1407819cd:	48 8d 88 10 b0 04 00 	lea    rcx,[rax+0x4b010]
   1407819d4:	e8 b7 6d b8 ff       	call   0x140308790
   1407819d9:	48 8b c8             	mov    rcx,rax
   1407819dc:	ff 15 46 e9 bb 00    	call   QWORD PTR [rip+0xbbe946]        # 0x141340328
   1407819e2:	ff 43 08             	inc    DWORD PTR [rbx+0x8]
   1407819e5:	83 7b 08 1e          	cmp    DWORD PTR [rbx+0x8],0x1e
   1407819e9:	7c 04                	jl     0x1407819ef
   1407819eb:	44 89 63 08          	mov    DWORD PTR [rbx+0x8],r12d
   1407819ef:	48 63 43 08          	movsxd rax,DWORD PTR [rbx+0x8]
   1407819f3:	48 8d bb 0c d0 02 00 	lea    rdi,[rbx+0x2d00c]
   1407819fa:	48 c1 e0 0c          	shl    rax,0xc
   1407819fe:	b9 00 10 00 00       	mov    ecx,0x1000
   140781a03:	48 03 f8             	add    rdi,rax
   140781a06:	33 c0                	xor    eax,eax
   140781a08:	f3 aa                	rep stos BYTE PTR es:[rdi],al
   140781a0a:	48 63 7b 08          	movsxd rdi,DWORD PTR [rbx+0x8]
   140781a0e:	48 8d 8b 10 b0 04 00 	lea    rcx,[rbx+0x4b010]
   140781a15:	48 c1 e7 0c          	shl    rdi,0xc
   140781a19:	e8 72 6d b8 ff       	call   0x140308790
   140781a1e:	48 8b c8             	mov    rcx,rax
   140781a21:	ff 15 b1 e6 bb 00    	call   QWORD PTR [rip+0xbbe6b1]        # 0x1413400d8
   140781a27:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   140781a2e:	00 00 
   140781a30:	8b 0d ea f8 06 02    	mov    ecx,DWORD PTR [rip+0x206f8ea]        # 0x1427f1320
   140781a36:	ba 10 00 00 00       	mov    edx,0x10
   140781a3b:	4c 8b 2c c8          	mov    r13,QWORD PTR [rax+rcx*8]
   140781a3f:	4c 03 ea             	add    r13,rdx
   140781a42:	41 8b 45 00          	mov    eax,DWORD PTR [r13+0x0]
   140781a46:	39 05 1c 15 9c 01    	cmp    DWORD PTR [rip+0x19c151c],eax        # 0x142142f68
   140781a4c:	0f 8f 7b 06 00 00    	jg     0x1407820cd
   140781a52:	8b 86 a8 01 00 00    	mov    eax,DWORD PTR [rsi+0x1a8]
   140781a58:	48 8d 93 0c d0 02 00 	lea    rdx,[rbx+0x2d00c]
   140781a5f:	ff c0                	inc    eax
   140781a61:	4c 8d 0d 6c 4d c1 00 	lea    r9,[rip+0xc14d6c]        # 0x1413967d4
   140781a68:	41 b8 00 10 00 00    	mov    r8d,0x1000
   140781a6e:	89 44 24 20          	mov    DWORD PTR [rsp+0x20],eax
   140781a72:	48 03 d7             	add    rdx,rdi
   140781a75:	48 8d 0d e9 14 9c 01 	lea    rcx,[rip+0x19c14e9]        # 0x142142f65
   140781a7c:	e8 6f 48 a1 ff       	call   0x1401962f0
   140781a81:	41 b9 01 00 00 00    	mov    r9d,0x1
   140781a87:	4c 8d 83 0c d0 02 00 	lea    r8,[rbx+0x2d00c]
   140781a8e:	4c 03 c7             	add    r8,rdi
   140781a91:	48 8d 8e 30 01 00 00 	lea    rcx,[rsi+0x130]
   140781a98:	41 8d 51 1c          	lea    edx,[r9+0x1c]
   140781a9c:	e8 3f e7 1a 00       	call   0x1409301e0
   140781aa1:	48 8b 8e e8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1e8]
   140781aa8:	48 85 c9             	test   rcx,rcx
   140781aab:	74 15                	je     0x140781ac2
   140781aad:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   140781ab0:	41 8b d4             	mov    edx,r12d
   140781ab3:	39 96 a8 01 00 00    	cmp    DWORD PTR [rsi+0x1a8],edx
   140781ab9:	0f 97 c2             	seta   dl
   140781abc:	ff 90 d0 00 00 00    	call   QWORD PTR [rax+0xd0]
   140781ac2:	48 8b 8e f0 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1f0]
   140781ac9:	48 85 c9             	test   rcx,rcx
   140781acc:	74 1d                	je     0x140781aeb
   140781ace:	8b 86 ac 01 00 00    	mov    eax,DWORD PTR [rsi+0x1ac]
   140781ad4:	41 8b d4             	mov    edx,r12d
   140781ad7:	48 8b 39             	mov    rdi,QWORD PTR [rcx]
   140781ada:	ff c8                	dec    eax
   140781adc:	39 86 a8 01 00 00    	cmp    DWORD PTR [rsi+0x1a8],eax
   140781ae2:	0f 92 c2             	setb   dl
   140781ae5:	ff 97 d0 00 00 00    	call   QWORD PTR [rdi+0xd0]
   140781aeb:	e8 b0 54 c7 ff       	call   0x1403f6fa0
   140781af0:	48 8b d8             	mov    rbx,rax
   140781af3:	48 8d 88 10 b0 04 00 	lea    rcx,[rax+0x4b010]
   140781afa:	e8 91 6c b8 ff       	call   0x140308790
   140781aff:	48 8b c8             	mov    rcx,rax
   140781b02:	ff 15 20 e8 bb 00    	call   QWORD PTR [rip+0xbbe820]        # 0x141340328
   140781b08:	ff 43 08             	inc    DWORD PTR [rbx+0x8]
   140781b0b:	83 7b 08 1e          	cmp    DWORD PTR [rbx+0x8],0x1e
   140781b0f:	7c 04                	jl     0x140781b15
   140781b11:	44 89 63 08          	mov    DWORD PTR [rbx+0x8],r12d
   140781b15:	48 63 43 08          	movsxd rax,DWORD PTR [rbx+0x8]
   140781b19:	48 8d bb 0c d0 02 00 	lea    rdi,[rbx+0x2d00c]
   140781b20:	48 c1 e0 0c          	shl    rax,0xc
   140781b24:	b9 00 10 00 00       	mov    ecx,0x1000
   140781b29:	48 03 f8             	add    rdi,rax
   140781b2c:	33 c0                	xor    eax,eax
   140781b2e:	f3 aa                	rep stos BYTE PTR es:[rdi],al
   140781b30:	48 63 7b 08          	movsxd rdi,DWORD PTR [rbx+0x8]
   140781b34:	48 8d 8b 10 b0 04 00 	lea    rcx,[rbx+0x4b010]
   140781b3b:	48 c1 e7 0c          	shl    rdi,0xc
   140781b3f:	48 81 c7 0c d0 02 00 	add    rdi,0x2d00c
   140781b46:	48 03 fb             	add    rdi,rbx
   140781b49:	e8 42 6c b8 ff       	call   0x140308790
   140781b4e:	48 8b c8             	mov    rcx,rax
   140781b51:	ff 15 81 e5 bb 00    	call   QWORD PTR [rip+0xbbe581]        # 0x1413400d8
   140781b57:	41 8b 45 00          	mov    eax,DWORD PTR [r13+0x0]
   140781b5b:	39 05 07 14 9c 01    	cmp    DWORD PTR [rip+0x19c1407],eax        # 0x142142f68
   140781b61:	0f 8f 9c 05 00 00    	jg     0x140782103
   140781b67:	48 8b 8e b0 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1b0]
   140781b6e:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   140781b71:	ff 90 90 00 00 00    	call   QWORD PTR [rax+0x90]
   140781b77:	8b 0d cb 6c 2c 01    	mov    ecx,DWORD PTR [rip+0x12c6ccb]        # 0x141a48848
   140781b7d:	4c 8d 0d 50 4c c1 00 	lea    r9,[rip+0xc14c50]        # 0x1413967d4
   140781b84:	41 0f af cf          	imul   ecx,r15d
   140781b88:	41 b8 00 10 00 00    	mov    r8d,0x1000
   140781b8e:	48 8b d7             	mov    rdx,rdi
   140781b91:	2b c1                	sub    eax,ecx
   140781b93:	48 8d 0d cb 13 9c 01 	lea    rcx,[rip+0x19c13cb]        # 0x142142f65
   140781b9a:	89 44 24 20          	mov    DWORD PTR [rsp+0x20],eax
   140781b9e:	e8 4d 47 a1 ff       	call   0x1401962f0
   140781ba3:	41 b9 01 00 00 00    	mov    r9d,0x1
   140781ba9:	48 8d 8e 30 01 00 00 	lea    rcx,[rsi+0x130]
   140781bb0:	4c 8b c7             	mov    r8,rdi
   140781bb3:	41 8d 51 53          	lea    edx,[r9+0x53]
   140781bb7:	e8 24 e6 1a 00       	call   0x1409301e0
   140781bbc:	41 8b 45 00          	mov    eax,DWORD PTR [r13+0x0]
   140781bc0:	39 05 a2 13 9c 01    	cmp    DWORD PTR [rip+0x19c13a2],eax        # 0x142142f68
   140781bc6:	0f 8f 6d 05 00 00    	jg     0x140782139
   140781bcc:	48 8d 05 15 b0 cb 00 	lea    rax,[rip+0xcbb015]        # 0x14143cbe8
   140781bd3:	45 85 ff             	test   r15d,r15d
   140781bd6:	48 8d 0d e3 e6 cb 00 	lea    rcx,[rip+0xcbe6e3]        # 0x1414402c0
   140781bdd:	41 b8 00 10 00 00    	mov    r8d,0x1000
   140781be3:	48 0f 4f c8          	cmovg  rcx,rax
   140781be7:	48 8b d7             	mov    rdx,rdi
   140781bea:	8b 05 58 6c 2c 01    	mov    eax,DWORD PTR [rip+0x12c6c58]        # 0x141a48848
   140781bf0:	4c 8b c9             	mov    r9,rcx
   140781bf3:	41 0f af c7          	imul   eax,r15d
   140781bf7:	48 89 8c 24 88 00 00 	mov    QWORD PTR [rsp+0x88],rcx
   140781bfe:	00 
   140781bff:	48 8d 0d 5f 13 9c 01 	lea    rcx,[rip+0x19c135f]        # 0x142142f65
   140781c06:	89 44 24 20          	mov    DWORD PTR [rsp+0x20],eax
   140781c0a:	e8 e1 46 a1 ff       	call   0x1401962f0
   140781c0f:	41 b9 01 00 00 00    	mov    r9d,0x1
   140781c15:	48 8d 8e 30 01 00 00 	lea    rcx,[rsi+0x130]
   140781c1c:	4c 8b c7             	mov    r8,rdi
   140781c1f:	41 8d 51 54          	lea    edx,[r9+0x54]
   140781c23:	e8 b8 e5 1a 00       	call   0x1409301e0
   140781c28:	48 8b 86 90 01 00 00 	mov    rax,QWORD PTR [rsi+0x190]
   140781c2f:	41 be 00 40 01 00    	mov    r14d,0x14000
   140781c35:	48 85 c0             	test   rax,rax
   140781c38:	74 04                	je     0x140781c3e
   140781c3a:	8b 00                	mov    eax,DWORD PTR [rax]
   140781c3c:	eb 03                	jmp    0x140781c41
   140781c3e:	49 8b c6             	mov    rax,r14
   140781c41:	8b c8                	mov    ecx,eax
   140781c43:	ba 0e 00 00 00       	mov    edx,0xe
   140781c48:	48 8b 05 b9 f4 04 02 	mov    rax,QWORD PTR [rip+0x204f4b9]        # 0x1427d1108
   140781c4f:	4c 39 24 c8          	cmp    QWORD PTR [rax+rcx*8],r12
   140781c53:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   140781c5a:	77 20                	ja     0x140781c7c
   140781c5c:	45 33 c0             	xor    r8d,r8d
   140781c5f:	e8 dc 86 0d 00       	call   0x14085a340
   140781c64:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   140781c6b:	45 33 c0             	xor    r8d,r8d
   140781c6e:	41 8d 50 11          	lea    edx,[r8+0x11]
   140781c72:	e8 c9 86 0d 00       	call   0x14085a340
   140781c77:	45 33 c0             	xor    r8d,r8d
   140781c7a:	eb 26                	jmp    0x140781ca2
   140781c7c:	41 b8 01 00 00 00    	mov    r8d,0x1
   140781c82:	e8 b9 86 0d 00       	call   0x14085a340
   140781c87:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   140781c8e:	ba 11 00 00 00       	mov    edx,0x11
   140781c93:	44 8d 42 f0          	lea    r8d,[rdx-0x10]
   140781c97:	e8 a4 86 0d 00       	call   0x14085a340
   140781c9c:	41 b8 01 00 00 00    	mov    r8d,0x1
   140781ca2:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   140781ca9:	ba 12 00 00 00       	mov    edx,0x12
   140781cae:	e8 8d 86 0d 00       	call   0x14085a340
   140781cb3:	48 8b 86 b8 01 00 00 	mov    rax,QWORD PTR [rsi+0x1b8]
   140781cba:	44 8b 3d 93 6b 2c 01 	mov    r15d,DWORD PTR [rip+0x12c6b93]        # 0x141a48854
   140781cc1:	44 0f af bc 24 80 00 	imul   r15d,DWORD PTR [rsp+0x80]
   140781cc8:	00 00 
   140781cca:	44 0f b6 60 14       	movzx  r12d,BYTE PTR [rax+0x14]
   140781ccf:	e8 cc 52 c7 ff       	call   0x1403f6fa0
   140781cd4:	48 8b d8             	mov    rbx,rax
   140781cd7:	48 8d 88 10 b0 04 00 	lea    rcx,[rax+0x4b010]
   140781cde:	e8 ad 6a b8 ff       	call   0x140308790
   140781ce3:	48 8b c8             	mov    rcx,rax
   140781ce6:	ff 15 3c e6 bb 00    	call   QWORD PTR [rip+0xbbe63c]        # 0x141340328
   140781cec:	ff 43 08             	inc    DWORD PTR [rbx+0x8]
   140781cef:	83 7b 08 1e          	cmp    DWORD PTR [rbx+0x8],0x1e
   140781cf3:	7c 07                	jl     0x140781cfc
   140781cf5:	c7 43 08 00 00 00 00 	mov    DWORD PTR [rbx+0x8],0x0
   140781cfc:	48 63 43 08          	movsxd rax,DWORD PTR [rbx+0x8]
   140781d00:	48 8d bb 0c d0 02 00 	lea    rdi,[rbx+0x2d00c]
   140781d07:	48 c1 e0 0c          	shl    rax,0xc
   140781d0b:	b9 00 10 00 00       	mov    ecx,0x1000
   140781d10:	48 03 f8             	add    rdi,rax
   140781d13:	33 c0                	xor    eax,eax
   140781d15:	f3 aa                	rep stos BYTE PTR es:[rdi],al
   140781d17:	48 63 7b 08          	movsxd rdi,DWORD PTR [rbx+0x8]
   140781d1b:	48 8d 8b 10 b0 04 00 	lea    rcx,[rbx+0x4b010]
   140781d22:	48 c1 e7 0c          	shl    rdi,0xc
   140781d26:	e8 65 6a b8 ff       	call   0x140308790
   140781d2b:	48 8b c8             	mov    rcx,rax
   140781d2e:	ff 15 a4 e3 bb 00    	call   QWORD PTR [rip+0xbbe3a4]        # 0x1413400d8
   140781d34:	41 8b 45 00          	mov    eax,DWORD PTR [r13+0x0]
   140781d38:	39 05 2a 12 9c 01    	cmp    DWORD PTR [rip+0x19c122a],eax        # 0x142142f68
   140781d3e:	0f 8f 2b 04 00 00    	jg     0x14078216f
   140781d44:	48 8d 93 0c d0 02 00 	lea    rdx,[rbx+0x2d00c]
   140781d4b:	44 89 64 24 20       	mov    DWORD PTR [rsp+0x20],r12d
   140781d50:	48 03 d7             	add    rdx,rdi
   140781d53:	4c 8d 0d 7a 4a c1 00 	lea    r9,[rip+0xc14a7a]        # 0x1413967d4
   140781d5a:	41 b8 00 10 00 00    	mov    r8d,0x1000
   140781d60:	48 8d 0d fe 11 9c 01 	lea    rcx,[rip+0x19c11fe]        # 0x142142f65
   140781d67:	e8 84 45 a1 ff       	call   0x1401962f0
   140781d6c:	41 b9 01 00 00 00    	mov    r9d,0x1
   140781d72:	4c 8d 83 0c d0 02 00 	lea    r8,[rbx+0x2d00c]
   140781d79:	4c 03 c7             	add    r8,rdi
   140781d7c:	48 8d 8e 30 01 00 00 	lea    rcx,[rsi+0x130]
   140781d83:	41 8d 51 5b          	lea    edx,[r9+0x5b]
   140781d87:	e8 54 e4 1a 00       	call   0x1409301e0
   140781d8c:	e8 0f 52 c7 ff       	call   0x1403f6fa0
   140781d91:	48 8b d8             	mov    rbx,rax
   140781d94:	48 8d 88 10 b0 04 00 	lea    rcx,[rax+0x4b010]
   140781d9b:	e8 f0 69 b8 ff       	call   0x140308790
   140781da0:	48 8b c8             	mov    rcx,rax
   140781da3:	ff 15 7f e5 bb 00    	call   QWORD PTR [rip+0xbbe57f]        # 0x141340328
   140781da9:	ff 43 08             	inc    DWORD PTR [rbx+0x8]
   140781dac:	83 7b 08 1e          	cmp    DWORD PTR [rbx+0x8],0x1e
   140781db0:	7c 07                	jl     0x140781db9
   140781db2:	c7 43 08 00 00 00 00 	mov    DWORD PTR [rbx+0x8],0x0
   140781db9:	48 63 43 08          	movsxd rax,DWORD PTR [rbx+0x8]
   140781dbd:	48 8d bb 0c d0 02 00 	lea    rdi,[rbx+0x2d00c]
   140781dc4:	48 c1 e0 0c          	shl    rax,0xc
   140781dc8:	b9 00 10 00 00       	mov    ecx,0x1000
   140781dcd:	48 03 f8             	add    rdi,rax
   140781dd0:	33 c0                	xor    eax,eax
   140781dd2:	f3 aa                	rep stos BYTE PTR es:[rdi],al
   140781dd4:	48 63 7b 08          	movsxd rdi,DWORD PTR [rbx+0x8]
   140781dd8:	48 8d 8b 10 b0 04 00 	lea    rcx,[rbx+0x4b010]
   140781ddf:	48 c1 e7 0c          	shl    rdi,0xc
   140781de3:	e8 a8 69 b8 ff       	call   0x140308790
   140781de8:	48 8b c8             	mov    rcx,rax
   140781deb:	ff 15 e7 e2 bb 00    	call   QWORD PTR [rip+0xbbe2e7]        # 0x1413400d8
   140781df1:	41 8b 45 00          	mov    eax,DWORD PTR [r13+0x0]
   140781df5:	39 05 6d 11 9c 01    	cmp    DWORD PTR [rip+0x19c116d],eax        # 0x142142f68
   140781dfb:	0f 8f a4 03 00 00    	jg     0x1407821a5
   140781e01:	48 8d 93 0c d0 02 00 	lea    rdx,[rbx+0x2d00c]
   140781e08:	45 2b e7             	sub    r12d,r15d
   140781e0b:	48 03 d7             	add    rdx,rdi
   140781e0e:	44 89 64 24 20       	mov    DWORD PTR [rsp+0x20],r12d
   140781e13:	4c 8d 0d ba 49 c1 00 	lea    r9,[rip+0xc149ba]        # 0x1413967d4
   140781e1a:	41 b8 00 10 00 00    	mov    r8d,0x1000
   140781e20:	48 8d 0d 3e 11 9c 01 	lea    rcx,[rip+0x19c113e]        # 0x142142f65
   140781e27:	e8 c4 44 a1 ff       	call   0x1401962f0
   140781e2c:	41 b9 01 00 00 00    	mov    r9d,0x1
   140781e32:	4c 8d 83 0c d0 02 00 	lea    r8,[rbx+0x2d00c]
   140781e39:	4c 03 c7             	add    r8,rdi
   140781e3c:	48 8d 8e 30 01 00 00 	lea    rcx,[rsi+0x130]
   140781e43:	41 8d 51 5d          	lea    edx,[r9+0x5d]
   140781e47:	e8 94 e3 1a 00       	call   0x1409301e0
   140781e4c:	e8 4f 51 c7 ff       	call   0x1403f6fa0
   140781e51:	48 8b d8             	mov    rbx,rax
   140781e54:	48 8d 88 10 b0 04 00 	lea    rcx,[rax+0x4b010]
   140781e5b:	e8 30 69 b8 ff       	call   0x140308790
   140781e60:	48 8b c8             	mov    rcx,rax
   140781e63:	ff 15 bf e4 bb 00    	call   QWORD PTR [rip+0xbbe4bf]        # 0x141340328
   140781e69:	ff 43 08             	inc    DWORD PTR [rbx+0x8]
   140781e6c:	83 7b 08 1e          	cmp    DWORD PTR [rbx+0x8],0x1e
   140781e70:	7c 07                	jl     0x140781e79
   140781e72:	c7 43 08 00 00 00 00 	mov    DWORD PTR [rbx+0x8],0x0
   140781e79:	48 63 43 08          	movsxd rax,DWORD PTR [rbx+0x8]
   140781e7d:	48 8d bb 0c d0 02 00 	lea    rdi,[rbx+0x2d00c]
   140781e84:	48 c1 e0 0c          	shl    rax,0xc
   140781e88:	b9 00 10 00 00       	mov    ecx,0x1000
   140781e8d:	48 03 f8             	add    rdi,rax
   140781e90:	33 c0                	xor    eax,eax
   140781e92:	f3 aa                	rep stos BYTE PTR es:[rdi],al
   140781e94:	48 63 7b 08          	movsxd rdi,DWORD PTR [rbx+0x8]
   140781e98:	48 8d 8b 10 b0 04 00 	lea    rcx,[rbx+0x4b010]
   140781e9f:	48 c1 e7 0c          	shl    rdi,0xc
   140781ea3:	e8 e8 68 b8 ff       	call   0x140308790
   140781ea8:	48 8b c8             	mov    rcx,rax
   140781eab:	ff 15 27 e2 bb 00    	call   QWORD PTR [rip+0xbbe227]        # 0x1413400d8
   140781eb1:	41 8b 45 00          	mov    eax,DWORD PTR [r13+0x0]
   140781eb5:	39 05 ad 10 9c 01    	cmp    DWORD PTR [rip+0x19c10ad],eax        # 0x142142f68
   140781ebb:	0f 8f 1a 03 00 00    	jg     0x1407821db
   140781ec1:	4c 8b 8c 24 88 00 00 	mov    r9,QWORD PTR [rsp+0x88]
   140781ec8:	00 
   140781ec9:	48 8d 93 0c d0 02 00 	lea    rdx,[rbx+0x2d00c]
   140781ed0:	48 03 d7             	add    rdx,rdi
   140781ed3:	44 89 7c 24 20       	mov    DWORD PTR [rsp+0x20],r15d
   140781ed8:	41 b8 00 10 00 00    	mov    r8d,0x1000
   140781ede:	48 8d 0d 80 10 9c 01 	lea    rcx,[rip+0x19c1080]        # 0x142142f65
   140781ee5:	e8 06 44 a1 ff       	call   0x1401962f0
   140781eea:	41 b9 01 00 00 00    	mov    r9d,0x1
   140781ef0:	4c 8d 83 0c d0 02 00 	lea    r8,[rbx+0x2d00c]
   140781ef7:	4c 03 c7             	add    r8,rdi
   140781efa:	48 8d 8e 30 01 00 00 	lea    rcx,[rsi+0x130]
   140781f01:	41 8d 51 5e          	lea    edx,[r9+0x5e]
   140781f05:	e8 d6 e2 1a 00       	call   0x1409301e0
   140781f0a:	48 8b 86 90 01 00 00 	mov    rax,QWORD PTR [rsi+0x190]
   140781f11:	48 85 c0             	test   rax,rax
   140781f14:	74 03                	je     0x140781f19
   140781f16:	44 8b 30             	mov    r14d,DWORD PTR [rax]
   140781f19:	48 8b 05 e8 f1 04 02 	mov    rax,QWORD PTR [rip+0x204f1e8]        # 0x1427d1108
   140781f20:	41 8b ce             	mov    ecx,r14d
   140781f23:	48 83 3c c8 00       	cmp    QWORD PTR [rax+rcx*8],0x0
   140781f28:	77 41                	ja     0x140781f6b
   140781f2a:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   140781f31:	45 33 c0             	xor    r8d,r8d
   140781f34:	41 8d 50 05          	lea    edx,[r8+0x5]
   140781f38:	e8 03 84 0d 00       	call   0x14085a340
   140781f3d:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   140781f44:	ba 05 00 00 00       	mov    edx,0x5
   140781f49:	48 8b b9 80 01 00 00 	mov    rdi,QWORD PTR [rcx+0x180]
   140781f50:	e8 8b bb 0c 00       	call   0x14084dae0
   140781f55:	48 8b cf             	mov    rcx,rdi
   140781f58:	48 8b d8             	mov    rbx,rax
   140781f5b:	e8 70 c5 0c 00       	call   0x14084e4d0
   140781f60:	41 b9 4e 5c 00 00    	mov    r9d,0x5c4e
   140781f66:	e9 91 00 00 00       	jmp    0x140781ffc
   140781f6b:	48 8b ce             	mov    rcx,rsi
   140781f6e:	e8 6d 70 f8 ff       	call   0x140708fe0
   140781f73:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   140781f7a:	ba 05 00 00 00       	mov    edx,0x5
   140781f7f:	85 c0                	test   eax,eax
   140781f81:	74 48                	je     0x140781fcb
   140781f83:	83 bc 24 80 00 00 00 	cmp    DWORD PTR [rsp+0x80],0x0
   140781f8a:	00 
   140781f8b:	7f 33                	jg     0x140781fc0
   140781f8d:	45 33 c0             	xor    r8d,r8d
   140781f90:	e8 ab 83 0d 00       	call   0x14085a340
   140781f95:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   140781f9c:	ba 05 00 00 00       	mov    edx,0x5
   140781fa1:	48 8b b9 80 01 00 00 	mov    rdi,QWORD PTR [rcx+0x180]
   140781fa8:	e8 33 bb 0c 00       	call   0x14084dae0
   140781fad:	48 8b cf             	mov    rcx,rdi
   140781fb0:	48 8b d8             	mov    rbx,rax
   140781fb3:	e8 18 c5 0c 00       	call   0x14084e4d0
   140781fb8:	41 b9 50 5c 00 00    	mov    r9d,0x5c50
   140781fbe:	eb 3c                	jmp    0x140781ffc
   140781fc0:	41 b8 01 00 00 00    	mov    r8d,0x1
   140781fc6:	e9 6d ff ff ff       	jmp    0x140781f38
   140781fcb:	45 33 c0             	xor    r8d,r8d
   140781fce:	e8 6d 83 0d 00       	call   0x14085a340
   140781fd3:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   140781fda:	ba 05 00 00 00       	mov    edx,0x5
   140781fdf:	48 8b b9 80 01 00 00 	mov    rdi,QWORD PTR [rcx+0x180]
   140781fe6:	e8 f5 ba 0c 00       	call   0x14084dae0
   140781feb:	48 8b cf             	mov    rcx,rdi
   140781fee:	48 8b d8             	mov    rbx,rax
   140781ff1:	e8 da c4 0c 00       	call   0x14084e4d0
   140781ff6:	41 b9 4f 5c 00 00    	mov    r9d,0x5c4f
   140781ffc:	45 33 c0             	xor    r8d,r8d
   140781fff:	48 8b d3             	mov    rdx,rbx
   140782002:	48 8b c8             	mov    rcx,rax
   140782005:	e8 46 2b 12 00       	call   0x1408a4b50
   14078200a:	e8 c1 c2 de ff       	call   0x14056e2d0
   14078200f:	48 85 c0             	test   rax,rax
   140782012:	0f 84 9d 00 00 00    	je     0x1407820b5
   140782018:	e8 b3 c2 de ff       	call   0x14056e2d0
   14078201d:	83 38 00             	cmp    DWORD PTR [rax],0x0
   140782020:	0f 84 8f 00 00 00    	je     0x1407820b5
   140782026:	48 8b 8e c8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1c8]
   14078202d:	ba 05 00 00 00       	mov    edx,0x5
   140782032:	e8 a9 ba 0c 00       	call   0x14084dae0
   140782037:	f6 40 40 04          	test   BYTE PTR [rax+0x40],0x4
   14078203b:	74 1b                	je     0x140782058
   14078203d:	48 8b 8e e8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1e8]
   140782044:	33 d2                	xor    edx,edx
   140782046:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   140782049:	ff 90 d0 00 00 00    	call   QWORD PTR [rax+0xd0]
   14078204f:	48 8b 8e f0 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1f0]
   140782056:	eb 52                	jmp    0x1407820aa
   140782058:	8b 86 a8 01 00 00    	mov    eax,DWORD PTR [rsi+0x1a8]
   14078205e:	85 c0                	test   eax,eax
   140782060:	75 3c                	jne    0x14078209e
   140782062:	48 8b 86 f8 01 00 00 	mov    rax,QWORD PTR [rsi+0x1f8]
   140782069:	48 8b 88 b8 01 00 00 	mov    rcx,QWORD PTR [rax+0x1b8]
   140782070:	f7 41 40 00 01 00 00 	test   DWORD PTR [rcx+0x40],0x100
   140782077:	74 d6                	je     0x14078204f
   140782079:	48 8b 86 00 02 00 00 	mov    rax,QWORD PTR [rsi+0x200]
   140782080:	48 8b 88 d0 01 00 00 	mov    rcx,QWORD PTR [rax+0x1d0]
   140782087:	f7 41 40 00 01 00 00 	test   DWORD PTR [rcx+0x40],0x100
   14078208e:	74 bf                	je     0x14078204f
   140782090:	48 8b 8e f0 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1f0]
   140782097:	ba 01 00 00 00       	mov    edx,0x1
   14078209c:	eb 0e                	jmp    0x1407820ac
   14078209e:	83 f8 01             	cmp    eax,0x1
   1407820a1:	75 12                	jne    0x1407820b5
   1407820a3:	48 8b 8e e8 01 00 00 	mov    rcx,QWORD PTR [rsi+0x1e8]
   1407820aa:	33 d2                	xor    edx,edx
   1407820ac:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1407820af:	ff 90 d0 00 00 00    	call   QWORD PTR [rax+0xd0]
   1407820b5:	48 8b 9c 24 90 00 00 	mov    rbx,QWORD PTR [rsp+0x90]
   1407820bc:	00 
   1407820bd:	48 83 c4 40          	add    rsp,0x40
   1407820c1:	41 5f                	pop    r15
   1407820c3:	41 5e                	pop    r14
   1407820c5:	41 5d                	pop    r13
   1407820c7:	41 5c                	pop    r12
   1407820c9:	5f                   	pop    rdi
   1407820ca:	5e                   	pop    rsi
   1407820cb:	5d                   	pop    rbp
   1407820cc:	c3                   	ret
   1407820cd:	48 8d 0d 94 0e 9c 01 	lea    rcx,[rip+0x19c0e94]        # 0x142142f68
   1407820d4:	e8 b3 70 87 00       	call   0x140ff918c
   1407820d9:	83 3d 88 0e 9c 01 ff 	cmp    DWORD PTR [rip+0x19c0e88],0xffffffff        # 0x142142f68
   1407820e0:	0f 85 6c f9 ff ff    	jne    0x140781a52
   1407820e6:	48 8d 0d e3 9e bb 00 	lea    rcx,[rip+0xbb9ee3]        # 0x14133bfd0
   1407820ed:	e8 aa 6e 87 00       	call   0x140ff8f9c
   1407820f2:	48 8d 0d 6f 0e 9c 01 	lea    rcx,[rip+0x19c0e6f]        # 0x142142f68
   1407820f9:	e8 2e 70 87 00       	call   0x140ff912c
   1407820fe:	e9 4f f9 ff ff       	jmp    0x140781a52
   140782103:	48 8d 0d 5e 0e 9c 01 	lea    rcx,[rip+0x19c0e5e]        # 0x142142f68
   14078210a:	e8 7d 70 87 00       	call   0x140ff918c
   14078210f:	83 3d 52 0e 9c 01 ff 	cmp    DWORD PTR [rip+0x19c0e52],0xffffffff        # 0x142142f68
   140782116:	0f 85 4b fa ff ff    	jne    0x140781b67
   14078211c:	48 8d 0d ad 9e bb 00 	lea    rcx,[rip+0xbb9ead]        # 0x14133bfd0
   140782123:	e8 74 6e 87 00       	call   0x140ff8f9c
   140782128:	48 8d 0d 39 0e 9c 01 	lea    rcx,[rip+0x19c0e39]        # 0x142142f68
   14078212f:	e8 f8 6f 87 00       	call   0x140ff912c
   140782134:	e9 2e fa ff ff       	jmp    0x140781b67
   140782139:	48 8d 0d 28 0e 9c 01 	lea    rcx,[rip+0x19c0e28]        # 0x142142f68
   140782140:	e8 47 70 87 00       	call   0x140ff918c
   140782145:	83 3d 1c 0e 9c 01 ff 	cmp    DWORD PTR [rip+0x19c0e1c],0xffffffff        # 0x142142f68
   14078214c:	0f 85 7a fa ff ff    	jne    0x140781bcc
   140782152:	48 8d 0d 77 9e bb 00 	lea    rcx,[rip+0xbb9e77]        # 0x14133bfd0
   140782159:	e8 3e 6e 87 00       	call   0x140ff8f9c
   14078215e:	48 8d 0d 03 0e 9c 01 	lea    rcx,[rip+0x19c0e03]        # 0x142142f68
   140782165:	e8 c2 6f 87 00       	call   0x140ff912c
   14078216a:	e9 5d fa ff ff       	jmp    0x140781bcc
   14078216f:	48 8d 0d f2 0d 9c 01 	lea    rcx,[rip+0x19c0df2]        # 0x142142f68
   140782176:	e8 11 70 87 00       	call   0x140ff918c
   14078217b:	83 3d e6 0d 9c 01 ff 	cmp    DWORD PTR [rip+0x19c0de6],0xffffffff        # 0x142142f68
   140782182:	0f 85 bc fb ff ff    	jne    0x140781d44
   140782188:	48 8d 0d 41 9e bb 00 	lea    rcx,[rip+0xbb9e41]        # 0x14133bfd0
   14078218f:	e8 08 6e 87 00       	call   0x140ff8f9c
   140782194:	48 8d 0d cd 0d 9c 01 	lea    rcx,[rip+0x19c0dcd]        # 0x142142f68
   14078219b:	e8 8c 6f 87 00       	call   0x140ff912c
   1407821a0:	e9 9f fb ff ff       	jmp    0x140781d44
   1407821a5:	48 8d 0d bc 0d 9c 01 	lea    rcx,[rip+0x19c0dbc]        # 0x142142f68
   1407821ac:	e8 db 6f 87 00       	call   0x140ff918c
   1407821b1:	83 3d b0 0d 9c 01 ff 	cmp    DWORD PTR [rip+0x19c0db0],0xffffffff        # 0x142142f68
   1407821b8:	0f 85 43 fc ff ff    	jne    0x140781e01
   1407821be:	48 8d 0d 0b 9e bb 00 	lea    rcx,[rip+0xbb9e0b]        # 0x14133bfd0
   1407821c5:	e8 d2 6d 87 00       	call   0x140ff8f9c
   1407821ca:	48 8d 0d 97 0d 9c 01 	lea    rcx,[rip+0x19c0d97]        # 0x142142f68
   1407821d1:	e8 56 6f 87 00       	call   0x140ff912c
   1407821d6:	e9 26 fc ff ff       	jmp    0x140781e01
   1407821db:	48 8d 0d 86 0d 9c 01 	lea    rcx,[rip+0x19c0d86]        # 0x142142f68
   1407821e2:	e8 a5 6f 87 00       	call   0x140ff918c
   1407821e7:	83 3d 7a 0d 9c 01 ff 	cmp    DWORD PTR [rip+0x19c0d7a],0xffffffff        # 0x142142f68
   1407821ee:	0f 85 cd fc ff ff    	jne    0x140781ec1
   1407821f4:	48 8d 0d d5 9d bb 00 	lea    rcx,[rip+0xbb9dd5]        # 0x14133bfd0
   1407821fb:	e8 9c 6d 87 00       	call   0x140ff8f9c
   140782200:	48 8d 0d 61 0d 9c 01 	lea    rcx,[rip+0x19c0d61]        # 0x142142f68
   140782207:	e8 20 6f 87 00       	call   0x140ff912c
   14078220c:	e9 b0 fc ff ff       	jmp    0x140781ec1
