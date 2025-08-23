[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
    uint counter0 = 0;
    while ((counter0 < 3)) {
      counter0 = (counter0 + 1);
      uint counter1 = 0;
      while ((counter1 < 2)) {
        counter1 = (counter1 + 1);
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
          result = (result + WaveActiveMax(7));
        }
      }
    }
  }
  if (((WaveGetLaneIndex() & 1) == 0)) {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
    uint counter2 = 0;
    while ((counter2 < 3)) {
      counter2 = (counter2 + 1);
      if ((WaveGetLaneIndex() == 11)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
      if ((WaveGetLaneIndex() < 4)) {
        if ((WaveGetLaneIndex() == 10)) {
          if ((WaveGetLaneIndex() == 9)) {
            result = (result + WaveActiveSum(result));
          }
          if ((WaveGetLaneIndex() == 11)) {
            result = (result + WaveActiveSum(result));
          }
        }
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      }
      if ((WaveGetLaneIndex() == 12)) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
      }
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMin(result));
        }
        for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMin(result));
          }
          if ((i4 == 1)) {
            continue;
          }
        }
        if ((WaveGetLaneIndex() < 3)) {
          result = (result + WaveActiveMax(result));
        }
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 15))) {
        if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 9))) {
          result = (result + WaveActiveSum(result));
        }
        for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
          if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 9))) {
            result = (result + WaveActiveMax(result));
          }
          if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveMin(result));
          }
        }
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
}
