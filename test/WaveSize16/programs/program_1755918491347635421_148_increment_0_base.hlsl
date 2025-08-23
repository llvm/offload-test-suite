[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() & 1) == 1)) {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMin(result));
    }
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
          if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
            result = (result + WaveActiveMax(result));
          }
        }
      }
    case 1: {
        uint counter1 = 0;
        while ((counter1 < 2)) {
          counter1 = (counter1 + 1);
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
            result = (result + WaveActiveSum(3));
          }
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveSum(result));
          }
        }
        break;
      }
    case 2: {
        if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10))) {
          if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 4)));
          }
        }
        break;
      }
    default: {
        result = (result + WaveActiveSum(99));
        break;
      }
    }
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveSum(result));
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
          uint counter2 = 0;
          while ((counter2 < 3)) {
            counter2 = (counter2 + 1);
            if ((WaveGetLaneIndex() == 3)) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
            if (((WaveGetLaneIndex() & 1) == 0)) {
              if (((WaveGetLaneIndex() & 1) == 0)) {
                result = (result + WaveActiveMin(result));
              }
            } else {
            if ((WaveGetLaneIndex() == 14)) {
              result = (result + WaveActiveMin(7));
            }
            if ((WaveGetLaneIndex() == 5)) {
              result = (result + WaveActiveMax(result));
            }
          }
        }
        break;
      }
    case 2: {
        if ((WaveGetLaneIndex() == 1)) {
          uint counter3 = 0;
          while ((counter3 < 3)) {
            counter3 = (counter3 + 1);
            if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 0))) {
              result = (result + WaveActiveSum(2));
            }
            if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 12))) {
              result = (result + WaveActiveMax(1));
            }
          }
          if ((WaveGetLaneIndex() == 1)) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
        }
        break;
      }
    }
    break;
  }
  case 1: {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      for (uint i4 = 0; (i4 < 2); i4 = (i4 + 1)) {
        if ((WaveGetLaneIndex() == 7)) {
          result = (result + WaveActiveMin(result));
        }
        for (uint i5 = 0; (i5 < 2); i5 = (i5 + 1)) {
          if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10))) {
            result = (result + WaveActiveSum((WaveGetLaneIndex() + 2)));
          }
          if ((i5 == 1)) {
            continue;
          }
        }
        if ((WaveGetLaneIndex() == 6)) {
          result = (result + WaveActiveMin(result));
        }
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
    }
    break;
  }
  }
}
