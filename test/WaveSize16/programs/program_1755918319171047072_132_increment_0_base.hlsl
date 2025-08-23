[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 4))) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
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
          }
        }
        if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
        }
        if ((i0 == 1)) {
          continue;
        }
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
          if ((WaveGetLaneIndex() == 13)) {
            if ((WaveGetLaneIndex() == 0)) {
              result = (result + WaveActiveMin(result));
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
            if ((WaveGetLaneIndex() == 9)) {
              result = (result + WaveActiveMin(result));
            }
          }
          break;
        }
      }
    }
  case 2: {
      if (true) {
        result = (result + WaveActiveSum(3));
      }
      break;
    }
  }
  uint counter2 = 0;
  while ((counter2 < 2)) {
    counter2 = (counter2 + 1);
    if ((WaveGetLaneIndex() == 3)) {
      result = (result + WaveActiveMin(result));
    }
    for (uint i3 = 0; (i3 < 2); i3 = (i3 + 1)) {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
      if ((WaveGetLaneIndex() == 15)) {
        if ((WaveGetLaneIndex() == 5)) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
        }
      } else {
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveSum(result));
      }
    }
  }
  }
  if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
    uint counter4 = 0;
    while ((counter4 < 2)) {
      counter4 = (counter4 + 1);
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveMin(result));
      }
      for (uint i5 = 0; (i5 < 3); i5 = (i5 + 1)) {
        if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveSum(result));
        }
        if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveMax(result));
        }
      }
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMin(result));
      }
    }
    if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
      result = (result + WaveActiveMin(result));
    }
  }
}
