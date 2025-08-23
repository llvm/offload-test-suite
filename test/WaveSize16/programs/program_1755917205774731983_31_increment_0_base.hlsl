[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() & 1) == 0)) {
    if ((WaveGetLaneIndex() == 7)) {
      if ((WaveGetLaneIndex() == 7)) {
        result = (result + WaveActiveSum(2));
      }
      uint counter0 = 0;
      while ((counter0 < 2)) {
        counter0 = (counter0 + 1);
        if ((WaveGetLaneIndex() < 6)) {
          result = (result + WaveActiveSum(result));
        }
      }
      if ((WaveGetLaneIndex() == 14)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
    }
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveMax(result));
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      if ((WaveGetLaneIndex() >= 8)) {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveMax(6));
        }
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if ((WaveGetLaneIndex() == 9)) {
            result = (result + WaveActiveSum(result));
          }
          if ((WaveGetLaneIndex() == 6)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        if ((WaveGetLaneIndex() >= 9)) {
          result = (result + WaveActiveMin(7));
        }
      }
      break;
    }
  default: {
      result = (result + WaveActiveSum(99));
      break;
    }
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
      break;
    }
  case 2: {
      uint counter2 = 0;
      while ((counter2 < 3)) {
        counter2 = (counter2 + 1);
        for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
          if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11))) {
            if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveMax(1));
            }
          }
          if ((i3 == 2)) {
            break;
          }
        }
      }
      break;
    }
  }
}
