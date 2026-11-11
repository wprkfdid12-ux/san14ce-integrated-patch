
game_static_only.bin:     file format pei-x86-64


Disassembly of section .text:

00000001408a2c50 <.text+0x8a1c50>:
   1408a2c50:	48 89 6c 24 18       	mov    QWORD PTR [rsp+0x18],rbp
   1408a2c55:	48 89 74 24 20       	mov    QWORD PTR [rsp+0x20],rsi
   1408a2c5a:	57                   	push   rdi
   1408a2c5b:	41 56                	push   r14
   1408a2c5d:	41 57                	push   r15
   1408a2c5f:	48 83 ec 70          	sub    rsp,0x70
   1408a2c63:	48 8b 05 26 8f 1e 01 	mov    rax,QWORD PTR [rip+0x11e8f26]        # 0x141a8bb90
   1408a2c6a:	48 33 c4             	xor    rax,rsp
   1408a2c6d:	48 89 44 24 60       	mov    QWORD PTR [rsp+0x60],rax
   1408a2c72:	48 8b e9             	mov    rbp,rcx
   1408a2c75:	48 8b 89 78 01 00 00 	mov    rcx,QWORD PTR [rcx+0x178]
   1408a2c7c:	e8 3f b8 a8 ff       	call   0x14032e4c0
   1408a2c81:	85 c0                	test   eax,eax
   1408a2c83:	74 11                	je     0x1408a2c96
   1408a2c85:	48 8b 85 78 01 00 00 	mov    rax,QWORD PTR [rbp+0x178]
   1408a2c8c:	44 0f b7 88 a6 00 00 	movzx  r9d,WORD PTR [rax+0xa6]
   1408a2c93:	00 
   1408a2c94:	eb 06                	jmp    0x1408a2c9c
   1408a2c96:	41 b9 96 08 00 00    	mov    r9d,0x896
   1408a2c9c:	33 f6                	xor    esi,esi
   1408a2c9e:	c7 44 24 38 01 00 00 	mov    DWORD PTR [rsp+0x38],0x1
   1408a2ca5:	00 
   1408a2ca6:	89 74 24 30          	mov    DWORD PTR [rsp+0x30],esi
   1408a2caa:	48 8d bd 30 01 00 00 	lea    rdi,[rbp+0x130]
   1408a2cb1:	c7 44 24 28 01 00 00 	mov    DWORD PTR [rsp+0x28],0x1
   1408a2cb8:	00 
   1408a2cb9:	48 8b cf             	mov    rcx,rdi
   1408a2cbc:	48 89 6c 24 20       	mov    QWORD PTR [rsp+0x20],rbp
   1408a2cc1:	8d 56 02             	lea    edx,[rsi+0x2]
   1408a2cc4:	44 8d 46 0a          	lea    r8d,[rsi+0xa]
   1408a2cc8:	e8 a3 cf 08 00       	call   0x14092fc70
   1408a2ccd:	48 8b 8d 78 01 00 00 	mov    rcx,QWORD PTR [rbp+0x178]
   1408a2cd4:	44 8b fe             	mov    r15d,esi
   1408a2cd7:	89 74 24 44          	mov    DWORD PTR [rsp+0x44],esi
   1408a2cdb:	e8 e0 b7 a8 ff       	call   0x14032e4c0
   1408a2ce0:	85 c0                	test   eax,eax
   1408a2ce2:	74 11                	je     0x1408a2cf5
   1408a2ce4:	48 8b 8d 78 01 00 00 	mov    rcx,QWORD PTR [rbp+0x178]
   1408a2ceb:	e8 70 92 98 ff       	call   0x14022bf60
   1408a2cf0:	4c 8b f0             	mov    r14,rax
   1408a2cf3:	eb 03                	jmp    0x1408a2cf8
   1408a2cf5:	4c 8b f6             	mov    r14,rsi
   1408a2cf8:	49 8b ce             	mov    rcx,r14
   1408a2cfb:	e8 c0 b7 a8 ff       	call   0x14032e4c0
   1408a2d00:	85 c0                	test   eax,eax
   1408a2d02:	74 19                	je     0x1408a2d1d
   1408a2d04:	48 8d 54 24 44       	lea    rdx,[rsp+0x44]
   1408a2d09:	49 8b ce             	mov    rcx,r14
   1408a2d0c:	e8 9f 94 a0 ff       	call   0x1402ac1b0
   1408a2d11:	49 8b 06             	mov    rax,QWORD PTR [r14]
   1408a2d14:	49 8b ce             	mov    rcx,r14
   1408a2d17:	ff 50 60             	call   QWORD PTR [rax+0x60]
   1408a2d1a:	44 8b f8             	mov    r15d,eax
   1408a2d1d:	ba 04 00 00 00       	mov    edx,0x4
   1408a2d22:	48 89 9c 24 98 00 00 	mov    QWORD PTR [rsp+0x98],rbx
   1408a2d29:	00 
   1408a2d2a:	48 8b cf             	mov    rcx,rdi
   1408a2d2d:	e8 5e 96 08 00       	call   0x14092c390
   1408a2d32:	41 b9 01 00 00 00    	mov    r9d,0x1
   1408a2d38:	4c 8d 44 24 44       	lea    r8,[rsp+0x44]
   1408a2d3d:	48 8d 88 30 01 00 00 	lea    rcx,[rax+0x130]
   1408a2d44:	41 8d 51 01          	lea    edx,[r9+0x1]
   1408a2d48:	e8 93 d4 08 00       	call   0x1409301e0
   1408a2d4d:	49 8b ce             	mov    rcx,r14
   1408a2d50:	e8 6b b7 a8 ff       	call   0x14032e4c0
   1408a2d55:	85 c0                	test   eax,eax
   1408a2d57:	74 2e                	je     0x1408a2d87
   1408a2d59:	ba 04 00 00 00       	mov    edx,0x4
   1408a2d5e:	48 8b cf             	mov    rcx,rdi
   1408a2d61:	e8 2a 96 08 00       	call   0x14092c390
   1408a2d66:	33 d2                	xor    edx,edx
   1408a2d68:	49 8b ce             	mov    rcx,r14
   1408a2d6b:	48 8b d8             	mov    rbx,rax
   1408a2d6e:	e8 1d 99 a0 ff       	call   0x1402ac690
   1408a2d73:	44 8b c0             	mov    r8d,eax
   1408a2d76:	48 8d 8b 30 01 00 00 	lea    rcx,[rbx+0x130]
   1408a2d7d:	ba 02 00 00 00       	mov    edx,0x2
   1408a2d82:	e8 79 b5 08 00       	call   0x14092e300
   1408a2d87:	ba 04 00 00 00       	mov    edx,0x4
   1408a2d8c:	48 8b cf             	mov    rcx,rdi
   1408a2d8f:	e8 fc 95 08 00       	call   0x14092c390
   1408a2d94:	45 8b c7             	mov    r8d,r15d
   1408a2d97:	ba 01 00 00 00       	mov    edx,0x1
   1408a2d9c:	48 8d 88 30 01 00 00 	lea    rcx,[rax+0x130]
   1408a2da3:	e8 d8 c2 08 00       	call   0x14092f080
   1408a2da8:	48 8b 8d 78 01 00 00 	mov    rcx,QWORD PTR [rbp+0x178]
   1408a2daf:	ba 02 00 00 00       	mov    edx,0x2
   1408a2db4:	e8 77 8c ff ff       	call   0x14089ba30
   1408a2db9:	44 8b c0             	mov    r8d,eax
   1408a2dbc:	ba 0c 00 00 00       	mov    edx,0xc
   1408a2dc1:	48 8b cf             	mov    rcx,rdi
   1408a2dc4:	e8 07 b6 08 00       	call   0x14092e3d0
   1408a2dc9:	48 8b 8d 78 01 00 00 	mov    rcx,QWORD PTR [rbp+0x178]
   1408a2dd0:	ba 03 00 00 00       	mov    edx,0x3
   1408a2dd5:	e8 56 8c ff ff       	call   0x14089ba30
   1408a2dda:	44 8b c0             	mov    r8d,eax
   1408a2ddd:	ba 0e 00 00 00       	mov    edx,0xe
   1408a2de2:	48 8b cf             	mov    rcx,rdi
   1408a2de5:	e8 e6 b5 08 00       	call   0x14092e3d0
   1408a2dea:	48 8b 8d 78 01 00 00 	mov    rcx,QWORD PTR [rbp+0x178]
   1408a2df1:	ba 04 00 00 00       	mov    edx,0x4
   1408a2df6:	e8 35 8c ff ff       	call   0x14089ba30
   1408a2dfb:	44 8b c0             	mov    r8d,eax
   1408a2dfe:	ba 10 00 00 00       	mov    edx,0x10
   1408a2e03:	48 8b cf             	mov    rcx,rdi
   1408a2e06:	e8 c5 b5 08 00       	call   0x14092e3d0
   1408a2e0b:	48 8b 8d 78 01 00 00 	mov    rcx,QWORD PTR [rbp+0x178]
   1408a2e12:	ba 05 00 00 00       	mov    edx,0x5
   1408a2e17:	e8 14 8c ff ff       	call   0x14089ba30
   1408a2e1c:	44 8b c0             	mov    r8d,eax
   1408a2e1f:	ba 12 00 00 00       	mov    edx,0x12
   1408a2e24:	48 8b cf             	mov    rcx,rdi
   1408a2e27:	e8 a4 b5 08 00       	call   0x14092e3d0
   1408a2e2c:	48 8b 8d 78 01 00 00 	mov    rcx,QWORD PTR [rbp+0x178]
   1408a2e33:	ba 06 00 00 00       	mov    edx,0x6
   1408a2e38:	e8 f3 8b ff ff       	call   0x14089ba30
   1408a2e3d:	44 8b c0             	mov    r8d,eax
   1408a2e40:	ba 14 00 00 00       	mov    edx,0x14
   1408a2e45:	48 8b cf             	mov    rcx,rdi
   1408a2e48:	e8 83 b5 08 00       	call   0x14092e3d0
   1408a2e4d:	48 8b 8d 78 01 00 00 	mov    rcx,QWORD PTR [rbp+0x178]
   1408a2e54:	48 85 c9             	test   rcx,rcx
   1408a2e57:	0f 84 bf 03 00 00    	je     0x1408a321c
   1408a2e5d:	e8 8e b5 98 ff       	call   0x14022e3f0
   1408a2e62:	ba 0a 00 00 00       	mov    edx,0xa
   1408a2e67:	4c 8b c0             	mov    r8,rax
   1408a2e6a:	48 8b cf             	mov    rcx,rdi
   1408a2e6d:	44 8d 4a f7          	lea    r9d,[rdx-0x9]
   1408a2e71:	e8 6a d3 08 00       	call   0x1409301e0
   1408a2e76:	48 8b 8d 78 01 00 00 	mov    rcx,QWORD PTR [rbp+0x178]
   1408a2e7d:	e8 6e a6 94 ff       	call   0x1401ed4f0
   1408a2e82:	8b d8                	mov    ebx,eax
   1408a2e84:	85 c0                	test   eax,eax
   1408a2e86:	0f 8e 8e 00 00 00    	jle    0x1408a2f1a
   1408a2e8c:	e8 0f 41 b5 ff       	call   0x1403f6fa0
   1408a2e91:	48 8b c8             	mov    rcx,rax
   1408a2e94:	e8 e7 f2 92 ff       	call   0x1401d2180
   1408a2e99:	ba 1b 00 00 00       	mov    edx,0x1b
   1408a2e9e:	48 8b cf             	mov    rcx,rdi
   1408a2ea1:	4c 8b f0             	mov    r14,rax
   1408a2ea4:	44 8d 42 e6          	lea    r8d,[rdx-0x1a]
   1408a2ea8:	e8 23 be 08 00       	call   0x14092ecd0
   1408a2ead:	48 8b 8d 78 01 00 00 	mov    rcx,QWORD PTR [rbp+0x178]
   1408a2eb4:	0f b6 91 4c 01 00 00 	movzx  edx,BYTE PTR [rcx+0x14c]
   1408a2ebb:	83 fa 06             	cmp    edx,0x6
   1408a2ebe:	77 11                	ja     0x1408a2ed1
   1408a2ec0:	48 8b 0d 51 b0 ed 01 	mov    rcx,QWORD PTR [rip+0x1edb051]        # 0x14277df18
   1408a2ec7:	4c 8b 84 d1 f8 8c 07 	mov    r8,QWORD PTR [rcx+rdx*8+0x78cf8]
   1408a2ece:	00 
   1408a2ecf:	eb 0e                	jmp    0x1408a2edf
   1408a2ed1:	48 8b 05 40 b0 ed 01 	mov    rax,QWORD PTR [rip+0x1edb040]        # 0x14277df18
   1408a2ed8:	4c 8b 80 f8 8c 07 00 	mov    r8,QWORD PTR [rax+0x78cf8]
   1408a2edf:	ba 1c 00 00 00       	mov    edx,0x1c
   1408a2ee4:	49 83 c0 10          	add    r8,0x10
   1408a2ee8:	48 8b cf             	mov    rcx,rdi
   1408a2eeb:	44 8d 4a e5          	lea    r9d,[rdx-0x1b]
   1408a2eef:	e8 ec d2 08 00       	call   0x1409301e0
   1408a2ef4:	e8 87 26 8f ff       	call   0x140195580
   1408a2ef9:	48 8b c8             	mov    rcx,rax
   1408a2efc:	89 5c 24 20          	mov    DWORD PTR [rsp+0x20],ebx
   1408a2f00:	4c 8d 0d d9 40 b3 00 	lea    r9,[rip+0xb340d9]        # 0x1413d6fe0
   1408a2f07:	41 b8 00 10 00 00    	mov    r8d,0x1000
   1408a2f0d:	49 8b d6             	mov    rdx,r14
   1408a2f10:	e8 db 33 8f ff       	call   0x1401962f0
   1408a2f15:	4d 8b c6             	mov    r8,r14
   1408a2f18:	eb 2f                	jmp    0x1408a2f49
   1408a2f1a:	45 33 c0             	xor    r8d,r8d
   1408a2f1d:	48 8b cf             	mov    rcx,rdi
   1408a2f20:	41 8d 50 1b          	lea    edx,[r8+0x1b]
   1408a2f24:	e8 a7 bd 08 00       	call   0x14092ecd0
   1408a2f29:	4c 8b 05 68 4c 1a 01 	mov    r8,QWORD PTR [rip+0x11a4c68]        # 0x141a47b98
   1408a2f30:	41 b9 01 00 00 00    	mov    r9d,0x1
   1408a2f36:	48 8b cf             	mov    rcx,rdi
   1408a2f39:	41 8d 51 1b          	lea    edx,[r9+0x1b]
   1408a2f3d:	e8 9e d2 08 00       	call   0x1409301e0
   1408a2f42:	4c 8b 05 4f 4c 1a 01 	mov    r8,QWORD PTR [rip+0x11a4c4f]        # 0x141a47b98
   1408a2f49:	41 b9 01 00 00 00    	mov    r9d,0x1
   1408a2f4f:	48 8b cf             	mov    rcx,rdi
   1408a2f52:	41 8d 51 1c          	lea    edx,[r9+0x1c]
   1408a2f56:	e8 85 d2 08 00       	call   0x1409301e0
   1408a2f5b:	48 8b 5d 08          	mov    rbx,QWORD PTR [rbp+0x8]
   1408a2f5f:	4c 8d 44 24 48       	lea    r8,[rsp+0x48]
   1408a2f64:	48 8b 95 78 01 00 00 	mov    rdx,QWORD PTR [rbp+0x178]
   1408a2f6b:	48 8b cb             	mov    rcx,rbx
   1408a2f6e:	e8 7d 61 e3 ff       	call   0x1406d90f0
   1408a2f73:	48 8b 85 78 01 00 00 	mov    rax,QWORD PTR [rbp+0x178]
   1408a2f7a:	48 8d 54 24 40       	lea    rdx,[rsp+0x40]
   1408a2f7f:	48 8b cb             	mov    rcx,rbx
   1408a2f82:	44 0f b7 40 10       	movzx  r8d,WORD PTR [rax+0x10]
   1408a2f87:	e8 34 8e e3 ff       	call   0x1406dbdc0
   1408a2f8c:	48 8b 8d 78 01 00 00 	mov    rcx,QWORD PTR [rbp+0x178]
   1408a2f93:	8b de                	mov    ebx,esi
   1408a2f95:	e8 46 cb 79 ff       	call   0x14003fae0
   1408a2f9a:	48 63 c8             	movsxd rcx,eax
   1408a2f9d:	85 c0                	test   eax,eax
   1408a2f9f:	7e 3b                	jle    0x1408a2fdc
   1408a2fa1:	4c 8b 0d 70 af ed 01 	mov    r9,QWORD PTR [rip+0x1edaf70]        # 0x14277df18
   1408a2fa8:	49 8d 91 b8 5d 08 00 	lea    rdx,[r9+0x85db8]
   1408a2faf:	48 8d 14 ca          	lea    rdx,[rdx+rcx*8]
   1408a2fb3:	48 83 f9 06          	cmp    rcx,0x6
   1408a2fb7:	7f 05                	jg     0x1408a2fbe
   1408a2fb9:	4c 8b 02             	mov    r8,QWORD PTR [rdx]
   1408a2fbc:	eb 07                	jmp    0x1408a2fc5
   1408a2fbe:	4d 8b 81 b8 5d 08 00 	mov    r8,QWORD PTR [r9+0x85db8]
   1408a2fc5:	41 39 70 30          	cmp    DWORD PTR [r8+0x30],esi
   1408a2fc9:	8b c6                	mov    eax,esi
   1408a2fcb:	0f 95 c0             	setne  al
   1408a2fce:	48 ff c9             	dec    rcx
   1408a2fd1:	03 d8                	add    ebx,eax
   1408a2fd3:	48 83 ea 08          	sub    rdx,0x8
   1408a2fd7:	48 85 c9             	test   rcx,rcx
   1408a2fda:	7f d7                	jg     0x1408a2fb3
   1408a2fdc:	48 8b 85 b0 01 00 00 	mov    rax,QWORD PTR [rbp+0x1b0]
   1408a2fe3:	4c 8d 74 24 48       	lea    r14,[rsp+0x48]
   1408a2fe8:	89 58 20             	mov    DWORD PTR [rax+0x20],ebx
   1408a2feb:	48 8d 9d b8 01 00 00 	lea    rbx,[rbp+0x1b8]
   1408a2ff2:	41 8b 0e             	mov    ecx,DWORD PTR [r14]
   1408a2ff5:	85 c9                	test   ecx,ecx
   1408a2ff7:	0f 84 82 01 00 00    	je     0x1408a317f
   1408a2ffd:	83 e9 01             	sub    ecx,0x1
   1408a3000:	0f 84 be 00 00 00    	je     0x1408a30c4
   1408a3006:	83 e9 01             	sub    ecx,0x1
   1408a3009:	74 56                	je     0x1408a3061
   1408a300b:	83 f9 01             	cmp    ecx,0x1
   1408a300e:	0f 85 f0 01 00 00    	jne    0x1408a3204
   1408a3014:	48 8b 4b 60          	mov    rcx,QWORD PTR [rbx+0x60]
   1408a3018:	ba 01 00 00 00       	mov    edx,0x1
   1408a301d:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a3020:	ff 90 c8 00 00 00    	call   QWORD PTR [rax+0xc8]
   1408a3026:	48 8b 4b 30          	mov    rcx,QWORD PTR [rbx+0x30]
   1408a302a:	ba 01 00 00 00       	mov    edx,0x1
   1408a302f:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a3032:	ff 90 c8 00 00 00    	call   QWORD PTR [rax+0xc8]
   1408a3038:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
   1408a303b:	ba 01 00 00 00       	mov    edx,0x1
   1408a3040:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a3043:	ff 90 c8 00 00 00    	call   QWORD PTR [rax+0xc8]
   1408a3049:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
   1408a304c:	48 8b 15 45 4b 1a 01 	mov    rdx,QWORD PTR [rip+0x11a4b45]        # 0x141a47b98
   1408a3053:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a3056:	ff 90 d8 01 00 00    	call   QWORD PTR [rax+0x1d8]
   1408a305c:	e9 67 01 00 00       	jmp    0x1408a31c8
   1408a3061:	48 8b 4b 60          	mov    rcx,QWORD PTR [rbx+0x60]
   1408a3065:	33 d2                	xor    edx,edx
   1408a3067:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a306a:	ff 90 c8 00 00 00    	call   QWORD PTR [rax+0xc8]
   1408a3070:	48 8b 4b 30          	mov    rcx,QWORD PTR [rbx+0x30]
   1408a3074:	33 d2                	xor    edx,edx
   1408a3076:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a3079:	ff 90 c8 00 00 00    	call   QWORD PTR [rax+0xc8]
   1408a307f:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
   1408a3082:	33 d2                	xor    edx,edx
   1408a3084:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a3087:	ff 90 c8 00 00 00    	call   QWORD PTR [rax+0xc8]
   1408a308d:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
   1408a3090:	33 d2                	xor    edx,edx
   1408a3092:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a3095:	ff 90 d0 00 00 00    	call   QWORD PTR [rax+0xd0]
   1408a309b:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
   1408a309e:	33 d2                	xor    edx,edx
   1408a30a0:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a30a3:	ff 90 08 02 00 00    	call   QWORD PTR [rax+0x208]
   1408a30a9:	48 8b 8d b0 01 00 00 	mov    rcx,QWORD PTR [rbp+0x1b0]
   1408a30b0:	45 33 c0             	xor    r8d,r8d
   1408a30b3:	8b d6                	mov    edx,esi
   1408a30b5:	e8 f6 14 f4 ff       	call   0x1407e45b0
   1408a30ba:	ba 01 00 00 00       	mov    edx,0x1
   1408a30bf:	e9 33 01 00 00       	jmp    0x1408a31f7
   1408a30c4:	48 8b 4b 60          	mov    rcx,QWORD PTR [rbx+0x60]
   1408a30c8:	33 d2                	xor    edx,edx
   1408a30ca:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a30cd:	ff 90 c8 00 00 00    	call   QWORD PTR [rax+0xc8]
   1408a30d3:	48 8b 4b 30          	mov    rcx,QWORD PTR [rbx+0x30]
   1408a30d7:	33 d2                	xor    edx,edx
   1408a30d9:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a30dc:	ff 90 c8 00 00 00    	call   QWORD PTR [rax+0xc8]
   1408a30e2:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
   1408a30e5:	ba 01 00 00 00       	mov    edx,0x1
   1408a30ea:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a30ed:	ff 90 c8 00 00 00    	call   QWORD PTR [rax+0xc8]
   1408a30f3:	48 8b 03             	mov    rax,QWORD PTR [rbx]
   1408a30f6:	8d 4e 01             	lea    ecx,[rsi+0x1]
   1408a30f9:	48 8b 38             	mov    rdi,QWORD PTR [rax]
   1408a30fc:	e8 3f 24 ab ff       	call   0x140355540
   1408a3101:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
   1408a3104:	48 8b d0             	mov    rdx,rax
   1408a3107:	ff 97 d8 01 00 00    	call   QWORD PTR [rdi+0x1d8]
   1408a310d:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
   1408a3110:	ba 01 00 00 00       	mov    edx,0x1
   1408a3115:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a3118:	ff 90 d0 00 00 00    	call   QWORD PTR [rax+0xd0]
   1408a311e:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
   1408a3121:	33 d2                	xor    edx,edx
   1408a3123:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a3126:	ff 90 08 02 00 00    	call   QWORD PTR [rax+0x208]
   1408a312c:	48 8b 4b c8          	mov    rcx,QWORD PTR [rbx-0x38]
   1408a3130:	33 d2                	xor    edx,edx
   1408a3132:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a3135:	ff 90 c8 00 00 00    	call   QWORD PTR [rax+0xc8]
   1408a313b:	83 fe 03             	cmp    esi,0x3
   1408a313e:	7d 09                	jge    0x1408a3149
   1408a3140:	0f b6 44 24 40       	movzx  eax,BYTE PTR [rsp+0x40]
   1408a3145:	3b f0                	cmp    esi,eax
   1408a3147:	eb 0a                	jmp    0x1408a3153
   1408a3149:	0f b6 44 24 41       	movzx  eax,BYTE PTR [rsp+0x41]
   1408a314e:	8d 4e fd             	lea    ecx,[rsi-0x3]
   1408a3151:	3b c8                	cmp    ecx,eax
   1408a3153:	7d 11                	jge    0x1408a3166
   1408a3155:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
   1408a3158:	ba 01 00 00 00       	mov    edx,0x1
   1408a315d:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a3160:	ff 90 08 02 00 00    	call   QWORD PTR [rax+0x208]
   1408a3166:	48 8b 8d b0 01 00 00 	mov    rcx,QWORD PTR [rbp+0x1b0]
   1408a316d:	41 b8 01 00 00 00    	mov    r8d,0x1
   1408a3173:	8b d6                	mov    edx,esi
   1408a3175:	e8 36 14 f4 ff       	call   0x1407e45b0
   1408a317a:	e9 85 00 00 00       	jmp    0x1408a3204
   1408a317f:	48 8b 4b 60          	mov    rcx,QWORD PTR [rbx+0x60]
   1408a3183:	33 d2                	xor    edx,edx
   1408a3185:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a3188:	ff 90 c8 00 00 00    	call   QWORD PTR [rax+0xc8]
   1408a318e:	48 8b 4b 30          	mov    rcx,QWORD PTR [rbx+0x30]
   1408a3192:	33 d2                	xor    edx,edx
   1408a3194:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a3197:	ff 90 c8 00 00 00    	call   QWORD PTR [rax+0xc8]
   1408a319d:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
   1408a31a0:	ba 01 00 00 00       	mov    edx,0x1
   1408a31a5:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a31a8:	ff 90 c8 00 00 00    	call   QWORD PTR [rax+0xc8]
   1408a31ae:	48 8b 03             	mov    rax,QWORD PTR [rbx]
   1408a31b1:	8d 4e 01             	lea    ecx,[rsi+0x1]
   1408a31b4:	48 8b 38             	mov    rdi,QWORD PTR [rax]
   1408a31b7:	e8 84 23 ab ff       	call   0x140355540
   1408a31bc:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
   1408a31bf:	48 8b d0             	mov    rdx,rax
   1408a31c2:	ff 97 d8 01 00 00    	call   QWORD PTR [rdi+0x1d8]
   1408a31c8:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
   1408a31cb:	33 d2                	xor    edx,edx
   1408a31cd:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a31d0:	ff 90 d0 00 00 00    	call   QWORD PTR [rax+0xd0]
   1408a31d6:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
   1408a31d9:	33 d2                	xor    edx,edx
   1408a31db:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a31de:	ff 90 08 02 00 00    	call   QWORD PTR [rax+0x208]
   1408a31e4:	48 8b 8d b0 01 00 00 	mov    rcx,QWORD PTR [rbp+0x1b0]
   1408a31eb:	45 33 c0             	xor    r8d,r8d
   1408a31ee:	8b d6                	mov    edx,esi
   1408a31f0:	e8 bb 13 f4 ff       	call   0x1407e45b0
   1408a31f5:	33 d2                	xor    edx,edx
   1408a31f7:	48 8b 4b c8          	mov    rcx,QWORD PTR [rbx-0x38]
   1408a31fb:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   1408a31fe:	ff 90 c8 00 00 00    	call   QWORD PTR [rax+0xc8]
   1408a3204:	ff c6                	inc    esi
   1408a3206:	48 83 c3 08          	add    rbx,0x8
   1408a320a:	49 83 c6 04          	add    r14,0x4
   1408a320e:	83 fe 06             	cmp    esi,0x6
   1408a3211:	0f 8c db fd ff ff    	jl     0x1408a2ff2
   1408a3217:	e9 f7 00 00 00       	jmp    0x1408a3313
   1408a321c:	4c 8b 05 75 49 1a 01 	mov    r8,QWORD PTR [rip+0x11a4975]        # 0x141a47b98
   1408a3223:	41 b9 01 00 00 00    	mov    r9d,0x1
   1408a3229:	48 8b cf             	mov    rcx,rdi
   1408a322c:	41 8d 51 09          	lea    edx,[r9+0x9]
   1408a3230:	e8 ab cf 08 00       	call   0x1409301e0
   1408a3235:	45 33 c0             	xor    r8d,r8d
   1408a3238:	48 8b cf             	mov    rcx,rdi
   1408a323b:	41 8d 50 1b          	lea    edx,[r8+0x1b]
   1408a323f:	e8 8c ba 08 00       	call   0x14092ecd0
   1408a3244:	4c 8b 05 4d 49 1a 01 	mov    r8,QWORD PTR [rip+0x11a494d]        # 0x141a47b98
   1408a324b:	41                   	rex.B
   1408a324c:	b9                   	.byte 0xb9
   1408a324d:	01 00                	add    DWORD PTR [rax],eax
	...
