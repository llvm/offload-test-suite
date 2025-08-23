[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
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
          uint counter0 = 0;
          while ((counter0 < 3)) {
            counter0 = (counter0 + 1);
            if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
              result = (result + WaveActiveSum(result));
            }
            if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 13))) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
          }
          break;
        }
      }
      break;
    }
  case 3: {
      if ((WaveGetLaneIndex() == 15)) {
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
      if ((WaveGetLaneIndex() >= 13)) {
        if ((WaveGetLaneIndex() >= 11)) {
          result = (result + WaveActiveMax(result));
        }
      }
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMin(6));
      }
    }
    break;
  }
  default: {
    result = (result + WaveActiveSum(99));
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
      for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
        }
        if (((WaveGetLaneIndex() & 1) == 1)) {
          switch ((WaveGetLaneIndex() % 4)) {
          case 0: {
              if ((WaveGetLaneIndex() < 8)) {
                result = (result + WaveActiveSum(1));
              }
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
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 15))) {
          result = (result + WaveActiveMin(result));
        }
      }
      break;
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      switch ((WaveGetLaneIndex() % 3)) {
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
      if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 9))) {
        if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveMin(result));
        }
        if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 10))) {
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMax(result));
          }
          uint counter2 = 0;
          while ((counter2 < 2)) {
            counter2 = (counter2 + 1);
            if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 13))) {
              result = (result + WaveActiveMax(8));
            }
          }
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveMin(result));
          }
        }
        if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      }
      break;
    }
  }
}
