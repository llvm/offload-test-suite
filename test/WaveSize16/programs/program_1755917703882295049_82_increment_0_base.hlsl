[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
    }
    if ((i0 == 2)) {
      break;
    }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 15))) {
        if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
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
        default: {
            result = (result + WaveActiveSum(99));
            break;
          }
        }
        if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      } else {
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
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
        result = (result + WaveActiveMax(result));
      }
    }
    break;
  }
  case 2: {
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        if ((WaveGetLaneIndex() >= 12)) {
          if ((WaveGetLaneIndex() < 3)) {
            result = (result + WaveActiveMax(5));
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
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
          if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 14))) {
            result = (result + WaveActiveSum(result));
          }
        }
        break;
      }
    }
    break;
  }
  case 3: {
    if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 15))) {
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveMin(result));
      }
      uint counter1 = 0;
      while ((counter1 < 3)) {
        counter1 = (counter1 + 1);
        if ((WaveGetLaneIndex() == 1)) {
          result = (result + WaveActiveMax(result));
        }
      }
      if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveMax(result));
      }
    } else {
    if ((WaveGetLaneIndex() == 7)) {
      result = (result + WaveActiveMax(result));
    }
  }
  break;
  }
  }
}
