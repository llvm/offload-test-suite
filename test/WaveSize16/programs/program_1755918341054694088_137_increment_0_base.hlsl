[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  uint counter0 = 0;
  while ((counter0 < 2)) {
    counter0 = (counter0 + 1);
    for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
      if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveSum(result));
      }
      if ((WaveGetLaneIndex() >= 10)) {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveMin(result));
        }
        if ((WaveGetLaneIndex() == 13)) {
          if ((WaveGetLaneIndex() == 10)) {
            result = (result + WaveActiveMax(result));
          }
          if ((WaveGetLaneIndex() == 3)) {
            result = (result + WaveActiveMax(result));
          }
        }
        if ((WaveGetLaneIndex() >= 9)) {
          result = (result + WaveActiveMin(result));
        }
      }
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
    }
    if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 11))) {
      result = (result + WaveActiveMin(WaveGetLaneIndex()));
    }
  }
  if ((WaveGetLaneIndex() < 8)) {
    if ((WaveGetLaneIndex() < 5)) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
    for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
      if ((WaveGetLaneIndex() == 14)) {
        if ((WaveGetLaneIndex() == 2)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
        }
      }
      if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 2))) {
        result = (result + WaveActiveMin(6));
      }
    }
    if ((WaveGetLaneIndex() < 2)) {
      result = (result + WaveActiveMin(result));
    }
  }
}
