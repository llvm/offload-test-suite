[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
            if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
              result = (result + WaveActiveMin(result));
            }
            if ((WaveGetLaneIndex() == 1)) {
              if ((WaveGetLaneIndex() == 6)) {
                result = (result + WaveActiveMin(result));
              }
              if ((WaveGetLaneIndex() == 1)) {
                result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
              }
            }
            if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
              result = (result + WaveActiveSum(result));
            }
          }
          break;
        }
      case 2: {
          switch ((WaveGetLaneIndex() % 3)) {
          case 0: {
              if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 15))) {
                if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
                  result = (result + WaveActiveMax(WaveGetLaneIndex()));
                }
                if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
                  result = (result + WaveActiveSum(result));
                }
              } else {
              if ((WaveGetLaneIndex() == 2)) {
                result = (result + WaveActiveMin(5));
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
}
