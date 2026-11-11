__int64 __fastcall sub_2C4B5C(double a1, __m128 _XMM1, __int64 a3, __int64 a4, __int64 a5, __int64 _RCX)
{
  char v6; // cf

  __asm
  {
    vmovss  xmm0, dword ptr [rcx]
    vmovdqu xmm2, xmmword ptr cs:1706090h
    vandps  xmm0, xmm0, xmm2
    vcomiss xmm0, xmm1
  }
  if ( !v6 )
    return 0;
  __asm
  {
    vmovss  xmm0, dword ptr [rcx+4]
    vandps  xmm0, xmm0, xmm2
    vcomiss xmm0, xmm1
  }
  __asm
  {
    vmovss  xmm0, dword ptr [rcx+8]
    vandps  xmm0, xmm0, xmm2
    vcomiss xmm0, xmm1
  }
  return 1;
}
