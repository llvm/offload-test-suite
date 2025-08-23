[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
    }
  case 1: {
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
        if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveSum(2));
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
  }
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
        if ((WaveGetLaneIndex() == 4)) {
          if ((WaveGetLaneIndex() == 13)) {
            result = (result + WaveActiveMax(10));
          }
          if ((WaveGetLaneIndex() < 5)) {
            if ((WaveGetLaneIndex() < 1)) {
              result = (result + WaveActiveMin(result));
            }
          }
          if ((WaveGetLaneIndex() == 14)) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
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
            if (((WaveGetLaneIndex() & 1) == 0)) {
              if (((WaveGetLaneIndex() & 1) == 0)) {
                result = (result + WaveActiveMin(8));
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
        }
      } else {
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
      }
      switch ((WaveGetLaneIndex() % 4)) {
      case 0: {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveSum(result));
            }
          } else {
          if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 15))) {
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
    case 2: {
        uint counter1 = 0;
        while ((counter1 < 2)) {
          counter1 = (counter1 + 1);
          if ((WaveGetLaneIndex() >= 11)) {
            result = (result + WaveActiveMin(result));
          }
          if ((counter1 == 1)) {
            break;
          }
        }
        break;
      }
    case 3: {
        if ((WaveGetLaneIndex() == 9)) {
          if ((WaveGetLaneIndex() == 5)) {
            result = (result + WaveActiveMin(7));
          }
        }
        break;
      }
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
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
}
