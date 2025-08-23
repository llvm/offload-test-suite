[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 6))) {
    if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
      result = (result + WaveActiveMax(result));
    }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      uint counter0 = 0;
      while ((counter0 < 3)) {
        counter0 = (counter0 + 1);
        if ((WaveGetLaneIndex() >= 11)) {
          result = (result + WaveActiveSum(result));
        }
        if ((WaveGetLaneIndex() >= 11)) {
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
        }
        if ((WaveGetLaneIndex() >= 12)) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  case 1: {
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 4))) {
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 6))) {
          result = (result + WaveActiveMax(result));
        }
        for (uint i1 = 0; (i1 < 2); i1 = (i1 + 1)) {
          uint counter2 = 0;
          while ((counter2 < 3)) {
            counter2 = (counter2 + 1);
            if ((WaveGetLaneIndex() == 13)) {
              result = (result + WaveActiveMin(result));
            }
          }
          if ((WaveGetLaneIndex() >= 9)) {
            result = (result + WaveActiveSum(result));
          }
        }
      }
    }
  case 2: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          uint counter3 = 0;
          while ((counter3 < 3)) {
            counter3 = (counter3 + 1);
            if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
              if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
                result = (result + WaveActiveMax(result));
              }
            } else {
            if (((WaveGetLaneIndex() & 1) == 1)) {
              result = (result + WaveActiveMin(7));
            }
          }
          if ((counter3 == 2)) {
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
    break;
  }
  case 3: {
    for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
      if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 1))) {
        result = (result + WaveActiveMax(result));
      }
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 13))) {
        uint counter5 = 0;
        while ((counter5 < 2)) {
          counter5 = (counter5 + 1);
          if ((WaveGetLaneIndex() >= 12)) {
            result = (result + WaveActiveMin(result));
          }
        }
      } else {
      if ((WaveGetLaneIndex() < 4)) {
        result = (result + WaveActiveMin(result));
      }
      for (uint i6 = 0; (i6 < 2); i6 = (i6 + 1)) {
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
        }
        if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveSum(result));
        }
        if ((i6 == 1)) {
          break;
        }
      }
    }
    if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 10))) {
      result = (result + WaveActiveMax(WaveGetLaneIndex()));
    }
    if ((i4 == 1)) {
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
  for (uint i7 = 0; (i7 < 3); i7 = (i7 + 1)) {
    if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 4))) {
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 11))) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
      }
      for (uint i8 = 0; (i8 < 3); i8 = (i8 + 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
      }
    }
    if ((WaveGetLaneIndex() < 6)) {
      result = (result + WaveActiveMin(result));
    }
  }
}
