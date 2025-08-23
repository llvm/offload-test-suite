[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 10))) {
    if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
    for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
      if ((WaveGetLaneIndex() == 2)) {
        result = (result + WaveActiveMax(result));
      }
    }
    if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveMax(WaveGetLaneIndex()));
    }
  } else {
  if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
    result = (result + WaveActiveSum(WaveGetLaneIndex()));
  }
  if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 10))) {
    if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveMin(9));
    }
  }
  }
}
