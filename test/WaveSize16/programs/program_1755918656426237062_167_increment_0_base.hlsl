[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
    uint counter0 = 0;
    while ((counter0 < 2)) {
      counter0 = (counter0 + 1);
      if ((WaveGetLaneIndex() == 1)) {
        result = (result + WaveActiveSum(result));
      }
      for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
        }
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
        if ((i1 == 1)) {
          continue;
        }
      }
    }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() < 3)) {
        if ((WaveGetLaneIndex() < 7)) {
          result = (result + WaveActiveSum(result));
        }
        uint counter2 = 0;
        while ((counter2 < 3)) {
          counter2 = (counter2 + 1);
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          uint counter3 = 0;
          while ((counter3 < 3)) {
            counter3 = (counter3 + 1);
            if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
              result = (result + WaveActiveMin(result));
            }
          }
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin(result));
          }
        }
        if ((WaveGetLaneIndex() < 1)) {
          result = (result + WaveActiveMax(result));
        }
      } else {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMin(result));
      }
      switch ((WaveGetLaneIndex() % 2)) {
      case 0: {
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
            if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveSum(result));
            }
          }
          break;
        }
      case 1: {
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12))) {
            if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 13))) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
            if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveMax(result));
            }
          }
          break;
        }
      }
    }
    break;
  }
  case 1: {
    if (((WaveGetLaneIndex() % 2) == 0)) {
      result = (result + WaveActiveSum(2));
    }
  }
  case 2: {
    if (true) {
      result = (result + WaveActiveSum(3));
    }
    break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
}
