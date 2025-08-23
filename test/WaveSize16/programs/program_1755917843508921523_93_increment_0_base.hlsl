[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
    }
    if ((WaveGetLaneIndex() >= 8)) {
      if ((WaveGetLaneIndex() < 7)) {
        result = (result + WaveActiveMax(result));
      }
      uint counter1 = 0;
      while ((counter1 < 3)) {
        counter1 = (counter1 + 1);
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax(7));
        }
        if ((counter1 == 2)) {
          break;
        }
      }
      if ((WaveGetLaneIndex() < 5)) {
        result = (result + WaveActiveMin(result));
      }
    } else {
    if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 4))) {
      result = (result + WaveActiveMax(result));
    }
    if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8))) {
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveMin(result));
      }
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
    }
    if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 7))) {
      result = (result + WaveActiveSum(2));
    }
  }
  if (((WaveGetLaneIndex() & 1) == 1)) {
    result = (result + WaveActiveMin(5));
  }
  }
}
