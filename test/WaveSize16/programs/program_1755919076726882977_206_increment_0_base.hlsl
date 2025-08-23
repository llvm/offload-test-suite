[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((WaveGetLaneIndex() == 11)) {
    result = (result + WaveActiveSum((WaveGetLaneIndex() + 1)));
  } else {
  if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 1))) {
    result = (result + WaveActiveMin(2));
  } else {
  if ((WaveGetLaneIndex() < 2)) {
    result = (result + WaveActiveMax(3));
  } else {
  if (((WaveGetLaneIndex() & 1) == 0)) {
    result = (result + WaveActiveSum(4));
  } else {
  if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
    result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
  }
  }
  }
  }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
        }
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15))) {
          if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
          uint counter0 = 0;
          while ((counter0 < 2)) {
            counter0 = (counter0 + 1);
            if ((WaveGetLaneIndex() == 5)) {
              result = (result + WaveActiveMax(result));
            }
            if ((WaveGetLaneIndex() == 14)) {
              result = (result + WaveActiveMin(10));
            }
          }
          if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
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
  case 2: {
      if (true) {
        result = (result + WaveActiveSum(3));
      }
      break;
    }
  case 3: {
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() == 14)) {
            for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
              if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 15))) {
                result = (result + WaveActiveSum(1));
              }
            }
          }
          break;
        }
      case 1: {
          if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10))) {
            if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10))) {
              if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 9))) {
                result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
              }
              if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11))) {
                result = (result + WaveActiveMin(WaveGetLaneIndex()));
              }
            }
          } else {
          if ((WaveGetLaneIndex() == 7)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
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
  if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 14))) {
    if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
      result = (result + WaveActiveMax(WaveGetLaneIndex()));
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
    if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
      result = (result + WaveActiveMax(result));
    }
  }
}
