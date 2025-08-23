[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 1))) {
    if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 3))) {
      result = (result + WaveActiveMin(WaveGetLaneIndex()));
    }
    switch ((WaveGetLaneIndex() % 2)) {
    case 0: {
        if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 15))) {
          uint counter0 = 0;
          while ((counter0 < 3)) {
            counter0 = (counter0 + 1);
            if ((WaveGetLaneIndex() < 6)) {
              result = (result + WaveActiveMax(result));
            }
          }
        }
        break;
      }
    case 1: {
        if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 4))) {
          if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 2))) {
            result = (result + WaveActiveMin(result));
          }
          uint counter1 = 0;
          while ((counter1 < 3)) {
            counter1 = (counter1 + 1);
            if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 13))) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
          }
        } else {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveSum(8));
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
        if ((WaveGetLaneIndex() < 8)) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
      }
      break;
    }
  }
  if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 2))) {
    result = (result + WaveActiveSum(10));
  }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if (((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 5))) {
        if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveMin(result));
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
    }
  case 1: {
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
      break;
    }
  }
}
