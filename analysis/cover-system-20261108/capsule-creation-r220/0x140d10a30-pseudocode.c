__int64 __fastcall sub_D0FA30(
        __int64 a1,
        double a2,
        double a3,
        double a4,
        double a5,
        double a6,
        double a7,
        __m128 _XMM6,
        __m128 _XMM7,
        __int64 a10,
        __int64 a11,
        __int64 a12)
{
  __int64 v20; // rdi
  __int64 v21; // rsi
  __int64 v23; // rdi
  __int64 v24; // rsi
  int v30; // r9d
  __int64 **v35; // r8
  unsigned int v36; // eax
  __int64 v37; // rcx
  __int64 **v38; // rax
  __int64 **v39; // rbx
  __int64 **v40; // r8
  __int64 **v41; // rdx
  __int64 v44; // rdx
  __int64 v45; // rbx
  __int64 v46; // rdi
  __int64 v47; // rsi
  __int64 v48; // r8
  __m256 v55; // [rsp+20h] [rbp-E0h] BYREF
  int v56; // [rsp+40h] [rbp-C0h]
  _QWORD *v60; // [rsp+50h] [rbp-B0h]
  __int64 v62; // [rsp+5Ch] [rbp-A4h]
  __int64 v63; // [rsp+68h] [rbp-98h]
  __int64 v64; // [rsp+70h] [rbp-90h]
  __int64 v65; // [rsp+78h] [rbp-88h]
  int v66; // [rsp+80h] [rbp-80h]
  __int64 **v67; // [rsp+88h] [rbp-78h]
  __int64 **v68; // [rsp+90h] [rbp-70h]
  char *v69; // [rsp+98h] [rbp-68h]
  _QWORD v70[4]; // [rsp+B0h] [rbp-50h] BYREF
  int v90; // [rsp+11Ch] [rbp+1Ch]
  int v93; // [rsp+128h] [rbp+28h]
  __int64 v96; // [rsp+140h] [rbp+40h] BYREF
  int v97; // [rsp+148h] [rbp+48h]
  __int64 v103; // [rsp+17Ch] [rbp+7Ch]
  __int64 v104; // [rsp+188h] [rbp+88h]
  char v111[112]; // [rsp+1D0h] [rbp+D0h] BYREF
  char vars0; // [rsp+240h] [rbp+140h] BYREF
  _UNKNOWN *retaddr; // [rsp+248h] [rbp+148h] BYREF

  _RAX = &retaddr;
  __asm
  {
    vmovaps xmmword ptr [rax-18h], xmm6
    vmovaps xmmword ptr [rax-28h], xmm7
    vmovaps xmmword ptr [rax-38h], xmm8
    vmovaps xmmword ptr [rax-48h], xmm9
  }
  sub_576E65(a1, a12, 0, &v55, 136);
  v60 = nullptr;
  __asm
  {
    vxorps  xmm6, xmm6, xmm6
    vmovss  [rsp+240h+var_1E8], xmm6
    vmovdqu xmm8, xmmword ptr cs:1706210h
    vmovups xmmword ptr [rsp+240h+var_220], xmm8
    vmovups xmmword ptr [rsp+240h+var_220+10h], xmm8
    vmovss  xmm9, dword ptr cs:17061B8h
    vmovss  [rsp+240h+var_200], xmm9
    vmovss  [rsp+240h+var_1F4], xmm6
    vmovss  [rsp+240h+var_1F8], xmm6
    vmovss  [rsp+240h+var_1FC], xmm6
  }
  v62 = 0;
  v63 = 0;
  v64 = 0;
  v65 = 0;
  v68 = v67;
  v66 = 1;
  _RAX = sub_F1610(v20, v21, a11, v111);
  __asm
  {
    vmovups ymm0, ymmword ptr [rax]
    vmovups [rsp+240h+var_220], ymm0
  }
  v56 = *(_DWORD *)(_RAX + 32);
  __asm
  {
    vmovss  xmm0, dword ptr [rax+24h]
    vmovss  [rsp+240h+var_1FC], xmm0
    vmovss  xmm1, dword ptr [rax+28h]
    vmovss  [rsp+240h+var_1F8], xmm1
    vmovss  xmm0, dword ptr [rax+2Ch]
    vmovss  [rsp+240h+var_1F4], xmm0
    vmovss  [rsp+240h+var_1E8], xmm9
  }
  LODWORD(v62) = v30 | v62;
  v97 = v30;
  v96 = 20213184;
  __asm
  {
    vmovss  [rbp+140h+var_F4], xmm9
    vmovaps ymm0, ymmword ptr cs:1706420h
    vmovups [rbp+140h+var_F0], ymm0
    vmovss  [rbp+140h+var_C8], xmm6
    vmovss  [rbp+140h+var_CC], xmm6
    vmovss  [rbp+140h+var_D0], xmm6
  }
  v103 = 8;
  v104 = 0;
  __asm
  {
    vmovss  xmm7, dword ptr cs:1706190h
    vmovss  [rbp+140h+var_A8], xmm7
    vmovss  [rbp+140h+var_B0], xmm9
    vmovss  [rbp+140h+var_AC], xmm7
    vpxor   xmm0, xmm0, xmm0
    vmovdqu [rbp+140h+var_A0], xmm0
    vpxor   xmm1, xmm1, xmm1
    vmovups [rbp+140h+var_90], xmm1
    vmovss  [rbp+140h+var_80], xmm9
  }
  v35 = v68;
  if ( v69 <= (char *)v68 )
  {
    v36 = 2 * (v68 - v67) + 2;
    if ( v67 != nullptr )
      v37 = (v69 - (char *)v67) >> 3;
    else
      LODWORD(v37) = 0;
    if ( (unsigned int)v37 < v36 )
    {
      __asm { vzeroupper }
      v38 = (__int64 **)MEMORY[0x131BE18](8LL * v36, v24, v67, 8LL * v36);
      v39 = v38;
      v40 = v68;
      v41 = v67;
      if ( v67 != v68 )
      {
        do
          *v38++ = *v41++;
        while ( v41 != v40 );
        v41 = v67;
      }
      if ( v41 != nullptr )
      {
        MEMORY[0x131BE10](v23, v24, v41, v41, v40);
        v41 = v67;
      }
      v69 = (char *)v39 + v23;
      v35 = &v39[(unsigned int)(v68 - v41)];
      v68 = v35;
      v67 = v39;
      __asm { vmovdqu xmm8, xmmword ptr cs:1706210h }
    }
  }
  *v35 = &v96;
  ++v68;
  __asm
  {
    vmovups [rbp+140h+var_190], xmm8
    vmovups [rbp+140h+var_180], xmm8
    vmovss  [rbp+140h+var_170], xmm9
    vmovss  [rbp+140h+var_164], xmm6
    vmovss  [rbp+140h+var_168], xmm6
    vmovss  [rbp+140h+var_16C], xmm6
    vmovss  [rbp+140h+var_158], xmm6
    vmovss  [rbp+140h+var_15C], xmm6
    vmovss  [rbp+140h+var_160], xmm6
    vmovss  [rbp+140h+var_148], xmm6
    vmovss  [rbp+140h+var_14C], xmm6
    vmovss  [rbp+140h+var_150], xmm6
    vmovss  [rbp+140h+var_13C], xmm6
    vmovss  [rbp+140h+var_140], xmm6
    vmovss  [rbp+140h+var_144], xmm6
    vmovss  xmm0, dword ptr cs:17064ECh
    vmovss  [rbp+140h+var_138], xmm0
    vmovss  [rbp+140h+var_154], xmm6
    vmovss  [rbp+140h+var_134], xmm6
    vmovss  xmm0, dword ptr cs:17061C4h
    vmovss  [rbp+140h+var_130], xmm0
    vmovss  [rbp+140h+var_12C], xmm7
    vmovss  [rbp+140h+var_120], xmm7
    vmovss  [rbp+140h+var_11C], xmm7
    vmovss  [rbp+140h+var_128], xmm6
  }
  v93 = 4;
  __asm
  {
    vmovss  [rbp+140h+var_114], xmm7
    vmovss  [rbp+140h+var_110], xmm6
  }
  v90 = 2432;
  v60 = v70;
  __asm { vzeroupper }
  v45 = (*(__int64 (__fastcall **)(__int64, __int64, __m256 *, __int64))(*(_QWORD *)v24 + 56LL))(v23, v24, &v55, v24);
  *(_QWORD *)(v45 + 8) = 0;
  v96 = 20215384;
  if ( v67 != nullptr )
    MEMORY[0x131BE10](v46, v47, v44, v67, v48);
  _R11 = &vars0;
  __asm
  {
    vmovaps xmm6, xmmword ptr [r11-10h]
    vmovaps xmm7, xmmword ptr [r11-20h]
    vmovaps xmm8, xmmword ptr [r11-30h]
    vmovaps xmm9, xmmword ptr [r11-40h]
  }
  return v45;
}
