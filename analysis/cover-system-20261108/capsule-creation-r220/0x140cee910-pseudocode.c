__int64 __fastcall sub_CED910(
        __int64 a1,
        __int64 a2,
        __int64 a3,
        __int64 _RCX,
        double a5,
        double a6,
        double a7,
        double a8,
        double a9,
        double a10,
        __m128 _XMM6,
        __m128 _XMM7)
{
  __int64 v16; // rbx
  __int64 v30; // rdi
  __int64 v31; // rsi
  int v32; // ecx
  int v33; // edi
  int v34; // esi
  int v35; // r8d
  int v36; // r9d
  __int64 v37; // rax
  int v38; // edi
  int v39; // esi
  int v40; // r8d
  int v41; // r9d
  int v46; // eax
  int v47; // ecx
  int v48; // r14d
  unsigned __int64 v49; // rdi
  __int64 v50; // rsi
  int v55; // r12d
  __int64 v56; // rsi
  __int64 v58; // rdi
  __int64 v59; // rax
  _BYTE *v60; // r13
  __int64 v61; // rcx
  int v63; // edi
  char v64; // r12
  int v65; // edx
  int v66; // esi
  int v67; // edi
  int v68; // esi
  __int64 v69; // rdi
  __int64 v70; // rsi
  int v76; // eax
  bool v77; // zf
  int v78; // esi
  char v79; // cl
  int v81; // edx
  int v82; // ecx
  int v83; // r8d
  int v84; // r9d
  __int64 v102; // rdx
  __int64 v103; // rdi
  __int64 v104; // rsi
  __int64 **v105; // rdx
  __int64 **v106; // rdi
  __int64 v107; // rsi
  __int64 **v108; // r8
  __int64 **v112; // rax
  __int64 **v113; // rax
  __int64 v118; // rax
  __int64 v119; // rdi
  __int64 v120; // rsi
  _QWORD *v121; // rax
  __int64 v122; // r8
  __int64 result; // rax
  __int64 v130; // [rsp+0h] [rbp-328h] BYREF
  __int64 v131; // [rsp+8h] [rbp-320h]
  __int64 v132; // [rsp+10h] [rbp-318h]
  __int64 v133; // [rsp+18h] [rbp-310h]
  int v134; // [rsp+40h] [rbp-2E8h]
  int v135; // [rsp+44h] [rbp-2E4h]
  int v136; // [rsp+50h] [rbp-2D8h]
  unsigned int v137; // [rsp+58h] [rbp-2D0h]
  int v138; // [rsp+5Ch] [rbp-2CCh]
  int v139; // [rsp+60h] [rbp-2C8h]
  _QWORD v140[3]; // [rsp+68h] [rbp-2C0h] BYREF
  __m256 v141; // [rsp+80h] [rbp-2A8h] BYREF
  __m256 v142; // [rsp+A0h] [rbp-288h]
  __int64 v143; // [rsp+C0h] [rbp-268h]
  __int64 v144; // [rsp+C8h] [rbp-260h]
  __int64 v145; // [rsp+D0h] [rbp-258h]
  __m256 v146; // [rsp+E0h] [rbp-248h] BYREF
  int v147; // [rsp+100h] [rbp-228h]
  __int64 v151; // [rsp+110h] [rbp-218h]
  __int64 v153; // [rsp+11Ch] [rbp-20Ch]
  __int64 v154; // [rsp+128h] [rbp-200h]
  __int64 v155; // [rsp+130h] [rbp-1F8h]
  __int64 v156; // [rsp+138h] [rbp-1F0h]
  int v157; // [rsp+140h] [rbp-1E8h]
  __int64 **v158; // [rsp+148h] [rbp-1E0h]
  __int64 **v159; // [rsp+150h] [rbp-1D8h]
  __int64 **v160; // [rsp+158h] [rbp-1D0h]
  __int64 v161; // [rsp+170h] [rbp-1B8h]
  char v162; // [rsp+178h] [rbp-1B0h] BYREF
  __int64 v163; // [rsp+190h] [rbp-198h] BYREF
  int v164; // [rsp+198h] [rbp-190h]
  __int64 v170; // [rsp+1CCh] [rbp-15Ch]
  __int64 v171; // [rsp+1D8h] [rbp-150h]
  __int64 v177; // [rsp+210h] [rbp-118h]
  __int16 v181; // [rsp+224h] [rbp-104h]
  __int16 v182; // [rsp+226h] [rbp-102h]
  int v183; // [rsp+228h] [rbp-100h]
  __m256 v184; // [rsp+230h] [rbp-F8h] BYREF
  int v185; // [rsp+250h] [rbp-D8h]
  _BYTE v189[64]; // [rsp+260h] [rbp-C8h] BYREF
  unsigned __int64 v190; // [rsp+2A0h] [rbp-88h]
  char v191; // [rsp+300h] [rbp-28h] BYREF
  _UNKNOWN *retaddr; // [rsp+328h] [rbp+0h] BYREF

  _RAX = &retaddr;
  __asm
  {
    vmovaps xmmword ptr [rax-38h], xmm6
    vmovaps xmmword ptr [rax-48h], xmm7
    vmovaps xmmword ptr [rax-58h], xmm8
    vmovaps xmmword ptr [rax-68h], xmm9
    vmovaps xmmword ptr [rax-78h], xmm10
  }
  v190 = (unsigned __int64)&v130 ^ MEMORY[0x1ACA440];
  v16 = _RCX;
  if ( (*(_DWORD *)(_RCX + 336) & 0x80000) == 0 )
    goto LABEL_52;
  if ( *(_QWORD *)(MEMORY[0x1C1F890] + 468LL) == 0 )
    goto LABEL_52;
  if ( *(_QWORD *)(_RCX + 380) != 0 )
    goto LABEL_52;
  _R15 = *(_QWORD *)(_RCX + 120);
  if ( *(_DWORD *)(_RCX + 652) == 0 )
    goto LABEL_52;
  __asm
  {
    vmovups ymm0, ymmword ptr [rcx+0A0h]
    vmovups [rsp+328h+var_2A8], ymm0
    vmovups ymm1, ymmword ptr [rcx+0C0h]
    vmovups [rsp+328h+var_288], ymm1
  }
  __asm { vzeroupper }
  sub_F10F4(a1, a2, a3, &v141);
  __asm
  {
    vmovss  xmm4, dword ptr [rsp+328h+var_288]
    vmovss  xmm5, dword ptr [rsp+328h+var_288+4]
    vmovss  xmm6, dword ptr [rsp+328h+var_288+8]
    vmovss  xmm3, dword ptr cs:1706080h
    vmovss  xmm0, dword ptr [rsp+328h+var_2A8+10h]
    vxorps  xmm1, xmm0, xmm3
    vmovss  dword ptr [rsp+328h+var_288], xmm1
    vmovss  xmm2, dword ptr [rsp+328h+var_2A8+14h]
    vxorps  xmm0, xmm2, xmm3
    vmovss  dword ptr [rsp+328h+var_288+4], xmm0
    vmovss  xmm1, dword ptr [rsp+328h+var_2A8+18h]
    vxorps  xmm2, xmm1, xmm3
    vmovss  dword ptr [rsp+328h+var_288+8], xmm2
    vmovss  dword ptr [rsp+328h+var_2A8+10h], xmm4
    vmovss  dword ptr [rsp+328h+var_2A8+14h], xmm5
    vmovss  dword ptr [rsp+328h+var_2A8+18h], xmm6
  }
  sub_F1610(v30, v31, &v141, &v184);
  v37 = sub_2D0F74(v33, v34, 0, v32, v35, v36);
  *(_QWORD *)(v16 + 380) = v37;
  *(_QWORD *)(v37 + 136) = 0;
  *(_QWORD *)(*(_QWORD *)(v16 + 380) + 96LL) = v16;
  *(_DWORD *)(*(_QWORD *)(v16 + 380) + 132LL) = *(_DWORD *)(*(_QWORD *)(MEMORY[0x1C1F890] + 468LL) + 96LL);
  __asm
  {
    vmovss  xmm0, dword ptr [r15+198h]
    vmulss  xmm8, xmm0, dword ptr [r15+1A4h]
    vmulss  xmm9, xmm0, dword ptr [r15+1A0h]
    vmulss  xmm10, xmm0, dword ptr [r15+19Ch]
  }
  v46 = *(_DWORD *)(_R15 + 712);
  v47 = *(_DWORD *)(_R15 + 724);
  if ( v47 >= 1 )
  {
    v48 = *(_DWORD *)(_R15 + 712);
    if ( v47 < v46 )
      v48 = *(_DWORD *)(_R15 + 724);
  }
  else
  {
    v48 = 1;
  }
  v135 = v46 / v48;
  v136 = sub_2D184C(
           v38,
           v39,
           *(_QWORD *)(MEMORY[0x1C1F350] + 768LL),
           *(_QWORD *)(MEMORY[0x1C1F890] + 468LL),
           v40,
           v41,
           v130,
           v131);
  v139 = v136 + 1;
  __asm { vxorps  xmm7, xmm7, xmm7 }
  if ( *(_QWORD *)(v16 + 660) == 0 )
  {
    v137 = *(_DWORD *)(v16 + 572) * v48 + 1;
    v55 = *(_DWORD *)(v16 + 568) * v48 + 1;
    v138 = v55;
    v140[0] = 0;
    v140[1] = 0;
    sub_1BEC94(v49, v137, v137 * v55, (unsigned int)v140, 4, 8);
    v58 = v140[0];
    if ( v55 > 0 )
    {
      v134 = 0;
      v59 = 4LL * (int)v56;
      v161 = v59;
      v60 = (_BYTE *)(v140[0] + 2LL);
      v144 = v140[0] + 2LL;
      v61 = (unsigned int)v55;
      v145 = (unsigned int)v55;
      __asm { vmovss  xmm6, dword ptr cs:17060E4h }
      v63 = 0;
      v64 = v136;
      v65 = v135;
      do
      {
        if ( (int)v56 > 0 )
        {
          v134 = 0;
          v143 = (unsigned int)v56;
          v66 = 0;
          do
          {
            v134 = v63 + *(_DWORD *)(v16 + 560);
            v136 = v66 + *(_DWORD *)(v16 + 564);
            sub_C3E934(v63, v66, (unsigned int)v189, _R15, v134, v136, v130, v131, v132, v133);
            sub_C3E308(
              v67,
              v68,
              (unsigned int)&v162,
              _R15,
              (unsigned int)v189,
              v134,
              v130,
              v131,
              v132,
              v133,
              v136,
              0,
              0,
              v48);
            __asm
            {
              vmulss  xmm1, xmm6, [rsp+328h+var_1A8]
              vmulss  xmm2, xmm1, dword ptr cs:18FED84h
              vaddss  xmm3, xmm2, xmm6
              vcvtss2si eax, xmm3
            }
            v76 = _EAX >> 1;
            if ( (__int16)v76 >= -32766 )
            {
              if ( (__int16)v76 >= 32766 )
                LOWORD(v76) = 32766;
            }
            else
            {
              LOWORD(v76) = -32766;
            }
            *((_WORD *)v60 - 1) = v76;
            v77 = (*(_BYTE *)sub_9BA5AC(
                               v69,
                               v70,
                               (unsigned int)(v134 - v134 % *(_DWORD *)(_R15 + 712)),
                               _R15,
                               (unsigned int)(v136 - v136 % *(_DWORD *)(_R15 + 712)))
                 & 1) == 0;
            v79 = v64;
            if ( !v77 )
              v79 = v64 + 1;
            v60[1] ^= (v60[1] ^ v79) & 0x7F;
            *v60 = v79 | 0x80;
            v60 += 4;
            v66 = v135 + v78;
            --v143;
          }
          while ( v143 != 0 );
          v56 = v137;
          v60 = (_BYTE *)v144;
          v59 = v161;
          v61 = v145;
          v65 = v135;
        }
        v63 += v65;
        v60 += v59;
        v144 = (__int64)v60;
        v145 = --v61;
      }
      while ( v61 != 0 );
      v58 = v140[0];
      v55 = v138;
    }
    *(_QWORD *)&v141.m256_f32[2] = 0x400000001LL;
    __asm
    {
      vmovss  dword ptr [rsp+328h+var_2A8+1Ch], xmm7
      vmovss  dword ptr [rsp+328h+var_288], xmm7
    }
    *(_QWORD *)v141.m256_f32 = __PAIR64__(v56, v55);
    *(_QWORD *)&v141.m256_f32[4] = v58;
    __asm
    {
      vmovss  xmm0, dword ptr cs:18FEE58h
      vmovss  dword ptr [rsp+328h+var_2A8+18h], xmm0
    }
    LODWORD(v142.m256_f32[1]) = 1;
    *(_QWORD *)(v16 + 660) = (*(__int64 (__fastcall **)(__int64, __int64, __m256 *))(*MEMORY[0x1C20F38] + 88LL))(
                               v58,
                               v56,
                               &v141);
    if ( v49 != 0 )
    {
      v50 = MEMORY[0x1B9BC10];
      if ( MEMORY[0x1B9BC10] == 0 )
      {
        sub_411A40(v49, 0, v81, v82, v83, v84, v130);
        v50 = MEMORY[0x1B9BC10];
      }
      _R8 = *(_QWORD *)(*(_QWORD *)v50 + 24LL);
      if ( (__int64 (__fastcall *)(int, int, int, int, int, int, __int64))_R8 == sub_DC3F0 )
      {
        if ( (_WORD)v49 != 0 && MEMORY[0x1AD0394] != -1 )
        {
          _RDX = MEMORY[0x131A378]();
          if ( _RDX != 0 && *((_BYTE *)&loc_2 + (v49 & 0xFFFFFFFFFFFF0000LL) + 1) == 0xE3 )
          {
            _R8 = 3LL * *((unsigned __int8 *)&loc_2 + (v49 & 0xFFFFFFFFFFFF0000LL));
            if ( *(_DWORD *)(_RDX + 24LL * *((unsigned __int8 *)&loc_2 + (v49 & 0xFFFFFFFFFFFF0000LL)) + 8) < 0x40u
              && *(_DWORD *)(_RDX + 24LL * *((unsigned __int8 *)&loc_2 + (v49 & 0xFFFFFFFFFFFF0000LL)) + 8)
               * (unsigned int)*(unsigned __int16 *)(v49 & 0xFFFFFFFFFFFF0000LL) < (unsigned int)sub_10000 )
            {
LABEL_35:
              *(_QWORD *)v49 = *(_QWORD *)(_RDX + 8 * _R8);
              *(_QWORD *)(v49 + 8) = 0;
              *(_QWORD *)(_RDX + 8 * _R8) = v49;
              ++*(_DWORD *)(_RDX + 8 * _R8 + 8);
              goto LABEL_38;
            }
            if ( *(_QWORD *)(_RDX + 24LL * *((unsigned __int8 *)&loc_2 + (v49 & 0xFFFFFFFFFFFF0000LL)) + 12) == 0 )
            {
              __asm
              {
                vmovsd  xmm0, qword ptr [rdx+r8*8]
                vmovsd  qword ptr [rdx+r8*8+0Ch], xmm0
              }
              *(_DWORD *)(_RDX + 8 * _R8 + 20) = *(_DWORD *)(_RDX + 8 * _R8 + 8);
              *(_QWORD *)(_RDX + 8 * _R8) = 0;
              *(_DWORD *)(_RDX + 8 * _R8 + 8) = 0;
              goto LABEL_35;
            }
          }
        }
        sub_DBC6C(v49, v50, v49, v50, _R8, v84, v130, v131);
      }
      else
      {
        ((void (__fastcall *)(unsigned __int64, __int64, unsigned __int64, __int64))_R8)(v49, v50, v49, v50);
      }
    }
  }
LABEL_38:
  v164 = 7;
  v163 = 23532448;
  __asm
  {
    vmovss  xmm6, dword ptr cs:17061B8h
    vmovss  [rsp+328h+var_18C], xmm6
    vmovaps ymm0, ymmword ptr cs:1706420h
    vmovups [rsp+328h+var_188], ymm0
    vmovss  [rsp+328h+var_160], xmm7
    vmovss  [rsp+328h+var_164], xmm7
    vmovss  [rsp+328h+var_168], xmm7
  }
  v170 = 40;
  v171 = 0;
  __asm
  {
    vmovss  xmm0, dword ptr cs:1706190h
    vmovss  [rsp+328h+var_140], xmm0
    vmovss  [rsp+328h+var_148], xmm6
    vmovss  [rsp+328h+var_144], xmm0
    vpxor   xmm0, xmm0, xmm0
    vmovdqu [rsp+328h+var_138], xmm0
    vpxor   xmm1, xmm1, xmm1
    vmovups [rsp+328h+var_128], xmm1
  }
  v177 = *(_QWORD *)(v16 + 660);
  __asm
  {
    vmulss  xmm0, xmm8, dword ptr cs:18FEA90h
    vmovss  [rsp+328h+var_110], xmm0
    vxorps  xmm2, xmm2, xmm2
    vcvtsi2ss xmm2, xmm2, r13d
    vmulss  xmm0, xmm10, dword ptr cs:17060C4h
    vmulss  xmm1, xmm0, xmm2
    vmovss  [rsp+328h+var_10C], xmm1
    vmulss  xmm0, xmm9, dword ptr cs:18FEDDCh
    vmulss  xmm1, xmm0, xmm2
    vmovss  [rsp+328h+var_108], xmm1
  }
  v183 = 0;
  v181 = 0;
  v182 = v139;
  __asm { vzeroupper }
  _RAX = sub_10A78C(v49, v50, 0, v140, 0);
  __asm
  {
    vmovups xmm0, xmmword ptr [rax]
    vmovdqu [rsp+328h+var_128], xmm0
  }
  sub_576E65(v103, v104, v102, &v146, 136);
  v151 = 0;
  __asm
  {
    vmovss  [rsp+328h+var_210], xmm7
    vmovaps ymm0, ymmword ptr cs:1706860h
    vmovups [rsp+328h+var_248], ymm0
    vmovss  [rsp+328h+var_228], xmm6
    vmovss  [rsp+328h+var_21C], xmm7
    vmovss  [rsp+328h+var_220], xmm7
    vmovss  [rsp+328h+var_224], xmm7
  }
  v153 = 0;
  v154 = 0;
  v155 = 0;
  v156 = 0;
  v112 = v158;
  v159 = v158;
  v157 = 1;
  if ( v160 <= v158 && (v158 == nullptr || (unsigned int)(v160 - v158) < 2) )
  {
    __asm { vzeroupper }
    v113 = (__int64 **)MEMORY[0x131BE18](v106, v107, v105, 16);
    v106 = v113;
    v108 = v159;
    v105 = v158;
    if ( v158 != v159 )
    {
      do
        *v113++ = *v105++;
      while ( v105 != v108 );
      v105 = v158;
    }
    if ( v105 != nullptr )
    {
      MEMORY[0x131BE10](v106, v107, v105, v105, v108);
      v105 = v158;
    }
    v160 = v106 + 2;
    v112 = &v106[(unsigned int)(v159 - v105)];
    v159 = v112;
    v158 = v106;
  }
  *v112 = &v163;
  ++v159;
  __asm
  {
    vmovups ymm0, [rsp+328h+var_F8]
    vmovups [rsp+328h+var_248], ymm0
  }
  v147 = v185;
  __asm
  {
    vmovss  xmm0, [rsp+328h+var_D4]
    vmovss  [rsp+328h+var_224], xmm0
    vmovss  xmm1, [rsp+328h+var_D0]
    vmovss  [rsp+328h+var_220], xmm1
    vmovss  xmm0, [rsp+328h+var_CC]
    vmovss  [rsp+328h+var_21C], xmm0
  }
  __asm { vzeroupper }
  v118 = sub_2AB6C8(v106, v107, v105, *(_QWORD *)(MEMORY[0x1C1F890] + 468LL), v108);
  v121 = (_QWORD *)(*(__int64 (__fastcall **)(__int64, __int64, __m256 *, __int64))(*(_QWORD *)v118 + 56LL))(
                     v119,
                     v120,
                     &v146,
                     v118);
  if ( v121 != nullptr )
  {
    *(_QWORD *)(*(_QWORD *)(v16 + 380) + 136LL) = v121;
    v121[1] = *(_QWORD *)(v16 + 380);
    if ( (*(_BYTE *)(_R15 + 784) & 0x20) != 0 )
      (*(void (__fastcall **)(__int64, __int64, __int64, _QWORD *))(*v121 + 168LL))(a1, a2, 3, v121);
  }
  if ( v158 != nullptr )
    MEMORY[0x131BE10](a1, a2, a3, v158, v122);
LABEL_52:
  result = sub_53D9B0(a1, a2, a3, (unsigned __int64)&v130 ^ v190);
  _R11 = &v191;
  __asm
  {
    vmovaps xmm6, xmmword ptr [r11-10h]
    vmovaps xmm7, xmmword ptr [r11-20h]
    vmovaps xmm8, xmmword ptr [r11-30h]
    vmovaps xmm9, xmmword ptr [r11-40h]
    vmovaps xmm10, xmmword ptr [r11-50h]
  }
  return result;
}
