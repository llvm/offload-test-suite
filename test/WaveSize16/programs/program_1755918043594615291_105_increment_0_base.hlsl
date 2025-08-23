[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        uint counter0 = 0;
        while ((counter0 < 2)) {
          counter0 = (counter0 + 1);
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMax(result));
          }
          for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
            if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 11))) {
              result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
            }
            if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
              result = (result + WaveActiveMax(result));
            }
          }
        }
      }
      break;
    }
  case 1: {
      if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 14))) {
        if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 2))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
        switch ((WaveGetLaneIndex() % 3)) {
        case 0: {
            if ((WaveGetLaneIndex() < 1)) {
              if ((WaveGetLaneIndex() < 4)) {
                result = (result + WaveActiveMin(WaveGetLaneIndex()));
              }
            } else {
            if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
              result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
            }
            if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
              result = (result + WaveActiveMax(result));
            }
          }
          break;
        }
      case 1: {
          uint counter2 = 0;
          while ((counter2 < 2)) {
            counter2 = (counter2 + 1);
            if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
              result = (result + WaveActiveSum(result));
            }
            if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 14))) {
              result = (result + WaveActiveMax(result));
            }
          }
          break;
        }
      case 2: {
          uint counter3 = 0;
          while ((counter3 < 3)) {
            counter3 = (counter3 + 1);
            if ((WaveGetLaneIndex() >= 9)) {
              result = (result + WaveActiveMax(WaveGetLaneIndex()));
            }
            if ((WaveGetLaneIndex() >= 8)) {
              result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
            }
            if ((counter3 == 2)) {
              break;
            }
          }
          break;
        }
      }
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 15))) {
        result = (result + WaveActiveSum(result));
      }
    } else {
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveMax(result));
    }
    if (((WaveGetLaneIndex() & 1) == 1)) {
      result = (result + WaveActiveSum(result));
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
  uint counter4 = 0;
  while ((counter4 < 3)) {
    counter4 = (counter4 + 1);
    if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 11))) {
      result = (result + WaveActiveMin(result));
    }
  }
  if ((WaveGetLaneIndex() >= 12)) {
    if ((WaveGetLaneIndex() >= 15)) {
      result = (result + WaveActiveMin(WaveGetLaneIndex()));
    }
    uint counter5 = 0;
    while ((counter5 < 2)) {
      counter5 = (counter5 + 1);
      if ((WaveGetLaneIndex() < 3)) {
        result = (result + WaveActiveSum(result));
      }
      for (uint i6 = 0; (i6 < 3); i6 = (i6 + 1)) {
        if ((WaveGetLaneIndex() == 3)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      }
      if ((WaveGetLaneIndex() < 6)) {
        result = (result + WaveActiveMax(result));
      }
    }
    if ((WaveGetLaneIndex() >= 10)) {
      result = (result + WaveActiveSum(2));
    }
  }
}
