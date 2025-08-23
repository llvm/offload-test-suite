[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
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
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 3)));
        }
        if ((WaveGetLaneIndex() < 2)) {
          if ((WaveGetLaneIndex() >= 13)) {
            result = (result + WaveActiveMin(result));
          }
          uint counter0 = 0;
          while ((counter0 < 2)) {
            counter0 = (counter0 + 1);
            if (((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 8))) {
              result = (result + WaveActiveMin(result));
            }
            if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveSum(WaveGetLaneIndex()));
            }
          }
          if ((WaveGetLaneIndex() >= 15)) {
            result = (result + WaveActiveMin(result));
          }
        }
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 12))) {
          result = (result + WaveActiveMin(10));
        }
      }
    }
  case 1: {
      if (((WaveGetLaneIndex() % 2) == 0)) {
        result = (result + WaveActiveSum(2));
      }
      break;
    }
  case 2: {
      if (((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 9))) {
        if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMin(result));
        }
        uint counter1 = 0;
        while ((counter1 < 3)) {
          counter1 = (counter1 + 1);
          for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
            if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
              result = (result + WaveActiveMax(result));
            }
            if (((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 8))) {
              result = (result + WaveActiveSum((WaveGetLaneIndex() + 5)));
            }
            if ((i2 == 1)) {
              continue;
            }
          }
          if ((((WaveGetLaneIndex() == 7) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 12))) {
            result = (result + WaveActiveMin(result));
          }
        }
      } else {
      if (((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveMin(result));
      }
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(result));
        }
        for (uint i3 = 0; (i3 < 3); i3 = (i3 + 1)) {
          if ((WaveGetLaneIndex() < 5)) {
            result = (result + WaveActiveMax(result));
          }
          if ((i3 == 1)) {
            continue;
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(result));
        }
      }
      if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveMin(result));
      }
    }
    break;
  }
  }
}
