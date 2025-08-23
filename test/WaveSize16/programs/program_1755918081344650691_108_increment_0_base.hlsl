[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 6)) {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveMax(1));
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
        if ((WaveGetLaneIndex() < 3)) {
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
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 15))) {
        result = (result + WaveActiveSum(result));
      }
    }
    break;
  }
  case 1: {
    if ((WaveGetLaneIndex() < 7)) {
      if ((WaveGetLaneIndex() < 7)) {
        result = (result + WaveActiveSum(9));
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
    }
    break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
  if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
    switch ((WaveGetLaneIndex() % 2)) {
    case 0: {
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveSum(1));
        }
      }
    case 1: {
        if ((WaveGetLaneIndex() < 8)) {
          if ((WaveGetLaneIndex() >= 15)) {
            result = (result + WaveActiveMin(result));
          }
        }
        break;
      }
    }
  }
  if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
    result = (result + WaveActiveSum(1));
  } else {
  if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 13))) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
  }
  }
}
