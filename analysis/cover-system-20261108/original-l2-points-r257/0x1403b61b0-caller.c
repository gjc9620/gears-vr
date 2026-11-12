__int64 __fastcall sub_3B51B0(
        _DWORD a1,
        __int64 a2,
        __int64 _RDX,
        __int64 _RCX,
        int a5,
        char a6,
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
        __int64 a18,
        unsigned __int8 a19)
{
  __int64 v25; // r14
  __int64 v31; // rax
  __int64 v32; // rdi
  __int64 v33; // rsi
  unsigned int v35; // esi
  int v45; // ecx
  int v46; // ecx
  int v69; // eax
  int v117; // eax
  __int64 v118; // [rsp+20h] [rbp-A1h] BYREF
  int v119; // [rsp+28h] [rbp-99h]
  _BYTE v121[16]; // [rsp+40h] [rbp-81h] BYREF
  _BYTE v122[176]; // [rsp+50h] [rbp-71h] BYREF
  char v123; // [rsp+100h] [rbp+3Fh] BYREF
  _UNKNOWN *retaddr; // [rsp+118h] [rbp+57h] BYREF

  _RAX = &retaddr;
  v25 = a5;
  _RDI = _RDX;
  _RBX = _RCX;
  __asm
  {
    vmovaps xmmword ptr [rax-28h], xmm6
    vmovaps xmmword ptr [rax-38h], xmm7
    vmovaps xmmword ptr [rax-48h], xmm8
    vmovaps xmmword ptr [rax-58h], xmm9
    vmovaps xmmword ptr [rax-68h], xmm10
    vmovaps xmmword ptr [rax-78h], xmm11
    vmovaps xmmword ptr [rax-88h], xmm12
  }
  if ( a5 < 0 || a5 >= *(_DWORD *)(_RCX + 840) )
  {
    __asm { vmovsd  xmm0, qword ptr [rcx+130h] }
    v117 = *(_DWORD *)(_RCX + 312);
    __asm { vmovsd  qword ptr [rdx], xmm0 }
    *(_DWORD *)(_RDX + 8) = v117;
    goto LABEL_16;
  }
  sub_10E6CC(_RDX, a2, &v118, _RCX, (unsigned int)a5);
  if ( a6 == 0 )
    a6 = *(_BYTE *)(212 * v25 + *(_QWORD *)(_RBX + 832) + 9);
  if ( a6 == 1 )
  {
    __asm { vmovsd  xmm0, qword ptr [rbx+384h] }
    _R12D = *(_DWORD *)(_RBX + 908);
  }
  else
  {
    __asm { vmovsd  xmm0, qword ptr [rbx+390h] }
    _R12D = *(_DWORD *)(_RBX + 920);
  }
  __asm { vmovsd  [rsp+110h+var_E0], xmm0 }
  if ( a19 == 0 )
  {
    __asm
    {
      vmovd   xmm1, r12d
      vaddss  xmm1, xmm1, [rsp+110h+var_E8]
      vmovss  [rsp+110h+var_E8], xmm1
    }
    goto LABEL_15;
  }
  v31 = sub_10DB80(_RDI, a19, v121, _RBX);
  _RAX = sub_1320E0(v32, v33, v31, v122);
  __asm
  {
    vmovss  xmm0, dword ptr [rax]
    vmovss  xmm7, dword ptr [rax+4]
    vmovss  xmm8, dword ptr [rax+8]
    vmovss  xmm2, dword ptr [rax+10h]
    vmovss  xmm9, dword ptr [rax+14h]
    vmovss  xmm10, dword ptr [rax+18h]
    vmovss  xmm3, dword ptr [rax+20h]
    vmovss  xmm11, dword ptr [rax+24h]
    vmovss  xmm12, dword ptr [rax+28h]
  }
  if ( (unsigned __int8)v35 <= 9u && (v45 = 554, _bittest(&v45, v35)) )
  {
    __asm
    {
      vmovss  xmm6, dword ptr [rsp+110h+var_E0]
      vmovss  xmm5, dword ptr [rsp+110h+var_E0+4]
      vmulss  xmm1, xmm0, xmm6
      vmulss  xmm0, xmm2, xmm5
      vsubss  xmm2, xmm1, xmm0
      vmovd   xmm4, r12d
      vmulss  xmm1, xmm3, xmm4
      vaddss  xmm2, xmm2, xmm1
      vaddss  xmm2, xmm2, dword ptr [rsp+110h+var_F0]
      vmovss  dword ptr [rsp+110h+var_F0], xmm2
      vmulss  xmm1, xmm7, xmm6
      vmulss  xmm0, xmm9, xmm5
      vsubss  xmm2, xmm1, xmm0
      vmulss  xmm1, xmm11, xmm4
      vaddss  xmm3, xmm2, xmm1
      vaddss  xmm2, xmm3, dword ptr [rsp+110h+var_F0+4]
      vmulss  xmm1, xmm8, xmm6
      vmulss  xmm0, xmm10, xmm5
      vmovss  dword ptr [rsp+110h+var_F0+4], xmm2
      vsubss  xmm2, xmm1, xmm0
    }
  }
  else
  {
    if ( (unsigned __int8)v35 > 0xAu || (v46 = 1108, !_bittest(&v46, v35)) )
    {
      if ( a6 != 2 || (((_BYTE)v35 - 7) & 0xFA) != 0 || (_BYTE)v35 == 12 )
        goto LABEL_15;
      __asm
      {
        vmovss  xmm5, dword ptr [rbx+39Ch]
        vmovss  xmm4, dword ptr [rbx+3A0h]
        vmovss  xmm6, dword ptr [rbx+3A4h]
        vmulss  xmm1, xmm4, xmm2
        vmulss  xmm0, xmm5, xmm0
        vaddss  xmm2, xmm1, xmm0
        vmulss  xmm1, xmm6, xmm3
        vaddss  xmm3, xmm2, xmm1
        vaddss  xmm2, xmm3, dword ptr [rsp+110h+var_F0]
        vmovss  dword ptr [rsp+110h+var_F0], xmm2
        vmulss  xmm1, xmm4, xmm9
        vmulss  xmm0, xmm5, xmm7
        vaddss  xmm2, xmm1, xmm0
        vmulss  xmm1, xmm6, xmm11
        vaddss  xmm3, xmm2, xmm1
        vaddss  xmm2, xmm3, dword ptr [rsp+110h+var_F0+4]
        vmulss  xmm1, xmm4, xmm10
        vmulss  xmm0, xmm5, xmm8
        vmovss  dword ptr [rsp+110h+var_F0+4], xmm2
        vaddss  xmm2, xmm1, xmm0
        vmulss  xmm1, xmm6, xmm12
      }
      goto LABEL_14;
    }
    __asm
    {
      vmovss  xmm6, dword ptr [rsp+110h+var_E0]
      vmovss  xmm5, dword ptr [rsp+110h+var_E0+4]
      vmulss  xmm1, xmm0, xmm6
      vmulss  xmm0, xmm2, xmm5
      vaddss  xmm2, xmm1, xmm0
      vmovd   xmm4, r12d
      vmulss  xmm1, xmm3, xmm4
      vaddss  xmm2, xmm2, xmm1
      vaddss  xmm2, xmm2, dword ptr [rsp+110h+var_F0]
      vmovss  dword ptr [rsp+110h+var_F0], xmm2
      vmulss  xmm1, xmm7, xmm6
      vmulss  xmm0, xmm9, xmm5
      vaddss  xmm2, xmm1, xmm0
      vmulss  xmm1, xmm11, xmm4
      vaddss  xmm3, xmm2, xmm1
      vaddss  xmm2, xmm3, dword ptr [rsp+110h+var_F0+4]
      vmulss  xmm1, xmm8, xmm6
      vmulss  xmm0, xmm10, xmm5
      vmovss  dword ptr [rsp+110h+var_F0+4], xmm2
      vaddss  xmm2, xmm1, xmm0
    }
  }
  __asm { vmulss  xmm1, xmm12, xmm4 }
LABEL_14:
  __asm
  {
    vaddss  xmm3, xmm2, xmm1
    vaddss  xmm2, xmm3, [rsp+110h+var_E8]
    vmovss  [rsp+110h+var_E8], xmm2
  }
LABEL_15:
  __asm { vmovsd  xmm0, [rsp+110h+var_F0] }
  v69 = v119;
  __asm { vmovsd  qword ptr [rdi], xmm0 }
  *(_DWORD *)(_RDI + 8) = v69;
LABEL_16:
  _R11 = &v123;
  __asm
  {
    vmovaps xmm6, xmmword ptr [r11-10h]
    vmovaps xmm7, xmmword ptr [r11-20h]
    vmovaps xmm8, xmmword ptr [r11-30h]
    vmovaps xmm9, xmmword ptr [r11-40h]
    vmovaps xmm10, xmmword ptr [r11-50h]
    vmovaps xmm11, xmmword ptr [r11-60h]
    vmovaps xmm12, xmmword ptr [r11-70h]
  }
  return _RDI;
}
