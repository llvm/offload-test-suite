[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 4))) {
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 3))) {
          result = (result + WaveActiveMax(result));
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
      } else {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveSum(3));
      }
      switch ((WaveGetLaneIndex() % 2)) {
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
      default: {
          result = (result + WaveActiveSum(99));
          break;
        }
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveSum(result));
      }
    }
    break;
  }
  case 1: {
    uint counter0 = 0;
    while ((counter0 < 2)) {
      counter0 = (counter0 + 1);
      if ((WaveGetLaneIndex() < 7)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
        if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
        }
        if ((i1 == 1)) {
          continue;
        }
      }
    }
    break;
  }
  case 2: {
    if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 4))) {
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 8))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
    } else {
    if ((WaveGetLaneIndex() < 5)) {
      result = (result + WaveActiveMax(WaveGetLaneIndex()));
    }
    uint counter2 = 0;
    while ((counter2 < 2)) {
      counter2 = (counter2 + 1);
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 3))) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
      }
    }
    if ((WaveGetLaneIndex() >= 12)) {
      result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
    }
  }
  break;
  }
  case 3: {
    uint counter3 = 0;
    while ((counter3 < 3)) {
      counter3 = (counter3 + 1);
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 12))) {
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMin(result));
        }
      }
    }
    break;
  }
  }
  for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
    if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 11))) {
      result = (result + WaveActiveSum(8));
    }
    if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 14))) {
      result = (result + WaveActiveMin(result));
    }
    if ((i4 == 1)) {
      continue;
    }
  }
}
