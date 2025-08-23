[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 10))) {
        if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveMax(result));
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
        default: {
            result = (result + WaveActiveSum(99));
            break;
          }
        }
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 11))) {
        result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
      }
      uint counter0 = 0;
      while ((counter0 < 3)) {
        counter0 = (counter0 + 1);
        if ((WaveGetLaneIndex() < 4)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 10))) {
            result = (result + WaveActiveMin(result));
          }
          if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMax(1));
          }
        }
        if ((counter0 == 2)) {
          break;
        }
      }
    }
    break;
  }
  case 1: {
    switch ((WaveGetLaneIndex() % 3)) {
    case 0: {
        for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
          if ((WaveGetLaneIndex() >= 11)) {
            result = (result + WaveActiveMax(result));
          }
          uint counter3 = 0;
          while ((counter3 < 3)) {
            counter3 = (counter3 + 1);
            if (((WaveGetLaneIndex() & 1) == 0)) {
              result = (result + WaveActiveMin(WaveGetLaneIndex()));
            }
          }
        }
        break;
      }
    case 1: {
        if ((WaveGetLaneIndex() == 1)) {
          if ((WaveGetLaneIndex() == 10)) {
            result = (result + WaveActiveSum(result));
          }
          for (uint i4 = 0; (i4 < 3); i4 = (i4 + 1)) {
            if ((WaveGetLaneIndex() < 2)) {
              result = (result + WaveActiveMin(result));
            }
          }
          if ((WaveGetLaneIndex() == 9)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        } else {
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15))) {
          result = (result + WaveActiveSum(3));
        }
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      }
      break;
    }
  case 2: {
      if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 11))) {
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
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
      if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveMax(4));
      }
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 12))) {
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMax(result));
        }
      }
    }
    break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
  break;
  }
  }
}
