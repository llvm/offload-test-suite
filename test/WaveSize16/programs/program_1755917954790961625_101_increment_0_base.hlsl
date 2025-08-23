[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() == 14)) {
    if ((WaveGetLaneIndex() == 12)) {
      result = (result + WaveActiveSum(4));
    }
    uint counter0 = 0;
    while ((counter0 < 2)) {
      counter0 = (counter0 + 1);
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveSum(result));
      }
      if ((WaveGetLaneIndex() == 3)) {
        if ((WaveGetLaneIndex() == 6)) {
          result = (result + WaveActiveSum(7));
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMax(result));
          }
        }
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMax(result));
      }
    }
    if ((WaveGetLaneIndex() == 9)) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
    }
  } else {
  if ((WaveGetLaneIndex() == 4)) {
    if ((WaveGetLaneIndex() == 2)) {
      result = (result + WaveActiveMax(WaveGetLaneIndex()));
    }
    uint counter1 = 0;
    while ((counter1 < 3)) {
      counter1 = (counter1 + 1);
      if ((WaveGetLaneIndex() >= 8)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveSum(result));
        }
        if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
        }
      }
    }
    if ((WaveGetLaneIndex() == 1)) {
      result = (result + WaveActiveSum(result));
    }
  }
  }
}
