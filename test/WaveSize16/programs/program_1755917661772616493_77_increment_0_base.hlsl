[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      uint counter0 = 0;
      while ((counter0 < 2)) {
        counter0 = (counter0 + 1);
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          if ((WaveGetLaneIndex() < 1)) {
            result = (result + WaveActiveSum(result));
          }
        }
      }
      break;
    }
  case 1: {
      if ((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 0))) {
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 6))) {
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
        if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      } else {
      if ((WaveGetLaneIndex() >= 12)) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
      }
      for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMin(result));
        }
        if ((i2 == 1)) {
          continue;
        }
      }
      if ((WaveGetLaneIndex() < 2)) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
    }
    break;
  }
  case 2: {
    uint counter3 = 0;
    while ((counter3 < 3)) {
      counter3 = (counter3 + 1);
      if ((WaveGetLaneIndex() >= 12)) {
        result = (result + WaveActiveMin(result));
      }
      if ((WaveGetLaneIndex() == 15)) {
        if ((WaveGetLaneIndex() == 1)) {
          result = (result + WaveActiveMax(result));
        }
        if ((WaveGetLaneIndex() == 9)) {
          result = (result + WaveActiveMax(result));
        }
      }
      if ((WaveGetLaneIndex() >= 14)) {
        result = (result + WaveActiveMin(result));
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
          if (((WaveGetLaneIndex() % 2) == 0)) {
            result = (result + WaveActiveSum(2));
          }
          break;
        }
      case 2: {
          if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
            if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11))) {
              result = (result + WaveActiveMax(3));
            }
          }
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
  }
}
