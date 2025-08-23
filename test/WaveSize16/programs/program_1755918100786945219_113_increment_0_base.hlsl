[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 2)) {
      case 0: {
          if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 12))) {
            if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 12))) {
              result = (result + WaveActiveMin(result));
            }
            if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveSum(result));
            }
          } else {
          if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 9))) {
            result = (result + WaveActiveMax(result));
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
    if ((WaveGetLaneIndex() == 15)) {
      if ((WaveGetLaneIndex() == 15)) {
        result = (result + WaveActiveSum(result));
      }
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
            if ((WaveGetLaneIndex() >= 11)) {
              result = (result + WaveActiveSum(4));
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
      if ((WaveGetLaneIndex() == 11)) {
        result = (result + WaveActiveMax(result));
      }
    } else {
    if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
    uint counter1 = 0;
    while ((counter1 < 3)) {
      counter1 = (counter1 + 1);
      if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMax(2));
      }
      uint counter2 = 0;
      while ((counter2 < 3)) {
        counter2 = (counter2 + 1);
        if ((WaveGetLaneIndex() == 7)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
        }
      }
      if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 11))) {
        result = (result + WaveActiveMax(3));
      }
    }
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
}
