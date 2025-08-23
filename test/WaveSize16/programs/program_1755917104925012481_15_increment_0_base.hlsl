[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
            if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
              result = (result + WaveActiveMax(result));
            }
          }
          break;
        }
      case 1: {
          if ((WaveGetLaneIndex() == 11)) {
            if ((WaveGetLaneIndex() == 13)) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
            if ((WaveGetLaneIndex() == 14)) {
              result = (result + WaveActiveMax(8));
            }
          } else {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin(result));
          }
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveSum(1));
          }
        }
        break;
      }
    case 2: {
        if (true) {
          result = (result + WaveActiveSum(3));
        }
        break;
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
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 7))) {
        if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 3))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        uint counter0 = 0;
        while ((counter0 < 2)) {
          counter0 = (counter0 + 1);
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveSum(result));
          }
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMax(result));
          }
        }
        if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 7))) {
          result = (result + WaveActiveMax(3));
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
      if ((WaveGetLaneIndex() >= 11)) {
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveMin(result));
          }
        }
      }
      break;
    }
  case 3: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
            if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
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
          if (true) {
            result = (result + WaveActiveSum(3));
          }
          break;
        }
      }
      break;
    }
  }
}
