[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
    if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if ((WaveGetLaneIndex() == 10)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 14))) {
          if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 10))) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
          }
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 8))) {
            result = (result + WaveActiveMax(9));
          }
        }
        if ((WaveGetLaneIndex() == 4)) {
          result = (result + WaveActiveMax(result));
        }
      }
      if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
    } else {
    if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 11))) {
      result = (result + WaveActiveSum(5));
    }
    uint counter2 = 0;
    while ((counter2 < 2)) {
      counter2 = (counter2 + 1);
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveSum(result));
      }
      if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 7))) {
          result = (result + WaveActiveMin(7));
        }
      }
      if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 11))) {
        result = (result + WaveActiveMax(result));
      }
    }
    if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
      result = (result + WaveActiveMin(result));
    }
  }
  if ((i0 == 1)) {
    break;
  }
  }
}
