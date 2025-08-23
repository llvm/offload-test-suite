[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 9))) {
        uint counter0 = 0;
        while ((counter0 < 3)) {
          counter0 = (counter0 + 1);
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
          }
          for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
            if ((WaveGetLaneIndex() == 13)) {
              result = (result + WaveActiveMin(result));
            }
          }
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMin(result));
          }
        }
        if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 10))) {
          result = (result + WaveActiveSum(result));
        }
      } else {
      if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 13))) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
      }
      switch ((WaveGetLaneIndex() % 2)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
          break;
        }
      case 1: {
          if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 15))) {
            if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 13))) {
              result = (result + WaveActiveMax(result));
            }
          } else {
          if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 15))) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        break;
      }
    }
  }
  break;
  }
  case 1: {
    uint counter2 = 0;
    while ((counter2 < 2)) {
      counter2 = (counter2 + 1);
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12))) {
        if (((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMax(result));
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
      } else {
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
        result = (result + WaveActiveSum(result));
      }
      switch ((WaveGetLaneIndex() % 3)) {
      case 0: {
          if ((WaveGetLaneIndex() < 8)) {
            result = (result + WaveActiveSum(1));
          }
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
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
        result = (result + WaveActiveMax(result));
      }
    }
    if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 11))) {
      result = (result + WaveActiveSum(3));
    }
  }
  break;
  }
  case 2: {
    if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 14))) {
      if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 4))) {
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveMax(9));
        }
        if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 14))) {
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
            result = (result + WaveActiveMin(result));
          }
        }
      }
      if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMax(2));
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
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
}
