[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
    if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
    }
    if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 12))) {
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveSum(result));
      }
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 1))) {
          result = (result + WaveActiveMin(result));
        }
        if ((counter1 == 1)) {
          break;
        }
      }
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveSum(5));
      }
    }
    if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
      result = (result + WaveActiveMax(result));
    }
    if ((i0 == 2)) {
      break;
    }
  }
}
