[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(1));
        }
        if ((WaveGetLaneIndex() >= 15)) {
          if ((WaveGetLaneIndex() >= 14)) {
            result = (result + WaveActiveMin(result));
          }
          if ((WaveGetLaneIndex() >= 9)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMin(10));
        }
      }
    }
  case 1: {
      uint counter0 = 0;
      while ((counter0 < 3)) {
        counter0 = (counter0 + 1);
        for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
          if ((WaveGetLaneIndex() < 5)) {
            result = (result + WaveActiveMax(result));
          }
          if ((WaveGetLaneIndex() == 7)) {
            if ((WaveGetLaneIndex() == 11)) {
              result = (result + WaveActiveMin(result));
            }
          } else {
          if ((WaveGetLaneIndex() >= 8)) {
            result = (result + WaveActiveSum(5));
          }
        }
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveMin(result));
        }
        if ((i1 == 1)) {
          continue;
        }
      }
    }
    break;
  }
  }
  uint counter2 = 0;
  while ((counter2 < 2)) {
    counter2 = (counter2 + 1);
    if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 2))) {
      result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
    }
    uint counter3 = 0;
    while ((counter3 < 3)) {
      counter3 = (counter3 + 1);
      switch ((WaveGetLaneIndex() % 2)) {
      case 0: {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
            if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
          }
          break;
        }
      case 1: {
          if (((WaveGetLaneIndex() % 2) == 0)) {
            result = (result + WaveActiveSum(2));
          }
          break;
        }
      default: {
          result = (result + WaveActiveSum(99));
          break;
        }
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
      }
    }
  }
}
