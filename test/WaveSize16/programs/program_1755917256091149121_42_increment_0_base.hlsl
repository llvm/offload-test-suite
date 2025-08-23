[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      for (uint i0 = 0; (i0 < 3); i0 = (i0 + 1)) {
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 10))) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
          }
          if ((WaveGetLaneIndex() == 14)) {
            if ((WaveGetLaneIndex() == 15)) {
              result = (result + WaveActiveMax((WaveGetLaneIndex() + 4)));
            }
            if ((WaveGetLaneIndex() == 9)) {
              result = (result + WaveActiveSum(result));
            }
          }
        }
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
        }
      }
      break;
    }
  case 1: {
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
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
          if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 0))) {
            if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 2))) {
              if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 14))) {
                result = (result + WaveActiveMin(result));
              }
            }
            if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 1))) {
              result = (result + WaveActiveMin(WaveGetLaneIndex()));
            }
          } else {
          if ((WaveGetLaneIndex() == 4)) {
            result = (result + WaveActiveMin(WaveGetLaneIndex()));
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
            }
          case 2: {
              if (true) {
                result = (result + WaveActiveSum(3));
              }
              break;
            }
          }
        }
        break;
      }
    }
    break;
  }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() == 3)) {
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
        if ((WaveGetLaneIndex() == 15)) {
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
  }
}
