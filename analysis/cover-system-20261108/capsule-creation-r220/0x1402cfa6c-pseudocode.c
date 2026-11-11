__int64 __fastcall sub_2CEA6C(
        _DWORD a1,
        _DWORD a2,
        __int64 a3,
        __int64 a4,
        _QWORD *a5,
        int a6,
        double a7,
        double a8,
        double a9,
        double a10,
        double a11,
        double a12,
        __m128 _XMM6,
        __m128 _XMM7)
{
  _QWORD *v21; // r13
  __int64 v27; // rax
  __int64 v28; // rdx
  __int64 v29; // rdi
  __int64 v30; // rsi
  __int64 v31; // rbx
  __int64 *j; // rdx
  unsigned __int64 v33; // rcx
  __int64 *v35; // r8
  _QWORD *v36; // r9
  int v37; // r13d
  int v39; // r12d
  int v40; // r13d
  unsigned __int64 v41; // rsi
  int v42; // r15d
  int v43; // r12d
  __int64 v44; // rsi
  __int64 v45; // rax
  __int64 v46; // r14
  __int64 v47; // rdx
  __int64 v48; // rax
  unsigned __int64 v49; // rdi
  __int64 k; // r14
  __int64 v51; // rdi
  unsigned int v62; // eax
  __int64 v63; // r13
  __int64 *v64; // rax
  __int64 *v65; // r14
  __int64 v66; // rcx
  __int64 v67; // rdi
  int v94; // edi
  int v95; // esi
  int v96; // r8d
  int v97; // r9d
  __int64 v98; // rax
  unsigned int v105; // eax
  _QWORD *v106; // r13
  __int64 v108; // rsi
  int v112; // edi
  int v113; // esi
  int v114; // r8d
  int v115; // r9d
  __int64 v116; // rax
  __int64 v117; // rsi
  unsigned int v130; // eax
  __int64 v131; // r15
  __int64 v132; // rsi
  __int64 *v133; // rax
  __int64 v134; // rcx
  __int64 v135; // r12
  __int64 v136; // r15
  __int64 *v137; // rax
  __int64 v138; // rcx
  _QWORD *v139; // r12
  __int64 v140; // rsi
  int v144; // edi
  int v145; // esi
  int v146; // r8d
  int v147; // r9d
  __int64 v148; // rax
  __int64 v149; // rsi
  unsigned int v155; // eax
  __int64 v156; // r15
  __int64 v157; // rsi
  __int64 *v158; // rax
  __int64 *i; // rdx
  __int64 v160; // [rsp+0h] [rbp-100h]
  __int64 v161; // [rsp+8h] [rbp-F8h]
  __int64 v162; // [rsp+10h] [rbp-F0h]
  _QWORD *v163; // [rsp+20h] [rbp-E0h] BYREF
  __int64 v164; // [rsp+28h] [rbp-D8h]
  __int64 v165; // [rsp+30h] [rbp-D0h] BYREF
  __int64 v166; // [rsp+38h] [rbp-C8h]
  int v167; // [rsp+40h] [rbp-C0h]
  __m256 v168; // [rsp+48h] [rbp-B8h] BYREF
  int v169; // [rsp+68h] [rbp-98h]
  __m256 v170; // [rsp+80h] [rbp-80h] BYREF
  char v172; // [rsp+140h] [rbp+40h] BYREF
  _UNKNOWN *retaddr; // [rsp+178h] [rbp+78h] BYREF
  _QWORD *v178; // [rsp+190h] [rbp+90h]
  int v179; // [rsp+198h] [rbp+98h]

  _RAX = &retaddr;
  v178 = a5;
  __asm
  {
    vmovaps xmmword ptr [rax-48h], xmm6
    vmovaps xmmword ptr [rax-58h], xmm7
    vmovaps xmmword ptr [rax-68h], xmm8
    vmovaps xmmword ptr [rax-78h], xmm9
    vmovaps xmmword ptr [rax-88h], xmm10
    vmovaps xmmword ptr [rax-98h], xmm11
    vmovaps xmmword ptr [rax-0A8h], xmm12
    vmovaps xmmword ptr [rax-0B8h], xmm13
  }
  v21 = a5;
  __asm
  {
    vmovss  xmm2, dword ptr cs:17060C4h
    vmulss  xmm11, xmm2, dword ptr [rdx+8]
    vmulss  xmm12, xmm2, dword ptr [rdx+4]
    vmulss  xmm10, xmm2, dword ptr [rdx]
    vmovss  dword ptr [rsp+170h+var_140], xmm10
    vmovss  dword ptr [rsp+170h+var_140+4], xmm12
    vmovss  dword ptr [rsp+170h+var_138], xmm11
  }
  v27 = sub_1BF3C8(a3, 136, 8, 136, (_DWORD)a5, a6);
  v31 = v27;
  if ( v27 != 0 )
  {
    *(_DWORD *)v27 = 1065353216;
    *(_QWORD *)(v27 + 104) = 0;
    *(_QWORD *)(v27 + 112) = 0;
    *(_QWORD *)(v27 + 120) = 0;
    sub_576E65(v29, v30, 0, v27, (unsigned int)v30);
    *(_QWORD *)(v31 + 48) = 0;
    *(_QWORD *)(v31 + 56) = 0;
    *(_QWORD *)v31 = 1065353216;
    *(_QWORD *)(v31 + 8) = 0;
    *(_QWORD *)(v31 + 16) = 1065353216;
    *(_QWORD *)(v31 + 24) = 0;
    *(_QWORD *)(v31 + 32) = 1065353216;
    *(_QWORD *)(v31 + 40) = 0;
    *(_QWORD *)(v31 + 72) = 0;
    *(_QWORD *)(v31 + 80) = 0;
    *(_DWORD *)(v31 + 64) = 0;
    *(_QWORD *)(v31 + 88) = 0;
    *(_QWORD *)(v31 + 112) = *(_QWORD *)(v31 + 104);
    *(_DWORD *)(v31 + 96) = 1;
  }
  else
  {
    v31 = 0;
  }
  v179 = sub_2CF600(v29, v30, v28, &v165);
  if ( v179 != 0 )
  {
    v37 = (int)v36;
    __asm { vmovss  xmm13, dword ptr cs:170678Ch }
    if ( *(_DWORD *)(a4 + 8) > (int)v36 )
    {
      v139 = v36;
      do
      {
        v140 = (__int64)v139 + *(_QWORD *)a4;
        if ( (*(_BYTE *)(v140 + 68) & 1) == 0 )
        {
          __asm { vzeroupper }
          sub_2CE478(_RDI, v140, (char *)v139 + *(_QWORD *)a4, &v168);
          __asm
          {
            vmulss  xmm6, xmm10, [rsp+170h+var_104]
            vmulss  xmm7, xmm12, [rsp+170h+var_100]
            vmulss  xmm8, xmm11, [rsp+170h+var_FC]
          }
          v148 = sub_1BF3C8(v144, v145, 8, 136, v146, v147);
          _RDI = v148;
          v36 = nullptr;
          if ( v148 != 0 )
          {
            *(_DWORD *)(v148 + 8) = 1;
            *(_QWORD *)v148 = 20213184;
            *(_QWORD *)(v148 + 12) = 1065353216;
            *(_QWORD *)(v148 + 20) = 0;
            *(_QWORD *)(v148 + 28) = 1065353216;
            *(_QWORD *)(v148 + 36) = 0;
            *(_QWORD *)(v148 + 44) = 1065353216;
            *(_QWORD *)(v148 + 52) = 0;
            *(_QWORD *)(v148 + 60) = 8;
            *(_QWORD *)(v148 + 72) = 0;
            *(_DWORD *)(v148 + 88) = -1082130432;
            *(_DWORD *)(v148 + 80) = 1065353216;
            *(_DWORD *)(v148 + 84) = -1082130432;
            *(_QWORD *)(v148 + 96) = 0;
            *(_QWORD *)(v148 + 104) = 0;
            *(_QWORD *)(v148 + 112) = 0;
            *(_QWORD *)(v148 + 120) = 0;
            *(_DWORD *)(v148 + 128) = 1065353216;
          }
          else
          {
            _RDI = 0;
          }
          __asm
          {
            vmulss  xmm0, xmm10, dword ptr [rsi+40h]
            vaddss  xmm1, xmm0, xmm13
            vmovss  dword ptr [rdi+80h], xmm1
            vmovups ymm0, [rsp+170h+var_128]
            vmovups ymmword ptr [rdi+0Ch], ymm0
          }
          *(_DWORD *)(_RDI + 44) = v169;
          __asm
          {
            vmovss  dword ptr [rdi+30h], xmm6
            vmovss  dword ptr [rdi+34h], xmm7
            vmovss  dword ptr [rdi+38h], xmm8
          }
          j = *(__int64 **)(v31 + 112);
          v33 = *(_QWORD *)(v31 + 120);
          if ( v33 <= (unsigned __int64)j )
          {
            v35 = *(__int64 **)(v31 + 104);
            v155 = 2 * (j - v35) + 2;
            if ( v35 != nullptr )
              v33 = (__int64)(v33 - (_QWORD)v35) >> 3;
            else
              LODWORD(v33) = 0;
            if ( (unsigned int)v33 < v155 )
            {
              v156 = 8LL * v155;
              __asm { vzeroupper }
              v157 = MEMORY[0x131BE18](_RDI, v149, j, v156);
              v35 = (__int64 *)v157;
              v158 = *(__int64 **)(v31 + 112);
              for ( i = *(__int64 **)(v31 + 104); i != v158; ++i )
                *v35++ = *i;
              v33 = *(_QWORD *)(v31 + 104);
              v36 = nullptr;
              if ( v33 != 0 )
              {
                MEMORY[0x131BE10](_RDI, v157, i, v33, v35);
                v36 = nullptr;
              }
              *(_QWORD *)(v31 + 120) = v156 + v157;
              j = (__int64 *)(v157
                            + 8LL * (unsigned int)((__int64)(*(_QWORD *)(v31 + 112) - *(_QWORD *)(v31 + 104)) >> 3));
              *(_QWORD *)(v31 + 112) = j;
              *(_QWORD *)(v31 + 104) = v157;
            }
          }
          *j = _RDI;
          *(_QWORD *)(v31 + 112) += 8LL;
        }
        ++v37;
        v139 += 10;
      }
      while ( v37 < *(_DWORD *)(a4 + 8) );
    }
    v39 = (int)v36;
    if ( *(_DWORD *)(a4 + 24) > (int)v36 )
    {
      v106 = v36;
      __asm { vmovss  xmm9, dword ptr cs:17060E4h }
      do
      {
        v108 = (__int64)v106 + *(_QWORD *)(a4 + 16);
        if ( (*(_BYTE *)(v108 + 76) & 1) == 0 )
        {
          __asm { vzeroupper }
          sub_2CE478(_RDI, v108, (char *)v106 + *(_QWORD *)(a4 + 16), &v168);
          __asm
          {
            vmulss  xmm6, xmm10, [rsp+170h+var_104]
            vmulss  xmm7, xmm12, [rsp+170h+var_100]
            vmulss  xmm8, xmm11, [rsp+170h+var_FC]
          }
          v116 = sub_1BF3C8(v112, v113, 8, 144, v114, v115);
          _RDI = v116;
          v36 = nullptr;
          if ( v116 != 0 )
          {
            *(_DWORD *)(v116 + 8) = 2;
            *(_QWORD *)v116 = 20215536;
            *(_QWORD *)(v116 + 12) = 1065353216;
            *(_QWORD *)(v116 + 20) = 0;
            *(_QWORD *)(v116 + 28) = 1065353216;
            *(_QWORD *)(v116 + 36) = 0;
            *(_QWORD *)(v116 + 44) = 1065353216;
            *(_QWORD *)(v116 + 52) = 0;
            *(_QWORD *)(v116 + 60) = 8;
            *(_QWORD *)(v116 + 72) = 0;
            *(_DWORD *)(v116 + 88) = -1082130432;
            *(_DWORD *)(v116 + 80) = 1065353216;
            *(_DWORD *)(v116 + 84) = -1082130432;
            *(_QWORD *)(v116 + 96) = 0;
            *(_QWORD *)(v116 + 104) = 0;
            *(_QWORD *)(v116 + 112) = 0;
            *(_QWORD *)(v116 + 120) = 0;
            *(_QWORD *)(v116 + 132) = 0;
            *(_DWORD *)(v116 + 128) = 0;
          }
          else
          {
            _RDI = 0;
          }
          __asm
          {
            vmulss  xmm0, xmm10, dword ptr [rsi+44h]
            vmulss  xmm1, xmm0, xmm9
            vaddss  xmm4, xmm1, xmm13
            vmulss  xmm0, xmm10, dword ptr [rsi+48h]
            vmulss  xmm1, xmm0, xmm9
            vaddss  xmm3, xmm1, xmm13
            vmulss  xmm0, xmm10, dword ptr [rsi+40h]
            vmulss  xmm1, xmm0, xmm9
            vaddss  xmm2, xmm1, xmm13
            vmovss  dword ptr [rdi+80h], xmm2
            vmovss  dword ptr [rdi+84h], xmm4
            vmovss  dword ptr [rdi+88h], xmm3
            vmovups ymm0, [rsp+170h+var_128]
            vmovups ymmword ptr [rdi+0Ch], ymm0
          }
          *(_DWORD *)(_RDI + 44) = v169;
          __asm
          {
            vmovss  dword ptr [rdi+30h], xmm6
            vmovss  dword ptr [rdi+34h], xmm7
            vmovss  dword ptr [rdi+38h], xmm8
          }
          v33 = *(_QWORD *)(v31 + 112);
          j = *(__int64 **)(v31 + 120);
          if ( (unsigned __int64)j <= v33 )
          {
            v35 = *(__int64 **)(v31 + 104);
            v130 = 2 * ((__int64)(v33 - (_QWORD)v35) >> 3) + 2;
            j = v35 != nullptr ? (__int64 *)(j - v35) : nullptr;
            if ( (unsigned int)j < v130 )
            {
              v131 = 8LL * v130;
              __asm { vzeroupper }
              v132 = MEMORY[0x131BE18](_RDI, v117, j, v131);
              v35 = (__int64 *)v132;
              v133 = *(__int64 **)(v31 + 112);
              for ( j = *(__int64 **)(v31 + 104); j != v133; ++j )
                *v35++ = *j;
              v134 = *(_QWORD *)(v31 + 104);
              v36 = nullptr;
              if ( v134 != 0 )
              {
                MEMORY[0x131BE10](_RDI, v132, j, v134, v35);
                v36 = nullptr;
              }
              *(_QWORD *)(v31 + 120) = v132 + v131;
              v33 = v132 + 8LL * (unsigned int)((__int64)(*(_QWORD *)(v31 + 112) - *(_QWORD *)(v31 + 104)) >> 3);
              *(_QWORD *)(v31 + 112) = v33;
              *(_QWORD *)(v31 + 104) = v132;
            }
          }
          *(_QWORD *)v33 = _RDI;
          *(_QWORD *)(v31 + 112) += 8LL;
        }
        ++v39;
        v106 += 10;
      }
      while ( v39 < *(_DWORD *)(a4 + 24) );
    }
    v40 = (int)v36;
    if ( *(_DWORD *)(a4 + 40) > (int)v36 )
    {
      _RSI = v36;
      __asm { vmovss  xmm9, dword ptr cs:1706080h }
      do
      {
        _R15 = *(_QWORD *)(a4 + 32);
        if ( (*((_BYTE *)_RSI + _R15 + 72) & 1) == 0 )
        {
          __asm
          {
            vmovups ymm0, ymmword ptr cs:1AF8A70h
            vmovups [rbp+70h+var_F0], ymm0
            vmovups ymm1, ymmword ptr cs:1AF8A90h
            vmovups [rbp+70h+var_D0], ymm1
            vmovss  xmm0, dword ptr [rsi+r15]
            vxorps  xmm2, xmm0, xmm9
            vmovss  dword ptr [rbp+70h+var_F0], xmm2
            vmovss  xmm1, dword ptr [rsi+r15+4]
            vxorps  xmm0, xmm1, xmm9
            vmovss  dword ptr [rbp+70h+var_F0+4], xmm0
            vmovss  xmm2, dword ptr [rsi+r15+8]
            vxorps  xmm1, xmm2, xmm9
            vmovss  dword ptr [rbp+70h+var_F0+8], xmm1
            vmovss  xmm0, dword ptr [rsi+r15+20h]
            vmovss  dword ptr [rbp+70h+var_F0+10h], xmm0
            vmovss  xmm1, dword ptr [rsi+r15+24h]
            vmovss  dword ptr [rbp+70h+var_F0+14h], xmm1
            vmovss  xmm0, dword ptr [rsi+r15+28h]
            vmovss  dword ptr [rbp+70h+var_F0+18h], xmm0
            vmovss  xmm1, dword ptr [rsi+r15+10h]
            vmovss  dword ptr [rbp+70h+var_D0], xmm1
            vmovss  xmm0, dword ptr [rsi+r15+14h]
            vmovss  dword ptr [rbp+70h+var_D0+4], xmm0
            vmovss  xmm1, dword ptr [rsi+r15+18h]
            vmovss  dword ptr [rbp+70h+var_D0+8], xmm1
            vmovss  xmm0, dword ptr [rsi+r15+30h]
            vmovss  dword ptr [rbp+70h+var_D0+10h], xmm0
            vmovss  xmm1, dword ptr [rsi+r15+34h]
            vmovss  dword ptr [rbp+70h+var_D0+14h], xmm1
            vmovss  xmm0, dword ptr [rsi+r15+38h]
            vmovss  dword ptr [rbp+70h+var_D0+18h], xmm0
          }
          __asm { vzeroupper }
          sub_2CE478(_RDI, _RSI, &v170, &v168);
          __asm
          {
            vmulss  xmm6, xmm10, [rsp+170h+var_104]
            vmulss  xmm7, xmm12, [rsp+170h+var_100]
            vmulss  xmm8, xmm11, [rsp+170h+var_FC]
          }
          v98 = sub_1BF3C8(v94, v95, 8, 144, v96, v97);
          _RDI = v98;
          v36 = nullptr;
          if ( v98 != 0 )
          {
            *(_DWORD *)(v98 + 8) = 3;
            *(_QWORD *)v98 = 20213208;
            *(_QWORD *)(v98 + 12) = 1065353216;
            *(_QWORD *)(v98 + 20) = 0;
            *(_QWORD *)(v98 + 28) = 1065353216;
            *(_QWORD *)(v98 + 36) = 0;
            *(_QWORD *)(v98 + 44) = 1065353216;
            *(_QWORD *)(v98 + 52) = 0;
            *(_QWORD *)(v98 + 60) = 8;
            *(_QWORD *)(v98 + 72) = 0;
            *(_DWORD *)(v98 + 88) = -1082130432;
            *(_DWORD *)(v98 + 80) = 1065353216;
            *(_DWORD *)(v98 + 84) = -1082130432;
            *(_QWORD *)(v98 + 96) = 0;
            *(_QWORD *)(v98 + 104) = 0;
            *(_QWORD *)(v98 + 112) = 0;
            *(_QWORD *)(v98 + 120) = 0;
            *(_DWORD *)(v98 + 128) = 1065353216;
            *(_QWORD *)(v98 + 132) = 1065353216;
          }
          else
          {
            _RDI = 0;
          }
          __asm
          {
            vmulss  xmm0, xmm10, dword ptr [rsi+r15+40h]
            vaddss  xmm1, xmm0, xmm13
            vmovss  dword ptr [rdi+80h], xmm1
            vmulss  xmm0, xmm10, dword ptr [rsi+r15+44h]
            vmovss  dword ptr [rdi+84h], xmm0
            vmovups ymm1, [rsp+170h+var_128]
            vmovups ymmword ptr [rdi+0Ch], ymm1
          }
          *(_DWORD *)(_RDI + 44) = v169;
          __asm
          {
            vmovss  dword ptr [rdi+30h], xmm6
            vmovss  dword ptr [rdi+34h], xmm7
            vmovss  dword ptr [rdi+38h], xmm8
          }
          v33 = *(_QWORD *)(v31 + 112);
          j = *(__int64 **)(v31 + 120);
          if ( (unsigned __int64)j <= v33 )
          {
            v35 = *(__int64 **)(v31 + 104);
            v105 = 2 * ((__int64)(v33 - (_QWORD)v35) >> 3) + 2;
            j = v35 != nullptr ? (__int64 *)(j - v35) : nullptr;
            if ( (unsigned int)j < v105 )
            {
              v135 = 8LL * v105;
              __asm { vzeroupper }
              v136 = MEMORY[0x131BE18](_RDI, _RSI, j, v135);
              v35 = (__int64 *)v136;
              v137 = *(__int64 **)(v31 + 112);
              for ( j = *(__int64 **)(v31 + 104); j != v137; ++j )
                *v35++ = *j;
              v138 = *(_QWORD *)(v31 + 104);
              v36 = nullptr;
              if ( v138 != 0 )
              {
                MEMORY[0x131BE10](_RDI, _RSI, j, v138, v35);
                v36 = nullptr;
              }
              *(_QWORD *)(v31 + 120) = v136 + v135;
              v33 = v136 + 8LL * (unsigned int)((__int64)(*(_QWORD *)(v31 + 112) - *(_QWORD *)(v31 + 104)) >> 3);
              *(_QWORD *)(v31 + 112) = v33;
              *(_QWORD *)(v31 + 104) = v136;
            }
          }
          *(_QWORD *)v33 = _RDI;
          *(_QWORD *)(v31 + 112) += 8LL;
        }
        ++v40;
        _RSI += 10;
      }
      while ( v40 < *(_DWORD *)(a4 + 40) );
    }
    _RDI = a3;
    v21 = v178;
  }
  v41 = (unsigned __int64)v36;
  v163 = v36;
  v42 = (int)v36;
  v164 = 0;
  if ( v21 == nullptr )
  {
    __asm { vzeroupper }
    sub_2D1AD0(_RDI, v36, a4, &v163, _RDI);
    v21 = &v163;
    v178 = &v163;
    v42 = v164;
    v41 = (unsigned __int64)v163;
    v36 = nullptr;
  }
  v43 = (int)v36;
  if ( *((_DWORD *)v21 + 2) > (int)v36 )
  {
    LODWORD(v44) = v179;
    do
    {
      v165 = 20215408;
      v167 = (int)v36;
      v166 = *v21 + 16LL * v43;
      __asm { vzeroupper }
      v45 = sub_569150(_RDI, v44, (_DWORD)j, (unsigned int)&v165, (_DWORD)v35, (_DWORD)v36, v160, v161, v162);
      v46 = v45;
      v36 = nullptr;
      if ( v45 != 0 )
      {
        *(_DWORD *)(v45 + 8) = 2;
        ++MEMORY[0x1C20F30];
        if ( (_DWORD)v44 != 0 && (unsigned int)sub_2CF640(_RDI, v44, v45, v31, v35, 0) != 0 )
        {
          sub_569360(_RDI, v44, v47, v46);
          --MEMORY[0x1C20F30];
          v36 = nullptr;
        }
        else
        {
          v48 = sub_1BF3C8(_RDI, v44, 8, 144, (_DWORD)v35, (_DWORD)v36);
          _RDI = v48;
          v36 = nullptr;
          if ( v48 != 0 )
          {
            *(_DWORD *)(v48 + 8) = 5;
            *(_QWORD *)v48 = 20215512;
            *(_QWORD *)(v48 + 12) = 1065353216;
            *(_QWORD *)(v48 + 20) = 0;
            *(_QWORD *)(v48 + 28) = 1065353216;
            *(_QWORD *)(v48 + 36) = 0;
            *(_QWORD *)(v48 + 44) = 1065353216;
            *(_QWORD *)(v48 + 52) = 0;
            *(_QWORD *)(v48 + 60) = 8;
            *(_QWORD *)(v48 + 72) = 0;
            *(_DWORD *)(v48 + 88) = -1082130432;
            *(_DWORD *)(v48 + 80) = 1065353216;
            *(_DWORD *)(v48 + 84) = -1082130432;
            *(_QWORD *)(v48 + 96) = 0;
            *(_QWORD *)(v48 + 104) = 0;
            *(_QWORD *)(v48 + 112) = 0;
            *(_QWORD *)(v48 + 120) = 0;
            *(_QWORD *)(v48 + 128) = 0;
            *(_DWORD *)(v48 + 136) = 0;
          }
          else
          {
            _RDI = 0;
          }
          *(_QWORD *)(_RDI + 128) = v46;
          *(_DWORD *)(_RDI + 136) = 0;
          v33 = *(_QWORD *)(v31 + 112);
          j = *(__int64 **)(v31 + 120);
          if ( (unsigned __int64)j <= v33 )
          {
            v35 = *(__int64 **)(v31 + 104);
            v62 = 2 * ((__int64)(v33 - (_QWORD)v35) >> 3) + 2;
            j = v35 != nullptr ? (__int64 *)(j - v35) : nullptr;
            if ( (unsigned int)j < v62 )
            {
              v63 = v62;
              v64 = (__int64 *)MEMORY[0x131BE18](_RDI, v44, j, v63 * 8);
              v65 = v64;
              v35 = *(__int64 **)(v31 + 112);
              for ( j = *(__int64 **)(v31 + 104); j != v35; ++j )
                *v64++ = *j;
              v66 = *(_QWORD *)(v31 + 104);
              v36 = nullptr;
              if ( v66 != 0 )
              {
                MEMORY[0x131BE10](_RDI, v44, j, v66, v35);
                v36 = nullptr;
              }
              *(_QWORD *)(v31 + 120) = &v65[v63];
              v33 = (unsigned __int64)&v65[(unsigned int)((__int64)(*(_QWORD *)(v31 + 112) - *(_QWORD *)(v31 + 104)) >> 3)];
              *(_QWORD *)(v31 + 112) = v33;
              *(_QWORD *)(v31 + 104) = v65;
              v21 = v178;
            }
          }
          *(_QWORD *)v33 = _RDI;
          *(_QWORD *)(v31 + 112) += 8LL;
        }
      }
      ++v43;
    }
    while ( v43 < *((_DWORD *)v21 + 2) );
    v41 = (unsigned __int64)v163;
  }
  if ( (unsigned int)((__int64)(*(_QWORD *)(v31 + 112) - *(_QWORD *)(v31 + 104)) >> 3) == 0 )
  {
    if ( v31 != 0 )
    {
      __asm { vzeroupper }
      sub_2CFE40(_RDI, v41, j, v31);
      v36 = nullptr;
    }
    v31 = (__int64)v36;
  }
  if ( v42 > 0 )
  {
    v49 = v41;
    for ( k = (unsigned int)v42; k != 0; --k )
    {
      __asm { vzeroupper }
      sub_BF354(v49, v41, j, v49);
      v49 = v51 + 16;
    }
  }
  if ( v41 != 0 )
  {
    v67 = MEMORY[0x1B9BC10];
    if ( MEMORY[0x1B9BC10] == 0 )
    {
      __asm { vzeroupper }
      sub_411A40(MEMORY[0x1B9BC10], v41, (_DWORD)j, v33, (_DWORD)v35, (_DWORD)v36, v160);
      v67 = MEMORY[0x1B9BC10];
    }
    _R8 = *(_QWORD *)(*(_QWORD *)v67 + 24LL);
    if ( (__int64 (__fastcall *)(int, int, int, int, int, int, __int64))_R8 != sub_DC3F0 )
    {
      __asm { vzeroupper }
      ((void (__fastcall *)(__int64, unsigned __int64, unsigned __int64, __int64))_R8)(v67, v41, v41, v67);
      goto LABEL_26;
    }
    if ( (_WORD)v41 != 0 && MEMORY[0x1AD0394] != -1 )
    {
      __asm { vzeroupper }
      _RDX = MEMORY[0x131A378]();
      LODWORD(v36) = 0;
      if ( _RDX != 0 && *((_BYTE *)&loc_2 + (v41 & 0xFFFFFFFFFFFF0000LL) + 1) == 0xE3 )
      {
        _R8 = 3LL * *((unsigned __int8 *)&loc_2 + (v41 & 0xFFFFFFFFFFFF0000LL));
        if ( *(_DWORD *)(_RDX + 24LL * *((unsigned __int8 *)&loc_2 + (v41 & 0xFFFFFFFFFFFF0000LL)) + 8) < 0x40u
          && *(_DWORD *)(_RDX + 24LL * *((unsigned __int8 *)&loc_2 + (v41 & 0xFFFFFFFFFFFF0000LL)) + 8)
           * (unsigned int)*(unsigned __int16 *)(v41 & 0xFFFFFFFFFFFF0000LL) < (unsigned int)sub_10000 )
        {
LABEL_49:
          *(_QWORD *)v41 = *(_QWORD *)(_RDX + 8 * _R8);
          *(_QWORD *)(v41 + 8) = 0;
          *(_QWORD *)(_RDX + 8 * _R8) = v41;
          ++*(_DWORD *)(_RDX + 8 * _R8 + 8);
          goto LABEL_26;
        }
        if ( *(_QWORD *)(_RDX + 24LL * *((unsigned __int8 *)&loc_2 + (v41 & 0xFFFFFFFFFFFF0000LL)) + 12) == 0 )
        {
          __asm
          {
            vmovsd  xmm0, qword ptr [rdx+r8*8]
            vmovsd  qword ptr [rdx+r8*8+0Ch], xmm0
          }
          *(_DWORD *)(_RDX + 8 * _R8 + 20) = *(_DWORD *)(_RDX + 8 * _R8 + 8);
          *(_QWORD *)(_RDX + 8 * _R8) = 0;
          *(_DWORD *)(_RDX + 8 * _R8 + 8) = 0;
          goto LABEL_49;
        }
      }
    }
    __asm { vzeroupper }
    sub_DBC6C(v67, v41, v41, v67, _R8, (_DWORD)v36, v160, v161);
  }
LABEL_26:
  __asm { vzeroupper }
  _R11 = &v172;
  __asm
  {
    vmovaps xmm6, xmmword ptr [r11-10h]
    vmovaps xmm7, xmmword ptr [r11-20h]
    vmovaps xmm8, xmmword ptr [r11-30h]
    vmovaps xmm9, xmmword ptr [r11-40h]
    vmovaps xmm10, xmmword ptr [r11-50h]
    vmovaps xmm11, xmmword ptr [r11-60h]
    vmovaps xmm12, xmmword ptr [r11-70h]
    vmovaps xmm13, xmmword ptr [r11-80h]
  }
  return v31;
}
