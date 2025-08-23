[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
    if (((WaveGetLaneIndex() & 1) == 0)) {
      result = (result + WaveActiveMin(result));
    }
    if ((WaveGetLaneIndex() >= 14)) {
      if ((WaveGetLaneIndex() >= 13)) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 2)));
      }
      uint counter1 = 0;
      while ((counter1 < 3)) {
        counter1 = (counter1 + 1);
        if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 5))) {
          result = (result + WaveActiveMin(result));
        }
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 0))) {
          result = (result + WaveActiveSum(5));
        }
      }
      if ((WaveGetLaneIndex() >= 14)) {
        result = (result + WaveActiveMin(result));
      }
    } else {
    if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
      result = (result + WaveActiveSum(result));
    }
    for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
      if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 14))) {
        result = (result + WaveActiveSum(result));
      }
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 14))) {
        result = (result + WaveActiveMin(10));
      }
    }
    if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 13))) {
      result = (result + WaveActiveMin(10));
    }
  }
  if (((WaveGetLaneIndex() & 1) == 0)) {
    result = (result + WaveActiveMax(result));
  }
  if ((i0 == 1)) {
    break;
  }
  }
  if (((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 10))) {
    if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 14))) {
      result = (result + WaveActiveSum(result));
    }
    uint counter3 = 0;
    while ((counter3 < 3)) {
      counter3 = (counter3 + 1);
      if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 11))) {
        result = (result + WaveActiveSum(result));
      }
      uint counter4 = 0;
      while ((counter4 < 3)) {
        counter4 = (counter4 + 1);
        if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
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
            break;
          }
        case 2: {
            if (true) {
              result = (result + WaveActiveSum(3));
            }
            break;
          }
        }
        if (((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 8))) {
          result = (result + WaveActiveMax(result));
        }
        if ((counter4 == 2)) {
          break;
        }
      }
      if (((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 12))) {
        result = (result + WaveActiveSum(WaveGetLaneIndex()));
      }
    }
    if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 10))) {
      result = (result + WaveActiveSum(3));
    }
  }
}
