__int64 __fastcall sub_2D0714(
        __int64 a1,
        __int64 a2,
        int a3,
        __int64 a4,
        int a5,
        int a6,
        double a7,
        double a8,
        double a9,
        double a10,
        double a11,
        double a12,
        __m128 _XMM6,
        __m128 _XMM7,
        __int64 a15,
        __int64 a16,
        __int64 a17,
        __int64 a18)
{
  __int64 v18; // rbx
  __int64 result; // rax
  __int64 v22; // rcx
  int v27; // ecx
  int v28; // edi
  int v29; // r8d
  int v30; // r9d
  __int64 v31; // rax
  int v32; // r15d
  __int64 v33; // rax
  __int64 v34; // rdi
  __int64 v35; // rsi
  int v36; // r9d
  int v50; // ecx
  __int64 v51; // r8
  __int64 v52; // r9
  int v53; // r10d
  int v54; // r11d
  __int64 **v55; // rax
  __int64 v56; // rcx
  __int64 v57; // rax
  int v58; // ecx
  int v59; // edi
  int v60; // r8d
  int v61; // r9d
  int v62; // edi
  int v63; // esi
  int v64; // r8d
  int v65; // r9d
  int v66; // ecx
  int v67; // r8d
  int v68; // r9d
  unsigned __int64 v69; // rdi
  __int64 v70; // rdi
  __int64 v71; // rax
  __int64 v72; // rax
  __int64 v79; // rax
  __int64 v80; // rdx
  __int64 v81; // rdi
  __int64 v82; // rsi
  int v83; // r8d
  int v84; // r9d
  __int64 v85; // r10
  int v86; // eax
  int v87; // ecx
  __int64 v88; // rdx
  __int64 v89; // rdi
  __int64 v90; // rsi
  __int64 v93; // rdi
  __int64 v94; // rsi
  __int64 v95; // rdi
  __int64 v96; // rsi
  __int64 **v97; // rdx
  __int64 **v98; // rdi
  __int64 v99; // rsi
  __int64 **v102; // rcx
  __int64 **v103; // rax
  __int64 **v104; // r8
  __int64 v109; // rax
  __int64 v110; // rdx
  __int64 v111; // rdi
  __int64 v112; // rsi
  __int64 v113; // r8
  __int64 v114; // r10
  int v120; // eax
  __int64 v121; // rax
  __int64 v122; // rdi
  __int64 v123; // rsi
  __int64 v124; // rax
  __int64 v125; // rdx
  __int64 v126; // rdi
  __int64 v127; // rsi
  __int64 v128; // r14
  int v130; // edx
  int v131; // ecx
  __int64 v132; // rsi
  int v133; // r8d
  int v134; // r9d
  __int64 *v135; // rbx
  __int64 v136; // rdi
  __int64 v140; // rcx
  __int64 v141; // [rsp+0h] [rbp-100h]
  __int64 v142; // [rsp+0h] [rbp-100h]
  __int64 v143; // [rsp+8h] [rbp-F8h]
  __int64 *v144; // [rsp+20h] [rbp-E0h] BYREF
  __int64 v145; // [rsp+28h] [rbp-D8h]
  int v146; // [rsp+30h] [rbp-D0h] BYREF
  __m256 v147; // [rsp+40h] [rbp-C0h] BYREF
  int v148; // [rsp+60h] [rbp-A0h]
  __m256 *v152; // [rsp+70h] [rbp-90h]
  __int64 v154; // [rsp+7Ch] [rbp-84h]
  __int64 v155; // [rsp+88h] [rbp-78h]
  __int64 v156; // [rsp+90h] [rbp-70h]
  __int64 v157; // [rsp+98h] [rbp-68h]
  int v158; // [rsp+A0h] [rbp-60h]
  __int64 **v159; // [rsp+A8h] [rbp-58h]
  __int64 **v160; // [rsp+B0h] [rbp-50h]
  __int64 **v161; // [rsp+B8h] [rbp-48h]
  _QWORD v162[2]; // [rsp+D0h] [rbp-30h] BYREF
  int v163; // [rsp+E0h] [rbp-20h]
  __m256 v164; // [rsp+F0h] [rbp-10h] BYREF
  int v184; // [rsp+15Ch] [rbp+5Ch]
  int v187; // [rsp+168h] [rbp+68h]
  __int64 v190; // [rsp+180h] [rbp+80h] BYREF
  int v191; // [rsp+188h] [rbp+88h]
  __int64 v197; // [rsp+1BCh] [rbp+BCh]
  __int64 v198; // [rsp+1C8h] [rbp+C8h]
  __int64 v204; // [rsp+200h] [rbp+100h]
  __int64 v205; // [rsp+208h] [rbp+108h]
  __m256 v206; // [rsp+210h] [rbp+110h] BYREF
  int v207; // [rsp+230h] [rbp+130h]
  _BYTE v211[112]; // [rsp+240h] [rbp+140h] BYREF
  char v212; // [rsp+2B0h] [rbp+1B0h] BYREF
  _UNKNOWN *retaddr; // [rsp+2D8h] [rbp+1D8h] BYREF

  result = (__int64)&retaddr;
  a16 = v18;
  a17 = a2;
  a18 = a1;
  __asm
  {
    vmovaps xmmword ptr [rax-38h], xmm6
    vmovaps xmmword ptr [rax-48h], xmm7
    vmovaps xmmword ptr [rax-58h], xmm8
  }
  v22 = *(_QWORD *)(a4 + 544);
  if ( v22 == 0 )
    goto LABEL_9;
  result = MEMORY[0x1C1F890];
  if ( *(_QWORD *)(MEMORY[0x1C1F890] + 468LL) == 0 )
    goto LABEL_9;
  if ( *(_QWORD *)(a4 + 380) != 0 )
    goto LABEL_9;
  result = *(_QWORD *)(a4 + 120);
  if ( result != 0 && (*(_BYTE *)(result + 172) & 1) != 0 && (*(_DWORD *)(a4 + 336) & 0x80000) == 0 )
    goto LABEL_9;
  if ( *(_DWORD *)(v22 + 324) != 0 )
  {
    result = sub_2D0EB0(a1, a2, a3, a4, a5, a6);
    goto LABEL_9;
  }
  if ( a3 == 0 )
    goto LABEL_9;
  sub_F1170(a1, a2, v211, a4, &v146);
  v31 = *(_QWORD *)(a4 + 120);
  if ( v31 == 0 || (v32 = 1, (*(_BYTE *)(v31 + 172) & 1) == 0) )
    v32 = 0;
  v33 = sub_2D0F74(v28, 0, 0, v27, v29, v30);
  *(_QWORD *)(a4 + 380) = v33;
  *(_QWORD *)(v33 + 136) = 0;
  *(_QWORD *)(*(_QWORD *)(a4 + 380) + 96LL) = a4;
  *(_DWORD *)(*(_QWORD *)(a4 + 380) + 132LL) = *(_DWORD *)(*(_QWORD *)(MEMORY[0x1C1F890] + 468LL) + 96LL);
  _RDX = *(_QWORD *)(a4 + 544);
  if ( *(int *)(_RDX + 308) > 0 )
  {
    _R8 = 0;
    while ( v35 == 0 )
    {
      _RAX = *(_QWORD *)(_RDX + 300);
      __asm
      {
        vmovss  xmm0, dword ptr [rax+r8]
        vsubss  xmm1, xmm0, [rsp+2D0h+var_2A0]
        vmovss  dword ptr [rsp+2D0h+var_2B0], xmm1
        vmovss  xmm2, dword ptr [rax+r8+4]
        vsubss  xmm0, xmm2, [rsp+2D0h+var_29C]
        vmovss  dword ptr [rsp+2D0h+var_2B0+4], xmm0
        vmovss  xmm1, dword ptr [rax+r8+8]
        vsubss  xmm2, xmm1, [rsp+2D0h+var_298]
        vmovss  dword ptr [rsp+2D0h+var_2A8], xmm2
        vmovss  xmm1, dword ptr cs:17060ACh
      }
      if ( (unsigned int)sub_2C4B5C(v34, 0, _RDX, &v144, *(double *)&_XMM0, *(double *)&_XMM1) != 0 )
      {
        v35 = *(_QWORD *)(v52 + *(_QWORD *)(_RDX + 284));
        if ( v35 == 0 )
        {
          result = sub_B2E1F4(v34, 0, *(_QWORD *)(a4 + 380), v50, v51, v52);
          *(_QWORD *)(a4 + 380) = 0;
          goto LABEL_9;
        }
      }
      _R8 = v51 + 12;
      v36 = v52 + 8;
      if ( v53 + 1 >= v54 )
      {
        if ( v35 != 0 )
          goto LABEL_26;
        goto LABEL_20;
      }
    }
    goto LABEL_26;
  }
LABEL_20:
  v55 = nullptr;
  v56 = *(_QWORD *)(a4 + 120);
  if ( v56 != 0 )
    v55 = (__int64 **)sub_45D298(v34, v35, _RDX, *(_QWORD *)(v56 + 64), (unsigned int)&v146, v36, v141, v143);
  v144 = nullptr;
  v145 = 0;
  if ( v55 == nullptr )
  {
    sub_CF1FD8(v34, v35, *(_QWORD *)(a4 + 544), &v144, &v146);
    v55 = &v144;
  }
  v162[0] = 20215408;
  v163 = 0;
  v162[1] = v55;
  v57 = (*(__int64 (__fastcall **)(__int64, __int64, _QWORD *))(*MEMORY[0x1C20F38] + 64LL))(v34, v35, v162);
  if ( v57 == 0 )
  {
    sub_B2E1F4(v59, 0, *(_QWORD *)(a4 + 380), v58, v60, v61);
    *(_QWORD *)(a4 + 380) = 0;
    result = 21990688;
    v162[0] = 21990688;
    LODWORD(v145) = 0;
    v135 = v144;
    if ( v144 == nullptr )
      goto LABEL_9;
    v136 = MEMORY[0x1B9BC10];
    if ( MEMORY[0x1B9BC10] == 0 )
    {
      sub_411A40(0, v132, v130, v131, v133, v134, v141);
      v136 = MEMORY[0x1B9BC10];
    }
    _R8 = *(_QWORD *)(*(_QWORD *)v136 + 24LL);
    if ( (__int64 (__fastcall *)(int, int, int, int, int, int, __int64))_R8 != sub_DC3F0 )
    {
      result = ((__int64 (__fastcall *)(__int64, __int64, __int64 *, __int64))_R8)(v136, v132, v135, v136);
      goto LABEL_9;
    }
    if ( (_WORD)v135 != 0 && MEMORY[0x1AD0394] != -1 )
    {
      _RDX = MEMORY[0x131A378]();
      if ( _RDX != 0 && *((_BYTE *)&loc_2 + ((unsigned __int64)v135 & 0xFFFFFFFFFFFF0000LL) + 1) == 0xE3 )
      {
        _R8 = 3LL * *((unsigned __int8 *)&loc_2 + ((unsigned __int64)v135 & 0xFFFFFFFFFFFF0000LL));
        if ( *(_DWORD *)(_RDX + 24LL * *((unsigned __int8 *)&loc_2 + ((unsigned __int64)v135 & 0xFFFFFFFFFFFF0000LL)) + 8) < 0x40u
          && *(_DWORD *)(_RDX + 24LL * *((unsigned __int8 *)&loc_2 + ((unsigned __int64)v135 & 0xFFFFFFFFFFFF0000LL)) + 8)
           * (unsigned int)*(unsigned __int16 *)((unsigned __int64)v135 & 0xFFFFFFFFFFFF0000LL) < (unsigned int)sub_10000 )
        {
LABEL_77:
          result = *(_QWORD *)(_RDX + 8 * _R8);
          *v135 = result;
          v135[1] = 0;
          *(_QWORD *)(_RDX + 8 * _R8) = v135;
          ++*(_DWORD *)(_RDX + 8 * _R8 + 8);
          goto LABEL_9;
        }
        if ( *(_QWORD *)(_RDX
                       + 24LL * *((unsigned __int8 *)&loc_2 + ((unsigned __int64)v135 & 0xFFFFFFFFFFFF0000LL))
                       + 12) == 0 )
        {
          __asm
          {
            vmovsd  xmm0, qword ptr [rdx+r8*8]
            vmovsd  qword ptr [rdx+r8*8+0Ch], xmm0
          }
          *(_DWORD *)(_RDX + 8 * _R8 + 20) = *(_DWORD *)(_RDX + 8 * _R8 + 8);
          *(_QWORD *)(_RDX + 8 * _R8) = 0;
          *(_DWORD *)(_RDX + 8 * _R8 + 8) = 0;
          goto LABEL_77;
        }
      }
    }
    result = sub_DBC6C(v136, v132, (_DWORD)v135, v136, _R8, v134, v141, v143);
    goto LABEL_9;
  }
  *(_DWORD *)(v57 + 8) = 2;
  ++MEMORY[0x1C20F40];
  a15 = v57;
  sub_C9360(v59, v57, (unsigned int)&a15, *(_QWORD *)(a4 + 544) + 284, v60, v61, v141);
  sub_19D00C(v62, v63, (unsigned int)&v146, *(_QWORD *)(a4 + 544) + 300, v64, v65, v142);
  v162[0] = 21990688;
  LODWORD(v145) = 0;
  v69 = (unsigned __int64)v144;
  if ( v144 != nullptr )
  {
    v128 = MEMORY[0x1B9BC10];
    if ( MEMORY[0x1B9BC10] == 0 )
    {
      sub_411A40((_DWORD)v144, v35, _RDX, v66, v67, v68, v141);
      v128 = MEMORY[0x1B9BC10];
    }
    _R8 = *(_QWORD *)(*(_QWORD *)v128 + 24LL);
    if ( (__int64 (__fastcall *)(int, int, int, int, int, int, __int64))_R8 != sub_DC3F0 )
    {
      ((void (__fastcall *)(unsigned __int64, __int64, unsigned __int64, __int64))_R8)(v69, v35, v69, v128);
      goto LABEL_26;
    }
    if ( (_WORD)v69 != 0 && MEMORY[0x1AD0394] != -1 )
    {
      _RDX = MEMORY[0x131A378]();
      if ( _RDX != 0 && *((_BYTE *)&loc_2 + (v69 & 0xFFFFFFFFFFFF0000LL) + 1) == 0xE3 )
      {
        _R8 = 3LL * *((unsigned __int8 *)&loc_2 + (v69 & 0xFFFFFFFFFFFF0000LL));
        if ( *(_DWORD *)(_RDX + 24LL * *((unsigned __int8 *)&loc_2 + (v69 & 0xFFFFFFFFFFFF0000LL)) + 8) < 0x40u
          && *(_DWORD *)(_RDX + 24LL * *((unsigned __int8 *)&loc_2 + (v69 & 0xFFFFFFFFFFFF0000LL)) + 8)
           * (unsigned int)*(unsigned __int16 *)(v69 & 0xFFFFFFFFFFFF0000LL) < (unsigned int)sub_10000 )
        {
LABEL_64:
          *(_QWORD *)v69 = *(_QWORD *)(_RDX + 8 * _R8);
          *(_QWORD *)(v69 + 8) = 0;
          *(_QWORD *)(_RDX + 8 * _R8) = v69;
          ++*(_DWORD *)(_RDX + 8 * _R8 + 8);
          goto LABEL_26;
        }
        if ( *(_QWORD *)(_RDX + 24LL * *((unsigned __int8 *)&loc_2 + (v69 & 0xFFFFFFFFFFFF0000LL)) + 12) == 0 )
        {
          __asm
          {
            vmovsd  xmm0, qword ptr [rdx+r8*8]
            vmovsd  qword ptr [rdx+r8*8+0Ch], xmm0
          }
          *(_DWORD *)(_RDX + 8 * _R8 + 20) = *(_DWORD *)(_RDX + 8 * _R8 + 8);
          *(_QWORD *)(_RDX + 8 * _R8) = 0;
          *(_DWORD *)(_RDX + 8 * _R8 + 8) = 0;
          goto LABEL_64;
        }
      }
    }
    sub_DBC6C(v69, v35, v69, v128, _R8, v68, v141, v143);
  }
LABEL_26:
  v70 = *(_QWORD *)(MEMORY[0x1C1F350] + 768LL);
  if ( *(_QWORD *)(a4 + 372) != 0 )
  {
    v70 = *(_QWORD *)(a4 + 372);
  }
  else
  {
    v71 = *(_QWORD *)(*(_QWORD *)(a4 + 544) + 248LL);
    if ( v71 != 0 )
    {
      v72 = *(_QWORD *)(v71 + 192);
      if ( v72 != 0 )
        v70 = v72;
    }
  }
  v191 = 6;
  v190 = 20215560;
  __asm
  {
    vmovss  xmm8, dword ptr cs:17061B8h
    vmovss  [rbp+1D0h+var_144], xmm8
    vmovaps ymm0, ymmword ptr cs:1706420h
    vmovups [rbp+1D0h+var_140], ymm0
    vxorps  xmm6, xmm6, xmm6
    vmovss  [rbp+1D0h+var_118], xmm6
    vmovss  [rbp+1D0h+var_11C], xmm6
    vmovss  [rbp+1D0h+var_120], xmm6
  }
  v197 = 8;
  v198 = 0;
  __asm
  {
    vmovss  xmm7, dword ptr cs:1706190h
    vmovss  [rbp+1D0h+var_F8], xmm7
    vmovss  [rbp+1D0h+var_100], xmm8
    vmovss  [rbp+1D0h+var_FC], xmm7
    vpxor   xmm0, xmm0, xmm0
    vmovdqu [rbp+1D0h+var_F0], xmm0
    vpxor   xmm1, xmm1, xmm1
    vmovups [rbp+1D0h+var_E0], xmm1
  }
  v205 = 0;
  v204 = v35;
  __asm { vzeroupper }
  v79 = sub_2D1CD8(v70, v35, _RDX, *(_QWORD *)(MEMORY[0x1C1F890] + 468LL));
  if ( v79 != 0 )
  {
    v86 = (*(__int64 (__fastcall **)(__int64, __int64, __int64, __int64))(*(_QWORD *)v79 + 8LL))(v81, v82, v80, v79);
    v87 = HIDWORD(v205);
    if ( v86 != -65536 )
      v87 = 2;
    HIDWORD(v205) = v87;
    v85 = MEMORY[0x1C1F890];
  }
  LODWORD(v205) = 0;
  HIWORD(v197) = sub_2D184C(v81, v82, v81, *(_QWORD *)(v85 + 468), v83, v84, v141, v143);
  LOBYTE(v88) = *(_BYTE *)(a4 + 340);
  _RAX = sub_10A78C(v89, v90, v88, &v144, a4 + 348);
  __asm
  {
    vmovups xmm0, xmmword ptr [rax]
    vmovdqu [rbp+1D0h+var_E0], xmm0
  }
  sub_F1610(v93, v94, v211, &v206);
  sub_576E65(v95, v96, 0, &v147, 136);
  v152 = nullptr;
  __asm
  {
    vmovss  [rsp+2D0h+var_258], xmm6
    vmovaps ymm0, ymmword ptr cs:1706860h
    vmovups [rsp+2D0h+var_290], ymm0
    vmovss  [rsp+2D0h+var_270], xmm8
    vmovss  [rsp+2D0h+var_264], xmm6
    vmovss  [rsp+2D0h+var_268], xmm6
    vmovss  [rsp+2D0h+var_26C], xmm6
  }
  v154 = 0;
  v155 = 0;
  v156 = 0;
  v157 = 0;
  v102 = v159;
  v160 = v159;
  v158 = 1;
  if ( v161 <= v159 && (v159 == nullptr || (unsigned int)(v161 - v159) < 2) )
  {
    __asm { vzeroupper }
    v103 = (__int64 **)MEMORY[0x131BE18](v98, v99, v97, 16);
    v98 = v103;
    v104 = v160;
    v97 = v159;
    if ( v159 != v160 )
    {
      do
        *v103++ = *v97++;
      while ( v97 != v104 );
      v97 = v159;
    }
    if ( v97 != nullptr )
    {
      MEMORY[0x131BE10](v98, v99, v97, v97, v104);
      v97 = v159;
    }
    v161 = v98 + 2;
    v102 = &v98[(unsigned int)(v160 - v97)];
    v160 = v102;
    v159 = v98;
  }
  *v102 = &v190;
  ++v160;
  __asm
  {
    vmovups ymm0, [rbp+1D0h+var_C0]
    vmovups [rsp+2D0h+var_290], ymm0
  }
  v148 = v207;
  __asm
  {
    vmovss  xmm0, [rbp+1D0h+var_9C]
    vmovss  [rsp+2D0h+var_26C], xmm0
    vmovss  xmm1, [rbp+1D0h+var_98]
    vmovss  [rsp+2D0h+var_268], xmm1
    vmovss  xmm0, [rbp+1D0h+var_94]
    vmovss  [rsp+2D0h+var_264], xmm0
    vmovss  [rsp+2D0h+var_258], xmm8
  }
  __asm { vzeroupper }
  v109 = sub_2D1CD8(v98, v99, v97, *(_QWORD *)(MEMORY[0x1C1F890] + 468LL));
  if ( (*(_BYTE *)(a4 + 352) & 0x40) != 0 && v32 == 0 )
  {
    v140 = v157;
    if ( v109 != 0 )
      v140 = v109;
    v157 = v140;
  }
  __asm
  {
    vmovaps ymm0, ymmword ptr cs:1706860h
    vmovups [rbp+1D0h+var_1E0], ymm0
    vmovss  [rbp+1D0h+var_1C0], xmm8
    vmovss  [rbp+1D0h+var_1B4], xmm6
    vmovss  [rbp+1D0h+var_1B8], xmm6
    vmovss  [rbp+1D0h+var_1BC], xmm6
    vmovss  [rbp+1D0h+var_1A8], xmm6
    vmovss  [rbp+1D0h+var_1AC], xmm6
    vmovss  [rbp+1D0h+var_1B0], xmm6
    vmovss  [rbp+1D0h+var_198], xmm6
    vmovss  [rbp+1D0h+var_19C], xmm6
    vmovss  [rbp+1D0h+var_1A0], xmm6
    vmovss  [rbp+1D0h+var_18C], xmm6
    vmovss  [rbp+1D0h+var_190], xmm6
    vmovss  [rbp+1D0h+var_194], xmm6
    vmovss  xmm0, dword ptr cs:17064ECh
    vmovss  [rbp+1D0h+var_188], xmm0
    vmovss  [rbp+1D0h+var_1A4], xmm6
    vmovss  [rbp+1D0h+var_184], xmm6
    vmovss  xmm0, dword ptr cs:17061C4h
    vmovss  [rbp+1D0h+var_180], xmm0
    vmovss  [rbp+1D0h+var_17C], xmm7
    vmovss  [rbp+1D0h+var_170], xmm7
    vmovss  [rbp+1D0h+var_16C], xmm7
    vmovss  [rbp+1D0h+var_178], xmm6
  }
  v187 = 4;
  v184 = 2304;
  __asm
  {
    vmovss  [rbp+1D0h+var_164], xmm7
    vmovss  [rbp+1D0h+var_160], xmm6
  }
  if ( v32 == 0 )
  {
    v184 = 2432;
    v152 = &v164;
  }
  v120 = v154;
  if ( (*(_DWORD *)(a4 + 336) & 0x80000) == 0 )
    v120 = 1;
  LODWORD(v154) = v120;
  __asm { vzeroupper }
  v121 = sub_2AB6C8(v111, v112, v110, *(_QWORD *)(v114 + 468), v113);
  v124 = (*(__int64 (__fastcall **)(__int64, __int64, __m256 *, __int64))(*(_QWORD *)v121 + 56LL))(
           v122,
           v123,
           &v147,
           v121);
  if ( v124 != 0 )
  {
    *(_QWORD *)(*(_QWORD *)(a4 + 380) + 136LL) = v124;
    *(_QWORD *)(v124 + 8) = *(_QWORD *)(a4 + 380);
  }
  result = sub_2CFE88(v126, v127, v125, &v147);
LABEL_9:
  _R11 = &v212;
  __asm
  {
    vmovaps xmm6, xmmword ptr [r11-10h]
    vmovaps xmm7, xmmword ptr [r11-20h]
    vmovaps xmm8, xmmword ptr [r11-30h]
  }
  return result;
}
