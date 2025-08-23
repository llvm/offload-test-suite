[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
    if ((WaveGetLaneIndex() == 5)) {
      result = (result + WaveActiveSum(result));
    }
    uint counter1 = 0;
    while ((counter1 < 2)) {
      counter1 = (counter1 + 1);
      if ((WaveGetLaneIndex() == 1)) {
        result = (result + WaveActiveMax(result));
      }
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 5))) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          if (((WaveGetLaneIndex() & 1) == 0)) {
            result = (result + WaveActiveMax(result));
          }
        }
        if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 9))) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      }
      if ((counter1 == 1)) {
        break;
      }
    }
    if ((WaveGetLaneIndex() == 13)) {
      result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if ((WaveGetLaneIndex() >= 9)) {
        if ((WaveGetLaneIndex() < 3)) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
          if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 4))) {
            result = (result + WaveActiveSum(result));
          }
          uint counter3 = 0;
          while ((counter3 < 3)) {
            counter3 = (counter3 + 1);
            if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 15))) {
              result = (result + WaveActiveSum(4));
            }
          }
          if ((((WaveGetLaneIndex() == 5) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 3))) {
            result = (result + WaveActiveMax(result));
          }
          if ((i2 == 1)) {
            break;
          }
        }
        if ((WaveGetLaneIndex() < 4)) {
          result = (result + WaveActiveMax(WaveGetLaneIndex()));
        }
      } else {
      if (((WaveGetLaneIndex() & 1) == 0)) {
        if ((WaveGetLaneIndex() >= 15)) {
          if ((WaveGetLaneIndex() < 4)) {
            result = (result + WaveActiveMax(WaveGetLaneIndex()));
          }
          if ((WaveGetLaneIndex() < 1)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
      }
      if ((WaveGetLaneIndex() < 2)) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
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
