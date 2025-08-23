[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
            }
            uint counter0 = 0;
            while ((counter0 < 2)) {
              counter0 = (counter0 + 1);
              if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 0))) {
                result = (result + WaveActiveMin(WaveGetLaneIndex()));
              }
              if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 3))) {
                result = (result + WaveActiveMax(8));
              }
            }
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveMax(result));
            }
          } else {
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          uint counter1 = 0;
          while ((counter1 < 3)) {
            counter1 = (counter1 + 1);
            if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
              result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
            }
            if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
            }
          }
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
            result = (result + WaveActiveMax(result));
          }
        }
        break;
      }
    case 1: {
        uint counter2 = 0;
        while ((counter2 < 2)) {
          counter2 = (counter2 + 1);
          for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
            if ((WaveGetLaneIndex() >= 9)) {
              result = (result + WaveActiveSum(result));
            }
            if ((i3 == 1)) {
              continue;
            }
          }
          if ((WaveGetLaneIndex() == 6)) {
            result = (result + WaveActiveMin(6));
          }
        }
        break;
      }
    case 2: {
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 9))) {
          if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 0))) {
            result = (result + WaveActiveMax(7));
          }
          uint counter4 = 0;
          while ((counter4 < 3)) {
            counter4 = (counter4 + 1);
            if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 12))) {
              result = (result + WaveActiveMax(3));
            }
            if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveMin(result));
            }
          }
          if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 6))) {
            result = (result + WaveActiveSum(result));
          }
        } else {
        for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 3))) {
            result = (result + WaveActiveMax(result));
          }
          if ((i5 == 2)) {
            break;
          }
        }
        if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMin(result));
        }
      }
      break;
    }
  }
  break;
  }
  case 1: {
    if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 0))) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
      uint counter6 = 0;
      while ((counter6 < 2)) {
        counter6 = (counter6 + 1);
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveMin(result));
        }
        for (uint i7 = 0; (i7 < 2); i7 = (i7 + 1)) {
          if ((WaveGetLaneIndex() >= 12)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMin(result));
        }
      }
    }
    break;
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
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 9))) {
            if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveSum(result));
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
            if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 12))) {
              result = (result + WaveActiveMax(result));
            }
          } else {
          if ((WaveGetLaneIndex() < 2)) {
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
    break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
  for (uint i8 = 0; (i8 < 3); i8 = (i8 + 1)) {
    if ((WaveGetLaneIndex() == 1)) {
      if ((WaveGetLaneIndex() < 3)) {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        if ((WaveGetLaneIndex() < 4)) {
          result = (result + WaveActiveMax(3));
        }
      }
    }
    if ((WaveGetLaneIndex() >= 9)) {
      result = (result + WaveActiveMin(2));
    }
  }
}
