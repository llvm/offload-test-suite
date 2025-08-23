[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 3)) {
  case 0: {
      if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 5))) {
        if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 11))) {
          result = (result + WaveActiveMin(2));
        }
        uint counter0 = 0;
        while ((counter0 < 2)) {
          counter0 = (counter0 + 1);
          if (((WaveGetLaneIndex() < 1) || (WaveGetLaneIndex() >= 12))) {
            result = (result + WaveActiveMin(result));
          }
          if (((WaveGetLaneIndex() < 2) || (WaveGetLaneIndex() >= 12))) {
            result = (result + WaveActiveMin(8));
          }
        }
        if ((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 1))) {
          result = (result + WaveActiveMin(2));
        }
      }
      break;
    }
  case 1: {
      if (((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14)) || (WaveGetLaneIndex() == 14))) {
        if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 7)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 4))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
        }
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if ((WaveGetLaneIndex() == 1)) {
            result = (result + WaveActiveMax(result));
          }
          if ((i1 == 1)) {
            continue;
          }
        }
        if ((((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 5))) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 5)));
        }
      } else {
      if (((WaveGetLaneIndex() < 3) || (WaveGetLaneIndex() >= 11))) {
        result = (result + WaveActiveMin(WaveGetLaneIndex()));
      }
      if ((WaveGetLaneIndex() >= 14)) {
        if ((WaveGetLaneIndex() >= 14)) {
          result = (result + WaveActiveMax((WaveGetLaneIndex() + 1)));
        }
      }
      if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 13))) {
        result = (result + WaveActiveMin(result));
      }
    }
    break;
  }
  case 2: {
    if ((((WaveGetLaneIndex() == 1) || (WaveGetLaneIndex() == 9)) || (WaveGetLaneIndex() == 14))) {
      if (((WaveGetLaneIndex() == 6) || (WaveGetLaneIndex() == 10))) {
        result = (result + WaveActiveMax(result));
      }
      for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
        if ((((WaveGetLaneIndex() == 2) || (WaveGetLaneIndex() == 8)) || (WaveGetLaneIndex() == 14))) {
          result = (result + WaveActiveMax(4));
        }
      }
      if (((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 14))) {
        result = (result + WaveActiveMin((WaveGetLaneIndex() + 1)));
      }
    }
    break;
  }
  default: {
    result = (result + WaveActiveSum(99));
    break;
  }
  }
}
