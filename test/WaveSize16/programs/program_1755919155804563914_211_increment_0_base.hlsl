[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13))) {
    if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 10))) {
      result = (result + WaveActiveSum(result));
    }
    for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
    }
  }
  if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 15))) {
    if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 11))) {
      result = (result + WaveActiveMax(WaveGetLaneIndex()));
    }
    if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
      result = (result + WaveActiveMax(3));
    }
  }
}
