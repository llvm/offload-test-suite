[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 15))) {
    if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 14))) {
      result = (result + WaveActiveSum(9));
    }
    if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 12))) {
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 9))) {
        result = (result + WaveActiveMax(WaveGetLaneIndex()));
      }
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 13))) {
        if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 11))) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
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
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMin(result));
        }
      }
      if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 5))) {
        result = (result + WaveActiveMax(result));
      }
    }
    if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15))) {
      result = (result + WaveActiveMin(result));
    }
  }
  switch ((WaveGetLaneIndex() % 4)) {
  case 0: {
      if ((WaveGetLaneIndex() < 8)) {
        result = (result + WaveActiveSum(1));
      }
      break;
    }
  case 1: {
      uint counter0 = 0;
      while ((counter0 < 2)) {
        counter0 = (counter0 + 1);
        if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMax(result));
        }
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 8))) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
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
  case 3: {
      uint counter2 = 0;
      while ((counter2 < 3)) {
        counter2 = (counter2 + 1);
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 2)));
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          if (((WaveGetLaneIndex() & 1) == 1)) {
            result = (result + WaveActiveMax((WaveGetLaneIndex() + 5)));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(3));
        }
      }
      break;
    }
  }
}
