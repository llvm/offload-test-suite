[numthreads(32, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
  uint result = 0;
  switch ((WaveGetLaneIndex() % 2)) {
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
        if (((WaveGetLaneIndex() < 4) || (WaveGetLaneIndex() >= 10))) {
          result = (result + WaveActiveMax(5));
        }
        for (uint i1 = 0; (i1 < 3); i1 = (i1 + 1)) {
          if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 12)) || (WaveGetLaneIndex() == 14))) {
            if ((((((WaveGetLaneIndex() == 0) || (WaveGetLaneIndex() == 4)) || (WaveGetLaneIndex() == 10)) || (WaveGetLaneIndex() == 15)) || (WaveGetLaneIndex() == 9))) {
              result = (result + WaveActiveMin(result));
            }
            if ((((((WaveGetLaneIndex() == 3) || (WaveGetLaneIndex() == 5)) || (WaveGetLaneIndex() == 11)) || (WaveGetLaneIndex() == 13)) || (WaveGetLaneIndex() == 12))) {
              result = (result + WaveActiveMin(6));
            }
          }
          if ((i1 == 2)) {
            break;
          }
        }
        if (((WaveGetLaneIndex() < 5) || (WaveGetLaneIndex() >= 12))) {
          result = (result + WaveActiveSum(result));
        }
      }
      break;
    }
  }
  switch ((WaveGetLaneIndex() % 2)) {
  case 0: {
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveSum(WaveGetLaneIndex()));
        }
        if (((WaveGetLaneIndex() & 1) == 0)) {
          result = (result + WaveActiveMin(result));
        }
      } else {
      for (uint i2 = 0; (i2 < 3); i2 = (i2 + 1)) {
        if ((WaveGetLaneIndex() >= 10)) {
          result = (result + WaveActiveMin((WaveGetLaneIndex() + 3)));
        }
        uint counter3 = 0;
        while ((counter3 < 2)) {
          counter3 = (counter3 + 1);
          if ((WaveGetLaneIndex() >= 15)) {
            result = (result + WaveActiveSum(6));
          }
        }
        if ((i2 == 1)) {
          continue;
        }
        if ((i2 == 2)) {
          break;
        }
      }
    }
  }
  case 1: {
    uint counter4 = 0;
    while ((counter4 < 2)) {
      counter4 = (counter4 + 1);
      if (((WaveGetLaneIndex() & 1) == 1)) {
        if (((WaveGetLaneIndex() & 1) == 0)) {
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
      }
      if (((WaveGetLaneIndex() & 1) == 0)) {
        result = (result + WaveActiveMax(result));
      }
    }
    break;
  }
  }
}
