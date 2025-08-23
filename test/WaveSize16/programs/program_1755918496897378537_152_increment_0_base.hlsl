[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 12))) {
    if ((((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 2))) {
      result = (result + WaveActiveSum(9));
    }
    for (uint i0 = 0; (i0 < 2); i0 = (i0 + 1)) {
      if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 5))) {
        result = (result + WaveActiveMax(result));
      }
      uint counter1 = 0;
      while ((counter1 < 3)) {
        counter1 = (counter1 + 1);
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(result));
        }
        if ((WaveGetLaneIndex() == 0)) {
          if ((WaveGetLaneIndex() == 1)) {
            result = (result + WaveActiveSum(WaveGetLaneIndex()));
          }
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMax(result));
        }
      }
      if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 0))) {
        result = (result + WaveActiveMin(result));
      }
    }
    if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 0))) {
      result = (result + WaveActiveSum(WaveGetLaneIndex()));
    }
  }
  for (uint i2 = 0; (i2 < 2); i2 = (i2 + 1)) {
    if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 6)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 11))) {
      result = (result + WaveActiveSum(result));
    }
    uint counter3 = 0;
    while ((counter3 < 2)) {
      counter3 = (counter3 + 1);
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 10))) {
        result = (result + WaveActiveMin(result));
      }
      if (((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 13))) {
        if ((((WaveGetLaneIndex() == 4) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 13))) {
          result = (result + WaveActiveMin(WaveGetLaneIndex()));
        }
        switch ((WaveGetLaneIndex() % 4)) {
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
        if ((((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 6))) {
          result = (result + WaveActiveSum((WaveGetLaneIndex() + 3)));
        }
      }
    }
  }
}
