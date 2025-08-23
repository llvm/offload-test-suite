[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
            if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
            }
            for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
              if ((WaveGetLaneIndex() < 3)) {
                result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
              }
              if ((WaveGetLaneIndex() < 6)) {
                result = (result + WaveActiveMin(result));
              }
            }
            if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12))) {
              result = (result + WaveActiveSum(result));
            }
          }
          break;
        }
      case 1: {
          for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
            if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
              result = (result + WaveActiveMin(result));
            }
            if (((WaveGetLaneIndex() & 1) == 0)) {
              if (((WaveGetLaneIndex() & 1) == 1)) {
                result = (result + WaveActiveSum(result));
              }
            }
            if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 10))) {
              result = (result + WaveActiveMax(result));
            }
          }
          break;
        }
      case 2: {
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
            if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12))) {
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
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
          }
        }
        break;
      }
    }
    break;
  }
  case 1: {
    if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
            if ((((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 3))) {
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
      default: {
          result = (result + WaveActiveSum(99));
          break;
        }
      }
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMin(result));
      }
    }
    break;
  }
  case 2: {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      uint counter4 = 0;
      while ((counter4 < 3)) {
        counter4 = (counter4 + 1);
        uint counter5 = 0;
        while ((counter5 < 3)) {
          counter5 = (counter5 + 1);
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
            result = (result + WaveActiveSum(4));
          }
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveSum(9));
      }
    }
    break;
  }
  case 3: {
    if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 15))) {
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 10))) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
    } else {
    uint counter6 = 0;
    while ((counter6 < 3)) {
      counter6 = (counter6 + 1);
      if ((WaveGetLaneIndex() == 1)) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
      }
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 13))) {
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveSum(result));
        }
      }
    }
    if ((WaveGetLaneIndex() >= 15)) {
      result = (result + WaveActiveMin(result));
    }
  }
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
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveMax(result));
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          result = (result + WaveActiveSum(result));
        }
      } else {
      for (uint i7 = 0; (i7 < 3); i7 = (i7 + 1)) {
        if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 8))) {
          result = (result + WaveActiveMax(4));
        }
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMin(result));
        }
      }
      if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 7))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
    }
    break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
}
