[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() == 4)) {
    if ((WaveGetLaneIndex() == 13)) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
    uint counter0 = 0;
    while ((counter0 < 3)) {
      counter0 = (counter0 + 1);
      if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 10))) {
        if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
        }
        if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
        }
      } else {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMax(result));
      }
    }
    if ((WaveGetLaneIndex() >= 12)) {
      result = (result + WaveActiveMin(WaveGetLaneIndex()));
    }
  }
  if ((WaveGetLaneIndex() == 5)) {
    result = (result + WaveActiveSum(WaveGetLaneIndex()));
  }
  }
  if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 6))) {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
      for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
        if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 2))) {
          result = (result + WaveActiveSum(result));
        }
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 1))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveSum(2));
      }
    }
    if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 9))) {
      result = (result + WaveActiveSum(result));
    }
  }
}
