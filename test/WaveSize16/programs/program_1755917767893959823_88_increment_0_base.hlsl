[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() < 8)) {
    switch ((WaveGetLaneIndex() % 2)) {
    case 0: {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
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
  }
  uint counter0 = 0;
  while ((counter0 < 3)) {
    counter0 = (counter0 + 1);
    if ((WaveGetLaneIndex() < 6)) {
      result = (result + WaveActiveSum(result));
    }
    if ((WaveGetLaneIndex() == 3)) {
      if ((WaveGetLaneIndex() == 8)) {
        result = (result + WaveActiveMin(3));
      }
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 7)) {
            if ((WaveGetLaneIndex() < 6)) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
            if ((WaveGetLaneIndex() < 7)) {
              result = (result + WaveActiveSum(result));
            }
          } else {
          if ((WaveGetLaneIndex() == 8)) {
            result = (result + WaveActiveMax(result));
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
    case 2: {
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if ((i1 == 1)) {
            continue;
          }
          if ((i1 == 2)) {
            break;
          }
        }
        break;
      }
    default: {
        result = (result + WaveActiveSum(99));
        break;
      }
    }
  } else {
  if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 3))) {
    if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 11))) {
      result = (result + WaveActiveMax(result));
    }
    switch ((WaveGetLaneIndex() % 4)) {
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
        if (true) {
          result = (result + WaveActiveSum(3));
        }
        break;
      }
    case 3: {
        if ((WaveGetLaneIndex() < 20)) {
          result = (result + WaveActiveSum(4));
        }
        break;
      }
    }
    if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 15))) {
      result = (result + WaveActiveMax(result));
    }
  }
  }
  if ((WaveGetLaneIndex() < 4)) {
    result = (result + WaveActiveMax(WaveGetLaneIndex()));
  }
  }
}
