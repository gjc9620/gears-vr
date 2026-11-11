__int64 __fastcall sub_2CE478(
        double a1,
        double a2,
        double a3,
        double a4,
        double a5,
        double a6,
        __m128 _XMM6,
        __m128 _XMM7,
        __int64 a9,
        __int64 a10,
        int *_RDX,
        __int64 _RCX)
{
  int v12; // eax

  *(_QWORD *)(_RCX + 32) = 1065353216;
  *(_QWORD *)(_RCX + 8) = 0;
  *(_QWORD *)(_RCX + 24) = 0;
  *(_QWORD *)_RCX = 1065353216;
  *(_QWORD *)(_RCX + 16) = 1065353216;
  *(_QWORD *)(_RCX + 40) = 0;
  v12 = *_RDX;
  __asm
  {
    vmovss  xmm5, dword ptr [rdx+4]
    vmovss  xmm1, dword ptr [rdx+10h]
    vmovss  xmm3, dword ptr [rdx+14h]
    vmovss  xmm0, dword ptr [rdx+20h]
    vmovss  xmm2, dword ptr [rdx+24h]
    vmovss  xmm4, dword ptr [rdx+28h]
    vmovaps [rsp+28h+var_18], xmm6
    vmovss  xmm6, dword ptr [rdx+18h]
    vmovaps [rsp+28h+var_28], xmm7
    vmovss  xmm7, dword ptr [rdx+8]
    vmovss  dword ptr [rcx+18h], xmm7
    vmovaps xmm7, [rsp+28h+var_28]
    vmovss  dword ptr [rcx+1Ch], xmm6
    vmovaps xmm6, [rsp+28h+var_18]
  }
  *(_DWORD *)_RCX = v12;
  __asm
  {
    vmovss  dword ptr [rcx+4], xmm1
    vmovss  dword ptr [rcx+8], xmm0
    vmovss  dword ptr [rcx+0Ch], xmm5
    vmovss  dword ptr [rcx+10h], xmm3
    vmovss  dword ptr [rcx+14h], xmm2
    vmovss  dword ptr [rcx+20h], xmm4
  }
  *(_DWORD *)(_RCX + 36) = _RDX[12];
  *(_DWORD *)(_RCX + 40) = _RDX[13];
  *(_DWORD *)(_RCX + 44) = _RDX[14];
  return _RCX;
}
